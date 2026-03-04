#ifndef WEVOA_ATA_H
#define WEVOA_ATA_H

#include <stdint.h>

int ata_init(uint8_t boot_drive);
int ata_identify(uint32_t* total_sectors_out);
int ata_read28(uint32_t lba, uint8_t count, void* buf);
int ata_write28(uint32_t lba, uint8_t count, const void* buf);

#endif

