#ifndef FS_H
#define FS_H

#include "types.h"

// Filesystem constants
#define FS_MAGIC            0x4D4F4E4B  // "MONK"
#define FS_VERSION          1
#define FS_MAX_INODES       64
#define FS_MAX_DIR_ENTRIES  256
#define FS_MAX_FILENAME     32
#define FS_MAX_PATH         256
#define FS_SECTOR_SIZE      512
#define FS_DIRECT_BLOCKS    10

// Sector layout
#define FS_SUPERBLOCK_SECTOR    0
#define FS_INODE_START_SECTOR   1
#define FS_INODE_SECTORS        8       // 64 inodes, 8 per sector
#define FS_DIRENTRY_START       9
#define FS_DIRENTRY_SECTORS     32      // 256 entries, 8 per sector
#define FS_DATA_START_SECTOR    41

// Inode types
#define INODE_TYPE_FREE     0
#define INODE_TYPE_FILE     1
#define INODE_TYPE_DIR      2

// Superblock structure (512 bytes)
struct superblock {
    uint32_t magic;
    uint32_t version;
    uint32_t total_sectors;
    uint32_t inode_count;
    uint32_t inode_start;
    uint32_t direntry_start;
    uint32_t data_start;
    uint32_t free_inodes;           // Count of free inodes
    uint32_t free_data_blocks;      // Count of free data blocks
    uint32_t root_inode;
    uint32_t next_free_block;       // Next data block to allocate
    uint8_t  reserved[468];
} __attribute__((packed));

// Inode structure (64 bytes)
struct inode {
    uint8_t  type;
    uint8_t  reserved;
    uint16_t permissions;
    uint32_t size;
    uint32_t blocks[FS_DIRECT_BLOCKS];
    uint32_t block_count;
    uint32_t parent_inode;
    uint32_t created;
    uint8_t  padding[4];
} __attribute__((packed));

// Directory entry (64 bytes)
struct dir_entry {
    uint32_t inode;
    uint32_t parent_inode;
    uint8_t  type;
    uint8_t  name_len;
    char     name[FS_MAX_FILENAME];
    uint8_t  padding[22];
} __attribute__((packed));

// Filesystem state
struct fs_state {
    struct superblock sb;
    uint32_t cwd_inode;
    char     cwd_path[FS_MAX_PATH];
    uint8_t  sector_buf[FS_SECTOR_SIZE];
    bool     mounted;
};

// Filesystem functions
void fs_init(void);
bool fs_format(void);
bool fs_mount(void);
bool fs_is_mounted(void);

// Path operations
const char* fs_get_cwd(void);
uint32_t fs_get_cwd_inode(void);

// Directory operations
bool fs_mkdir(const char *name);
bool fs_chdir(const char *name);
bool fs_list_dir(void);

// File operations
bool fs_create(const char *name);
bool fs_exists(const char *name);
bool fs_open(const char *name, uint32_t *inode);
bool fs_read(uint32_t inode, void *buf, uint32_t *size);
bool fs_write(uint32_t inode, const void *buf, uint32_t size);
bool fs_delete(const char *name);

// Get file/directory info
bool fs_get_entry(const char *name, struct dir_entry *entry);
bool fs_get_inode(uint32_t inode_num, struct inode *inode);

#endif
