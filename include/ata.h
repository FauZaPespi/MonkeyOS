#ifndef ATA_H
#define ATA_H

#include "types.h"

// ATA I/O ports (Primary controller)
#define ATA_PRIMARY_DATA        0x1F0   // Data register (R/W)
#define ATA_PRIMARY_ERROR       0x1F1   // Error register (R)
#define ATA_PRIMARY_FEATURES    0x1F1   // Features register (W)
#define ATA_PRIMARY_SECCOUNT    0x1F2   // Sector count
#define ATA_PRIMARY_LBA_LO      0x1F3   // LBA low byte
#define ATA_PRIMARY_LBA_MID     0x1F4   // LBA mid byte
#define ATA_PRIMARY_LBA_HI      0x1F5   // LBA high byte
#define ATA_PRIMARY_DRIVE       0x1F6   // Drive/head select
#define ATA_PRIMARY_STATUS      0x1F7   // Status register (R)
#define ATA_PRIMARY_COMMAND     0x1F7   // Command register (W)
#define ATA_PRIMARY_CONTROL     0x3F6   // Control register
#define ATA_PRIMARY_ALTSTATUS   0x3F6   // Alternate status (R)

// Status register bits
#define ATA_STATUS_ERR          0x01    // Error occurred
#define ATA_STATUS_IDX          0x02    // Index mark
#define ATA_STATUS_CORR         0x04    // Corrected data
#define ATA_STATUS_DRQ          0x08    // Data request ready
#define ATA_STATUS_SRV          0x10    // Overlapped service request
#define ATA_STATUS_DF           0x20    // Drive fault
#define ATA_STATUS_RDY          0x40    // Drive ready
#define ATA_STATUS_BSY          0x80    // Drive busy

// ATA commands
#define ATA_CMD_READ_PIO        0x20    // Read sectors (PIO)
#define ATA_CMD_READ_PIO_EXT    0x24    // Read sectors ext (LBA48)
#define ATA_CMD_WRITE_PIO       0x30    // Write sectors (PIO)
#define ATA_CMD_WRITE_PIO_EXT   0x34    // Write sectors ext (LBA48)
#define ATA_CMD_CACHE_FLUSH     0xE7    // Flush write cache
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA    // Flush write cache ext
#define ATA_CMD_IDENTIFY        0xEC    // Identify drive

// Drive selection
#define ATA_DRIVE_MASTER        0xE0    // Master drive (with LBA)
#define ATA_DRIVE_SLAVE         0xF0    // Slave drive (with LBA)

// Sector size
#define ATA_SECTOR_SIZE         512

// Drive info
struct ata_drive {
    bool     present;           // Drive detected
    bool     is_atapi;          // ATAPI (CD-ROM) vs ATA
    uint8_t  drive_num;         // 0 = master, 1 = slave
    uint32_t size_sectors;      // Size in sectors
    char     model[41];         // Model string (null-terminated)
};

// Function prototypes
void ata_init(void);
bool ata_identify(uint8_t drive, struct ata_drive *info);
bool ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, void *buffer);
bool ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, const void *buffer);
void ata_soft_reset(void);

// Get drive info
struct ata_drive* ata_get_drive(uint8_t drive);

#endif
