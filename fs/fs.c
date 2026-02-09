#include "fs.h"
#include "ata.h"
#include "vga.h"
#include "string.h"

static struct fs_state fs;

// Helper: Read a sector
static bool read_sector(uint32_t lba, void *buf) {
    return ata_read_sectors(0, lba, 1, buf);
}

// Helper: Write a sector
static bool write_sector(uint32_t lba, const void *buf) {
    return ata_write_sectors(0, lba, 1, buf);
}

// Helper: Read an inode
static bool read_inode(uint32_t inode_num, struct inode *inode) {
    if (inode_num >= FS_MAX_INODES) return FALSE;

    uint32_t sector = FS_INODE_START_SECTOR + (inode_num / 8);
    uint32_t offset = (inode_num % 8) * sizeof(struct inode);

    if (!read_sector(sector, fs.sector_buf)) return FALSE;

    mem_cpy(inode, fs.sector_buf + offset, sizeof(struct inode));
    return TRUE;
}

// Helper: Write an inode
static bool write_inode(uint32_t inode_num, const struct inode *inode) {
    if (inode_num >= FS_MAX_INODES) return FALSE;

    uint32_t sector = FS_INODE_START_SECTOR + (inode_num / 8);
    uint32_t offset = (inode_num % 8) * sizeof(struct inode);

    if (!read_sector(sector, fs.sector_buf)) return FALSE;

    mem_cpy(fs.sector_buf + offset, inode, sizeof(struct inode));
    return write_sector(sector, fs.sector_buf);
}

// Helper: Allocate a free inode
static int32_t alloc_inode(void) {
    struct inode inode;
    for (uint32_t i = 0; i < FS_MAX_INODES; i++) {
        if (read_inode(i, &inode) && inode.type == INODE_TYPE_FREE) {
            return i;
        }
    }
    return -1;  // No free inodes
}

// Helper: Allocate a data block
static uint32_t alloc_block(void) {
    uint32_t block = fs.sb.next_free_block;
    fs.sb.next_free_block++;

    // Write updated superblock
    mem_cpy(fs.sector_buf, &fs.sb, sizeof(struct superblock));
    write_sector(FS_SUPERBLOCK_SECTOR, fs.sector_buf);

    return block;
}

// Helper: Find directory entry by name in current directory
static bool find_entry(const char *name, struct dir_entry *entry, uint32_t *entry_sector, uint32_t *entry_offset) {
    for (uint32_t s = 0; s < FS_DIRENTRY_SECTORS; s++) {
        uint32_t sector = FS_DIRENTRY_START + s;
        if (!read_sector(sector, fs.sector_buf)) continue;

        for (uint32_t i = 0; i < 8; i++) {
            struct dir_entry *e = (struct dir_entry*)(fs.sector_buf + i * sizeof(struct dir_entry));
            if (e->inode != 0 && e->parent_inode == fs.cwd_inode) {
                if (str_cmp(e->name, name) == 0) {
                    if (entry) mem_cpy(entry, e, sizeof(struct dir_entry));
                    if (entry_sector) *entry_sector = sector;
                    if (entry_offset) *entry_offset = i * sizeof(struct dir_entry);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// Helper: Find free directory entry slot
static bool find_free_entry(uint32_t *entry_sector, uint32_t *entry_offset) {
    for (uint32_t s = 0; s < FS_DIRENTRY_SECTORS; s++) {
        uint32_t sector = FS_DIRENTRY_START + s;
        if (!read_sector(sector, fs.sector_buf)) continue;

        for (uint32_t i = 0; i < 8; i++) {
            struct dir_entry *e = (struct dir_entry*)(fs.sector_buf + i * sizeof(struct dir_entry));
            if (e->inode == 0) {
                *entry_sector = sector;
                *entry_offset = i * sizeof(struct dir_entry);
                return TRUE;
            }
        }
    }
    return FALSE;
}

void fs_init(void) {
    fs.mounted = FALSE;
    fs.cwd_inode = 0;
    str_cpy(fs.cwd_path, "/");

    // Try to mount existing filesystem
    if (!fs_mount()) {
        vga_puts("[*] No filesystem found. Use 'format' to create one.\n");
    }
}

bool fs_format(void) {
    // Initialize superblock
    mem_set(&fs.sb, 0, sizeof(struct superblock));
    fs.sb.magic = FS_MAGIC;
    fs.sb.version = FS_VERSION;
    fs.sb.total_sectors = 20480;  // 10MB
    fs.sb.inode_count = FS_MAX_INODES;
    fs.sb.inode_start = FS_INODE_START_SECTOR;
    fs.sb.direntry_start = FS_DIRENTRY_START;
    fs.sb.data_start = FS_DATA_START_SECTOR;
    fs.sb.free_inodes = FS_MAX_INODES - 1;  // Root uses 1
    fs.sb.root_inode = 0;
    fs.sb.next_free_block = FS_DATA_START_SECTOR;

    // Write superblock
    mem_set(fs.sector_buf, 0, FS_SECTOR_SIZE);
    mem_cpy(fs.sector_buf, &fs.sb, sizeof(struct superblock));
    if (!write_sector(FS_SUPERBLOCK_SECTOR, fs.sector_buf)) {
        return FALSE;
    }

    // Clear inode table
    mem_set(fs.sector_buf, 0, FS_SECTOR_SIZE);
    for (uint32_t i = 0; i < FS_INODE_SECTORS; i++) {
        if (!write_sector(FS_INODE_START_SECTOR + i, fs.sector_buf)) {
            return FALSE;
        }
    }

    // Create root directory inode
    struct inode root_inode;
    mem_set(&root_inode, 0, sizeof(struct inode));
    root_inode.type = INODE_TYPE_DIR;
    root_inode.size = 0;
    root_inode.parent_inode = 0;  // Root is its own parent
    if (!write_inode(0, &root_inode)) {
        return FALSE;
    }

    // Clear directory entries
    mem_set(fs.sector_buf, 0, FS_SECTOR_SIZE);
    for (uint32_t i = 0; i < FS_DIRENTRY_SECTORS; i++) {
        if (!write_sector(FS_DIRENTRY_START + i, fs.sector_buf)) {
            return FALSE;
        }
    }

    // Mount the new filesystem
    fs.mounted = TRUE;
    fs.cwd_inode = 0;
    str_cpy(fs.cwd_path, "/");

    return TRUE;
}

bool fs_mount(void) {
    // Read superblock
    if (!read_sector(FS_SUPERBLOCK_SECTOR, fs.sector_buf)) {
        return FALSE;
    }

    mem_cpy(&fs.sb, fs.sector_buf, sizeof(struct superblock));

    // Verify magic number
    if (fs.sb.magic != FS_MAGIC) {
        return FALSE;
    }

    fs.mounted = TRUE;
    fs.cwd_inode = fs.sb.root_inode;
    str_cpy(fs.cwd_path, "/");

    return TRUE;
}

bool fs_is_mounted(void) {
    return fs.mounted;
}

const char* fs_get_cwd(void) {
    return fs.cwd_path;
}

uint32_t fs_get_cwd_inode(void) {
    return fs.cwd_inode;
}

bool fs_mkdir(const char *name) {
    if (!fs.mounted) return FALSE;
    if (str_len(name) >= FS_MAX_FILENAME) return FALSE;

    // Check if already exists
    if (find_entry(name, NULL, NULL, NULL)) {
        return FALSE;
    }

    // Allocate inode
    int32_t new_inode = alloc_inode();
    if (new_inode < 0) return FALSE;

    // Create inode
    struct inode inode;
    mem_set(&inode, 0, sizeof(struct inode));
    inode.type = INODE_TYPE_DIR;
    inode.size = 0;
    inode.parent_inode = fs.cwd_inode;
    if (!write_inode(new_inode, &inode)) {
        return FALSE;
    }

    // Find free directory entry
    uint32_t entry_sector, entry_offset;
    if (!find_free_entry(&entry_sector, &entry_offset)) {
        return FALSE;
    }

    // Create directory entry
    if (!read_sector(entry_sector, fs.sector_buf)) {
        return FALSE;
    }

    struct dir_entry *entry = (struct dir_entry*)(fs.sector_buf + entry_offset);
    mem_set(entry, 0, sizeof(struct dir_entry));
    entry->inode = new_inode;
    entry->parent_inode = fs.cwd_inode;
    entry->type = INODE_TYPE_DIR;
    entry->name_len = str_len(name);
    str_ncpy(entry->name, name, FS_MAX_FILENAME - 1);

    return write_sector(entry_sector, fs.sector_buf);
}

bool fs_chdir(const char *name) {
    if (!fs.mounted) return FALSE;

    // Handle root
    if (str_cmp(name, "/") == 0) {
        fs.cwd_inode = fs.sb.root_inode;
        str_cpy(fs.cwd_path, "/");
        return TRUE;
    }

    // Handle parent directory
    if (str_cmp(name, "..") == 0) {
        if (fs.cwd_inode == fs.sb.root_inode) {
            return TRUE;  // Already at root
        }

        struct inode inode;
        if (!read_inode(fs.cwd_inode, &inode)) {
            return FALSE;
        }

        fs.cwd_inode = inode.parent_inode;

        // Update path
        if (fs.cwd_inode == fs.sb.root_inode) {
            str_cpy(fs.cwd_path, "/");
        } else {
            // Remove last component from path
            char *last_slash = str_rchr(fs.cwd_path, '/');
            if (last_slash && last_slash != fs.cwd_path) {
                *last_slash = '\0';
            } else {
                str_cpy(fs.cwd_path, "/");
            }
        }
        return TRUE;
    }

    // Find directory
    struct dir_entry entry;
    if (!find_entry(name, &entry, NULL, NULL)) {
        return FALSE;
    }

    if (entry.type != INODE_TYPE_DIR) {
        return FALSE;  // Not a directory
    }

    fs.cwd_inode = entry.inode;

    // Update path
    if (str_cmp(fs.cwd_path, "/") != 0) {
        str_cat(fs.cwd_path, "/");
    }
    str_cat(fs.cwd_path, name);

    return TRUE;
}

bool fs_list_dir(void) {
    if (!fs.mounted) return FALSE;

    bool found_any = FALSE;

    for (uint32_t s = 0; s < FS_DIRENTRY_SECTORS; s++) {
        uint32_t sector = FS_DIRENTRY_START + s;
        if (!read_sector(sector, fs.sector_buf)) continue;

        for (uint32_t i = 0; i < 8; i++) {
            struct dir_entry *e = (struct dir_entry*)(fs.sector_buf + i * sizeof(struct dir_entry));
            if (e->inode != 0 && e->parent_inode == fs.cwd_inode) {
                found_any = TRUE;

                if (e->type == INODE_TYPE_DIR) {
                    vga_set_color(VGA_LIGHT_BLUE, VGA_BLACK);
                    vga_puts("  [DIR]  ");
                } else {
                    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
                    vga_puts("  [FILE] ");
                }

                vga_puts(e->name);

                // Show file size for files
                if (e->type == INODE_TYPE_FILE) {
                    struct inode inode;
                    if (read_inode(e->inode, &inode)) {
                        vga_puts(" (");
                        vga_put_dec(inode.size);
                        vga_puts(" bytes)");
                    }
                }

                vga_putchar('\n');
            }
        }
    }

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    if (!found_any) {
        vga_puts("  (empty directory)\n");
    }

    return TRUE;
}

bool fs_create(const char *name) {
    if (!fs.mounted) return FALSE;
    if (str_len(name) >= FS_MAX_FILENAME) return FALSE;

    // Check if already exists
    if (find_entry(name, NULL, NULL, NULL)) {
        return FALSE;
    }

    // Allocate inode
    int32_t new_inode = alloc_inode();
    if (new_inode < 0) return FALSE;

    // Create inode
    struct inode inode;
    mem_set(&inode, 0, sizeof(struct inode));
    inode.type = INODE_TYPE_FILE;
    inode.size = 0;
    inode.parent_inode = fs.cwd_inode;
    if (!write_inode(new_inode, &inode)) {
        return FALSE;
    }

    // Find free directory entry
    uint32_t entry_sector, entry_offset;
    if (!find_free_entry(&entry_sector, &entry_offset)) {
        return FALSE;
    }

    // Create directory entry
    if (!read_sector(entry_sector, fs.sector_buf)) {
        return FALSE;
    }

    struct dir_entry *entry = (struct dir_entry*)(fs.sector_buf + entry_offset);
    mem_set(entry, 0, sizeof(struct dir_entry));
    entry->inode = new_inode;
    entry->parent_inode = fs.cwd_inode;
    entry->type = INODE_TYPE_FILE;
    entry->name_len = str_len(name);
    str_ncpy(entry->name, name, FS_MAX_FILENAME - 1);

    return write_sector(entry_sector, fs.sector_buf);
}

bool fs_exists(const char *name) {
    return find_entry(name, NULL, NULL, NULL);
}

bool fs_open(const char *name, uint32_t *inode_num) {
    struct dir_entry entry;
    if (!find_entry(name, &entry, NULL, NULL)) {
        return FALSE;
    }
    *inode_num = entry.inode;
    return TRUE;
}

bool fs_read(uint32_t inode_num, void *buf, uint32_t *size) {
    struct inode inode;
    if (!read_inode(inode_num, &inode)) {
        return FALSE;
    }

    if (inode.type != INODE_TYPE_FILE) {
        return FALSE;
    }

    *size = inode.size;
    if (*size == 0) return TRUE;

    uint8_t *dst = buf;
    uint32_t remaining = inode.size;

    for (uint32_t i = 0; i < inode.block_count && remaining > 0; i++) {
        if (!read_sector(inode.blocks[i], fs.sector_buf)) {
            return FALSE;
        }

        uint32_t to_copy = remaining > FS_SECTOR_SIZE ? FS_SECTOR_SIZE : remaining;
        mem_cpy(dst, fs.sector_buf, to_copy);
        dst += to_copy;
        remaining -= to_copy;
    }

    return TRUE;
}

bool fs_write(uint32_t inode_num, const void *buf, uint32_t size) {
    struct inode inode;
    if (!read_inode(inode_num, &inode)) {
        return FALSE;
    }

    if (inode.type != INODE_TYPE_FILE) {
        return FALSE;
    }

    // Calculate blocks needed
    uint32_t blocks_needed = (size + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (blocks_needed > FS_DIRECT_BLOCKS) {
        return FALSE;  // File too large
    }

    // Allocate new blocks if needed
    while (inode.block_count < blocks_needed) {
        inode.blocks[inode.block_count] = alloc_block();
        inode.block_count++;
    }

    // Write data
    const uint8_t *src = buf;
    uint32_t remaining = size;

    for (uint32_t i = 0; i < blocks_needed; i++) {
        uint32_t to_write = remaining > FS_SECTOR_SIZE ? FS_SECTOR_SIZE : remaining;

        mem_set(fs.sector_buf, 0, FS_SECTOR_SIZE);
        mem_cpy(fs.sector_buf, src, to_write);

        if (!write_sector(inode.blocks[i], fs.sector_buf)) {
            return FALSE;
        }

        src += to_write;
        remaining -= to_write;
    }

    // Update inode
    inode.size = size;
    return write_inode(inode_num, &inode);
}

bool fs_delete(const char *name) {
    struct dir_entry entry;
    uint32_t entry_sector, entry_offset;

    if (!find_entry(name, &entry, &entry_sector, &entry_offset)) {
        return FALSE;
    }

    // For directories, check if empty
    if (entry.type == INODE_TYPE_DIR) {
        // Check for children
        for (uint32_t s = 0; s < FS_DIRENTRY_SECTORS; s++) {
            uint32_t sector = FS_DIRENTRY_START + s;
            if (!read_sector(sector, fs.sector_buf)) continue;

            for (uint32_t i = 0; i < 8; i++) {
                struct dir_entry *e = (struct dir_entry*)(fs.sector_buf + i * sizeof(struct dir_entry));
                if (e->inode != 0 && e->parent_inode == entry.inode) {
                    return FALSE;  // Directory not empty
                }
            }
        }
    }

    // Clear inode
    struct inode inode;
    mem_set(&inode, 0, sizeof(struct inode));
    write_inode(entry.inode, &inode);

    // Clear directory entry
    if (!read_sector(entry_sector, fs.sector_buf)) {
        return FALSE;
    }

    struct dir_entry *e = (struct dir_entry*)(fs.sector_buf + entry_offset);
    mem_set(e, 0, sizeof(struct dir_entry));

    return write_sector(entry_sector, fs.sector_buf);
}

bool fs_get_entry(const char *name, struct dir_entry *entry) {
    return find_entry(name, entry, NULL, NULL);
}

bool fs_get_inode(uint32_t inode_num, struct inode *inode) {
    return read_inode(inode_num, inode);
}
