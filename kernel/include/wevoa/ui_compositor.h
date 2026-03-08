#ifndef WEVOA_UI_COMPOSITOR_H
#define WEVOA_UI_COMPOSITOR_H

#include <stdbool.h>
#include <stdint.h>

#define WEVOA_UI_PERF_SAMPLE_COUNT 64u

struct wevoa_ui_perf_ring {
    uint64_t samples[WEVOA_UI_PERF_SAMPLE_COUNT];
    uint8_t index;
    uint8_t count;
};

struct wevoa_ui_perf_stats {
    uint64_t last_cycles;
    uint64_t avg_cycles;
    uint64_t p95_cycles;
    uint64_t p99_cycles;
};

void wevoa_ui_compositor_blur_region(uint8_t* buffer,
                                     uint32_t pitch,
                                     uint32_t width,
                                     uint32_t height,
                                     int32_t x,
                                     int32_t y,
                                     int32_t w,
                                     int32_t h,
                                     uint8_t passes,
                                     uint8_t* scratch);

void wevoa_ui_compositor_record_frame(struct wevoa_ui_perf_ring* ring,
                                      uint64_t frame_cycles,
                                      struct wevoa_ui_perf_stats* out_stats);

#endif
