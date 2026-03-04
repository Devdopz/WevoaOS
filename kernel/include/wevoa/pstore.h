#ifndef WEVOA_PSTORE_H
#define WEVOA_PSTORE_H

#include <stdint.h>

#define WEVOA_PSTORE_REGION_SECTORS 128u
#define WEVOA_PSTORE_PAYLOAD_SECTORS 16u
#define WEVOA_PSTORE_PAYLOAD_MAX_BYTES (WEVOA_PSTORE_PAYLOAD_SECTORS * 512u)

int pstore_init(uint8_t boot_drive);
int pstore_ready(void);
int pstore_load(void* payload, uint32_t payload_size, uint32_t* sequence_out);
int pstore_save(const void* payload, uint32_t payload_size, uint32_t* sequence_out);
int pstore_reset(void);

#endif

