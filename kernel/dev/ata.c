#include "wevoa/ata.h"

#include "wevoa/ports.h"
#include "wevoa/wstatus.h"

#include <stdint.h>

#define ATA_IO_BASE 0x1F0u
#define ATA_CTRL_BASE 0x3F6u

#define ATA_REG_DATA         (ATA_IO_BASE + 0u)
#define ATA_REG_ERROR        (ATA_IO_BASE + 1u)
#define ATA_REG_FEATURES     (ATA_IO_BASE + 1u)
#define ATA_REG_SECCOUNT0    (ATA_IO_BASE + 2u)
#define ATA_REG_LBA0         (ATA_IO_BASE + 3u)
#define ATA_REG_LBA1         (ATA_IO_BASE + 4u)
#define ATA_REG_LBA2         (ATA_IO_BASE + 5u)
#define ATA_REG_HDDEVSEL     (ATA_IO_BASE + 6u)
#define ATA_REG_STATUS       (ATA_IO_BASE + 7u)
#define ATA_REG_COMMAND      (ATA_IO_BASE + 7u)

#define ATA_REG_ALTSTATUS    (ATA_CTRL_BASE + 0u)
#define ATA_REG_CONTROL      (ATA_CTRL_BASE + 0u)

#define ATA_SR_ERR 0x01u
#define ATA_SR_DRQ 0x08u
#define ATA_SR_DF  0x20u
#define ATA_SR_DRDY 0x40u
#define ATA_SR_BSY 0x80u

#define ATA_CMD_IDENTIFY 0xECu
#define ATA_CMD_READ_PIO 0x20u
#define ATA_CMD_WRITE_PIO 0x30u
#define ATA_CMD_CACHE_FLUSH 0xE7u

static uint8_t g_drive_head = 0xE0u;
static uint32_t g_total_sectors = 0u;
static int g_inited = 0;

static void ata_select_drive(uint32_t lba_high4) {
    outb(ATA_REG_HDDEVSEL, (uint8_t)(g_drive_head | (lba_high4 & 0x0Fu)));
    io_wait();
}

static int ata_wait_not_bsy(uint32_t max_loops) {
    for (uint32_t i = 0u; i < max_loops; ++i) {
        uint8_t st = inb(ATA_REG_STATUS);
        if ((st & ATA_SR_BSY) == 0u) {
            return W_OK;
        }
    }
    return W_ERR_IO;
}

static int ata_wait_drq(uint32_t max_loops) {
    for (uint32_t i = 0u; i < max_loops; ++i) {
        uint8_t st = inb(ATA_REG_STATUS);
        if ((st & ATA_SR_BSY) != 0u) {
            continue;
        }
        if ((st & (ATA_SR_ERR | ATA_SR_DF)) != 0u) {
            return W_ERR_IO;
        }
        if ((st & ATA_SR_DRQ) != 0u) {
            return W_OK;
        }
    }
    return W_ERR_IO;
}

int ata_init(uint8_t boot_drive) {
    uint8_t drive_bit = (uint8_t)(boot_drive & 0x01u);
    g_drive_head = (uint8_t)(drive_bit ? 0xF0u : 0xE0u);
    g_total_sectors = 0u;
    g_inited = 1;

    outb(ATA_REG_CONTROL, 0x02u);
    io_wait();
    return W_OK;
}

int ata_identify(uint32_t* total_sectors_out) {
    if (total_sectors_out == 0) {
        return W_ERR_INVALID_ARG;
    }
    if (!g_inited) {
        return W_ERR_UNSUPPORTED;
    }

    if (ata_wait_not_bsy(200000u) != W_OK) {
        return W_ERR_IO;
    }

    ata_select_drive(0u);
    outb(ATA_REG_SECCOUNT0, 0u);
    outb(ATA_REG_LBA0, 0u);
    outb(ATA_REG_LBA1, 0u);
    outb(ATA_REG_LBA2, 0u);
    outb(ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t st = inb(ATA_REG_STATUS);
    if (st == 0u) {
        return W_ERR_UNSUPPORTED;
    }

    if (ata_wait_drq(300000u) != W_OK) {
        return W_ERR_UNSUPPORTED;
    }

    uint16_t identify_words[256];
    for (uint32_t i = 0u; i < 256u; ++i) {
        identify_words[i] = inw(ATA_REG_DATA);
    }

    uint32_t sectors28 = ((uint32_t)identify_words[61] << 16) | (uint32_t)identify_words[60];
    if (sectors28 == 0u) {
        return W_ERR_UNSUPPORTED;
    }

    g_total_sectors = sectors28;
    *total_sectors_out = g_total_sectors;
    return W_OK;
}

int ata_read28(uint32_t lba, uint8_t count, void* buf) {
    if (!g_inited || buf == 0 || count == 0u) {
        return W_ERR_INVALID_ARG;
    }
    if (lba >= 0x10000000u) {
        return W_ERR_INVALID_ARG;
    }

    uint16_t* out_words = (uint16_t*)buf;
    for (uint8_t s = 0u; s < count; ++s) {
        uint32_t cur_lba = lba + (uint32_t)s;
        if (ata_wait_not_bsy(200000u) != W_OK) {
            return W_ERR_IO;
        }

        ata_select_drive(cur_lba >> 24);
        outb(ATA_REG_FEATURES, 0u);
        outb(ATA_REG_SECCOUNT0, 1u);
        outb(ATA_REG_LBA0, (uint8_t)(cur_lba & 0xFFu));
        outb(ATA_REG_LBA1, (uint8_t)((cur_lba >> 8) & 0xFFu));
        outb(ATA_REG_LBA2, (uint8_t)((cur_lba >> 16) & 0xFFu));
        outb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);

        if (ata_wait_drq(300000u) != W_OK) {
            return W_ERR_IO;
        }

        for (uint32_t i = 0u; i < 256u; ++i) {
            out_words[(uint32_t)s * 256u + i] = inw(ATA_REG_DATA);
        }
    }

    return W_OK;
}

int ata_write28(uint32_t lba, uint8_t count, const void* buf) {
    if (!g_inited || buf == 0 || count == 0u) {
        return W_ERR_INVALID_ARG;
    }
    if (lba >= 0x10000000u) {
        return W_ERR_INVALID_ARG;
    }

    const uint16_t* in_words = (const uint16_t*)buf;
    for (uint8_t s = 0u; s < count; ++s) {
        uint32_t cur_lba = lba + (uint32_t)s;
        if (ata_wait_not_bsy(200000u) != W_OK) {
            return W_ERR_IO;
        }

        ata_select_drive(cur_lba >> 24);
        outb(ATA_REG_FEATURES, 0u);
        outb(ATA_REG_SECCOUNT0, 1u);
        outb(ATA_REG_LBA0, (uint8_t)(cur_lba & 0xFFu));
        outb(ATA_REG_LBA1, (uint8_t)((cur_lba >> 8) & 0xFFu));
        outb(ATA_REG_LBA2, (uint8_t)((cur_lba >> 16) & 0xFFu));
        outb(ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

        if (ata_wait_drq(300000u) != W_OK) {
            return W_ERR_IO;
        }

        for (uint32_t i = 0u; i < 256u; ++i) {
            outw(ATA_REG_DATA, in_words[(uint32_t)s * 256u + i]);
        }
    }

    if (ata_wait_not_bsy(200000u) != W_OK) {
        return W_ERR_IO;
    }
    ata_select_drive((lba + count - 1u) >> 24);
    outb(ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (ata_wait_not_bsy(300000u) != W_OK) {
        return W_ERR_IO;
    }

    return W_OK;
}

