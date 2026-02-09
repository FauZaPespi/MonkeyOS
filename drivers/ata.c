#include "ata.h"
#include "io.h"
#include "vga.h"

static struct ata_drive drives[2];  // Master and slave

// Wait for drive to be ready (not busy)
static void ata_wait_bsy(void) {
    while (inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY);
}

// Wait for drive to be ready and have data
static void ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_STATUS) & ATA_STATUS_DRQ));
}

// 400ns delay (read alternate status 4 times)
static void ata_delay(void) {
    inb(ATA_PRIMARY_ALTSTATUS);
    inb(ATA_PRIMARY_ALTSTATUS);
    inb(ATA_PRIMARY_ALTSTATUS);
    inb(ATA_PRIMARY_ALTSTATUS);
}

// Select a drive (0 = master, 1 = slave)
static void ata_select_drive(uint8_t drive) {
    outb(ATA_PRIMARY_DRIVE, drive == 0 ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE);
    ata_delay();  // Wait for drive selection
}

void ata_soft_reset(void) {
    outb(ATA_PRIMARY_CONTROL, 0x04);  // Set SRST bit
    ata_delay();
    outb(ATA_PRIMARY_CONTROL, 0x00);  // Clear SRST bit
    ata_delay();
    ata_wait_bsy();
}

static void fix_ata_string(char *str, int len) {
    // ATA strings are byte-swapped and space-padded
    for (int i = 0; i < len; i += 2) {
        char tmp = str[i];
        str[i] = str[i + 1];
        str[i + 1] = tmp;
    }
    // Trim trailing spaces
    for (int i = len - 1; i >= 0 && str[i] == ' '; i--) {
        str[i] = '\0';
    }
}

bool ata_identify(uint8_t drive, struct ata_drive *info) {
    uint16_t buffer[256];

    info->present = FALSE;
    info->drive_num = drive;

    // Select drive
    ata_select_drive(drive);

    // Set sector count and LBA to 0
    outb(ATA_PRIMARY_SECCOUNT, 0);
    outb(ATA_PRIMARY_LBA_LO, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HI, 0);

    // Send IDENTIFY command
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay();

    // Check if drive exists
    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0) {
        return FALSE;  // No drive
    }

    // Wait for BSY to clear
    ata_wait_bsy();

    // Check for ATAPI
    uint8_t lba_mid = inb(ATA_PRIMARY_LBA_MID);
    uint8_t lba_hi = inb(ATA_PRIMARY_LBA_HI);
    if (lba_mid != 0 || lba_hi != 0) {
        info->is_atapi = TRUE;
        return FALSE;  // Skip ATAPI for now
    }

    // Wait for DRQ or ERR
    while (1) {
        status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_STATUS_ERR) {
            return FALSE;  // Error
        }
        if (status & ATA_STATUS_DRQ) {
            break;  // Ready to transfer
        }
    }

    // Read identification data (256 words = 512 bytes)
    insw(ATA_PRIMARY_DATA, buffer, 256);

    // Parse the identification data
    info->present = TRUE;
    info->is_atapi = FALSE;

    // Get total sectors (words 60-61)
    info->size_sectors = buffer[60] | ((uint32_t)buffer[61] << 16);

    // Get model string (words 27-46)
    for (int i = 0; i < 40; i++) {
        info->model[i] = ((char*)&buffer[27])[i];
    }
    info->model[40] = '\0';
    fix_ata_string(info->model, 40);

    return TRUE;
}

bool ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, void *buffer) {
    if (count == 0) return FALSE;

    ata_wait_bsy();

    // Select drive and set high LBA bits
    outb(ATA_PRIMARY_DRIVE, (drive == 0 ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
    ata_delay();

    // Set sector count and LBA
    outb(ATA_PRIMARY_SECCOUNT, count);
    outb(ATA_PRIMARY_LBA_LO, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);

    // Send read command
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);

    uint16_t *buf = (uint16_t *)buffer;

    for (int s = 0; s < count; s++) {
        // Wait for data
        ata_wait_bsy();

        uint8_t status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_STATUS_ERR) {
            return FALSE;
        }

        ata_wait_drq();

        // Read 256 words (512 bytes)
        insw(ATA_PRIMARY_DATA, buf, 256);
        buf += 256;
    }

    return TRUE;
}

bool ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, const void *buffer) {
    if (count == 0) return FALSE;

    ata_wait_bsy();

    // Select drive and set high LBA bits
    outb(ATA_PRIMARY_DRIVE, (drive == 0 ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
    ata_delay();

    // Set sector count and LBA
    outb(ATA_PRIMARY_SECCOUNT, count);
    outb(ATA_PRIMARY_LBA_LO, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);

    // Send write command
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_PIO);

    const uint16_t *buf = (const uint16_t *)buffer;

    for (int s = 0; s < count; s++) {
        ata_wait_bsy();
        ata_wait_drq();

        // Write 256 words (512 bytes)
        outsw(ATA_PRIMARY_DATA, buf, 256);
        buf += 256;
    }

    // Flush cache
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_bsy();

    return TRUE;
}

struct ata_drive* ata_get_drive(uint8_t drive) {
    if (drive > 1) return NULL;
    return &drives[drive];
}

void ata_init(void) {
    vga_puts("ATA: Initializing...\n");

    // Soft reset
    ata_soft_reset();

    // Identify drives
    if (ata_identify(0, &drives[0])) {
        vga_puts("ATA: Master drive: ");
        vga_puts(drives[0].model);
        vga_puts(" (");
        vga_put_dec(drives[0].size_sectors / 2048);  // Convert to MB
        vga_puts(" MB)\n");
    } else {
        vga_puts("ATA: No master drive found\n");
    }

    if (ata_identify(1, &drives[1])) {
        vga_puts("ATA: Slave drive: ");
        vga_puts(drives[1].model);
        vga_puts("\n");
    }
}
