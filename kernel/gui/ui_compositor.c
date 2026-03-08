#include "wevoa/ui_compositor.h"

static int32_t clamp_i32_local(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

void wevoa_ui_compositor_blur_region(uint8_t* buffer,
                                     uint32_t pitch,
                                     uint32_t width,
                                     uint32_t height,
                                     int32_t x,
                                     int32_t y,
                                     int32_t w,
                                     int32_t h,
                                     uint8_t passes,
                                     uint8_t* scratch) {
    if (buffer == 0 || scratch == 0 || passes == 0u || w <= 2 || h <= 2) {
        return;
    }

    int32_t x0 = clamp_i32_local(x, 0, (int32_t)width - 1);
    int32_t y0 = clamp_i32_local(y, 0, (int32_t)height - 1);
    int32_t x1 = clamp_i32_local(x + w, 0, (int32_t)width);
    int32_t y1 = clamp_i32_local(y + h, 0, (int32_t)height);
    if (x1 - x0 <= 2 || y1 - y0 <= 2) {
        return;
    }

    for (uint8_t p = 0u; p < passes; ++p) {
        for (int32_t py = y0; py < y1; ++py) {
            uint32_t row = (uint32_t)py * pitch;
            for (int32_t px = x0; px < x1; ++px) {
                uint32_t sum = 0u;
                uint32_t cnt = 0u;
                for (int32_t oy = -1; oy <= 1; ++oy) {
                    int32_t sy = clamp_i32_local(py + oy, y0, y1 - 1);
                    uint32_t srow = (uint32_t)sy * pitch;
                    for (int32_t ox = -1; ox <= 1; ++ox) {
                        int32_t sx = clamp_i32_local(px + ox, x0, x1 - 1);
                        sum += buffer[srow + (uint32_t)sx];
                        cnt++;
                    }
                }
                scratch[row + (uint32_t)px] = (uint8_t)(sum / cnt);
            }
        }

        for (int32_t py = y0; py < y1; ++py) {
            uint32_t row = (uint32_t)py * pitch;
            for (int32_t px = x0; px < x1; ++px) {
                buffer[row + (uint32_t)px] = scratch[row + (uint32_t)px];
            }
        }
    }
}

void wevoa_ui_compositor_record_frame(struct wevoa_ui_perf_ring* ring,
                                      uint64_t frame_cycles,
                                      struct wevoa_ui_perf_stats* out_stats) {
    if (ring == 0 || out_stats == 0) {
        return;
    }

    out_stats->last_cycles = frame_cycles;
    ring->samples[ring->index] = frame_cycles;
    ring->index = (uint8_t)((ring->index + 1u) % WEVOA_UI_PERF_SAMPLE_COUNT);
    if (ring->count < WEVOA_UI_PERF_SAMPLE_COUNT) {
        ring->count++;
    }

    uint64_t sorted[WEVOA_UI_PERF_SAMPLE_COUNT];
    uint64_t sum = 0ull;
    for (uint8_t i = 0u; i < ring->count; ++i) {
        sorted[i] = ring->samples[i];
        sum += sorted[i];
    }

    for (uint8_t i = 1u; i < ring->count; ++i) {
        uint64_t v = sorted[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && sorted[(uint32_t)j] > v) {
            sorted[(uint32_t)j + 1u] = sorted[(uint32_t)j];
            j--;
        }
        sorted[(uint32_t)j + 1u] = v;
    }

    if (ring->count == 0u) {
        out_stats->avg_cycles = 0ull;
        out_stats->p95_cycles = 0ull;
        out_stats->p99_cycles = 0ull;
        return;
    }

    out_stats->avg_cycles = sum / ring->count;
    uint32_t i95 = ((uint32_t)ring->count * 95u + 99u) / 100u;
    uint32_t i99 = ((uint32_t)ring->count * 99u + 99u) / 100u;
    if (i95 == 0u) {
        i95 = 1u;
    }
    if (i99 == 0u) {
        i99 = 1u;
    }
    out_stats->p95_cycles = sorted[i95 - 1u];
    out_stats->p99_cycles = sorted[i99 - 1u];
}
