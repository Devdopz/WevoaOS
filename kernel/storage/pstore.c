#include "wevoa/pstore.h"

#include "wevoa/ata.h"
#include "wevoa/string.h"
#include "wevoa/wstatus.h"

#include <stdint.h>

#define PSTORE_MAGIC 0x31535041564f4557ull /* "WEVOAPS1" */
#define PSTORE_VERSION 1u

struct pstore_header_v1 {
    uint64_t magic;
    uint32_t version;
    uint32_t payload_sectors;
    uint32_t sequence;
    uint32_t payload_crc32;
    uint8_t reserved[512u - 24u];
} __attribute__((packed));

static uint32_t g_base_lba = 0u;
static uint32_t g_total_sectors = 0u;
static uint32_t g_sequence = 0u;
static int g_ready = 0;

static uint8_t g_payload_buf[WEVOA_PSTORE_PAYLOAD_MAX_BYTES];

static uint32_t crc32_calc(const uint8_t* data, uint32_t n) {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0u; i < n; ++i) {
        crc ^= (uint32_t)data[i];
        for (uint32_t b = 0u; b < 8u; ++b) {
            uint32_t mask = (uint32_t)(-(int32_t)(crc & 1u));
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static int pstore_read_header(struct pstore_header_v1* hdr) {
    if (!g_ready || hdr == 0) {
        return W_ERR_UNSUPPORTED;
    }
    return ata_read28(g_base_lba, 1u, hdr);
}

static int pstore_write_header(const struct pstore_header_v1* hdr) {
    if (!g_ready || hdr == 0) {
        return W_ERR_UNSUPPORTED;
    }
    return ata_write28(g_base_lba, 1u, hdr);
}

int pstore_init(uint8_t boot_drive) {
    int rc = ata_init(boot_drive);
    if (rc != W_OK) {
        g_ready = 0;
        return rc;
    }

    rc = ata_identify(&g_total_sectors);
    if (rc != W_OK) {
        g_ready = 0;
        return rc;
    }

    if (g_total_sectors <= WEVOA_PSTORE_REGION_SECTORS + WEVOA_PSTORE_PAYLOAD_SECTORS + 1u) {
        g_ready = 0;
        return W_ERR_NO_SPACE;
    }

    g_base_lba = g_total_sectors - WEVOA_PSTORE_REGION_SECTORS;
    g_sequence = 0u;
    g_ready = 1;
    return W_OK;
}

int pstore_ready(void) {
    return g_ready ? 1 : 0;
}

int pstore_load(void* payload, uint32_t payload_size, uint32_t* sequence_out) {
    if (!g_ready) {
        return W_ERR_UNSUPPORTED;
    }
    if (payload == 0 || payload_size == 0u || payload_size > WEVOA_PSTORE_PAYLOAD_MAX_BYTES) {
        return W_ERR_INVALID_ARG;
    }

    struct pstore_header_v1 hdr;
    int rc = pstore_read_header(&hdr);
    if (rc != W_OK) {
        return rc;
    }

    if (hdr.magic != PSTORE_MAGIC || hdr.version != PSTORE_VERSION) {
        return W_ERR_NOT_FOUND;
    }
    if (hdr.payload_sectors != WEVOA_PSTORE_PAYLOAD_SECTORS) {
        return W_ERR_CORRUPT;
    }

    kmemset(g_payload_buf, 0, sizeof(g_payload_buf));
    rc = ata_read28(g_base_lba + 1u, WEVOA_PSTORE_PAYLOAD_SECTORS, g_payload_buf);
    if (rc != W_OK) {
        return rc;
    }

    uint32_t crc = crc32_calc(g_payload_buf, WEVOA_PSTORE_PAYLOAD_MAX_BYTES);
    if (crc != hdr.payload_crc32) {
        return W_ERR_CORRUPT;
    }

    kmemcpy(payload, g_payload_buf, payload_size);
    g_sequence = hdr.sequence;
    if (sequence_out != 0) {
        *sequence_out = g_sequence;
    }
    return W_OK;
}

int pstore_save(const void* payload, uint32_t payload_size, uint32_t* sequence_out) {
    if (!g_ready) {
        return W_ERR_UNSUPPORTED;
    }
    if (payload == 0 || payload_size == 0u || payload_size > WEVOA_PSTORE_PAYLOAD_MAX_BYTES) {
        return W_ERR_INVALID_ARG;
    }

    kmemset(g_payload_buf, 0, sizeof(g_payload_buf));
    kmemcpy(g_payload_buf, payload, payload_size);

    struct pstore_header_v1 hdr;
    kmemset(&hdr, 0, sizeof(hdr));
    hdr.magic = PSTORE_MAGIC;
    hdr.version = PSTORE_VERSION;
    hdr.payload_sectors = WEVOA_PSTORE_PAYLOAD_SECTORS;
    hdr.sequence = g_sequence + 1u;
    hdr.payload_crc32 = crc32_calc(g_payload_buf, WEVOA_PSTORE_PAYLOAD_MAX_BYTES);

    int rc = ata_write28(g_base_lba + 1u, WEVOA_PSTORE_PAYLOAD_SECTORS, g_payload_buf);
    if (rc != W_OK) {
        return rc;
    }

    rc = pstore_write_header(&hdr);
    if (rc != W_OK) {
        return rc;
    }

    g_sequence = hdr.sequence;
    if (sequence_out != 0) {
        *sequence_out = g_sequence;
    }
    return W_OK;
}

int pstore_reset(void) {
    if (!g_ready) {
        return W_ERR_UNSUPPORTED;
    }

    struct pstore_header_v1 hdr;
    kmemset(&hdr, 0, sizeof(hdr));
    kmemset(g_payload_buf, 0, sizeof(g_payload_buf));

    int rc = ata_write28(g_base_lba + 1u, WEVOA_PSTORE_PAYLOAD_SECTORS, g_payload_buf);
    if (rc != W_OK) {
        return rc;
    }

    rc = pstore_write_header(&hdr);
    if (rc != W_OK) {
        return rc;
    }
    g_sequence = 0u;
    return W_OK;
}

