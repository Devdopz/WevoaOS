#include "wevoa/gui.h"
#include "wevoa/config.h"
#include "wevoa/net.h"
#include "wevoa/ports.h"
#include "wevoa/pstore.h"
#include "wevoa/ui_compositor.h"
#include "wevoa/ui_icons.h"
#include "wevoa/ui_shell.h"
#include "wevoa/ui_style.h"
#include "wevoa/ui_tokens.h"
#include "wevoa/wstatus.h"

#include <stdbool.h>
#include <stdint.h>

#define KEY_MAX 128u
#define BACKBUFFER_MAX_W 2560u
#define BACKBUFFER_MAX_H 1600u
#define BACKBUFFER_SIZE (BACKBUFFER_MAX_W * BACKBUFFER_MAX_H)

#define SC_BACKSPACE 0x0Eu
#define SC_1       0x02u
#define SC_2       0x03u
#define SC_3       0x04u
#define SC_4       0x05u
#define SC_5       0x06u
#define SC_6       0x07u
#define SC_7       0x08u
#define SC_8       0x09u
#define SC_9       0x0Au
#define SC_0       0x0Bu
#define SC_MINUS   0x0Cu
#define SC_EQUAL   0x0Du
#define SC_TAB     0x0Fu
#define SC_Q       0x10u
#define SC_W       0x11u
#define SC_E       0x12u
#define SC_R       0x13u
#define SC_T       0x14u
#define SC_Y       0x15u
#define SC_U       0x16u
#define SC_A       0x1Eu
#define SC_S       0x1Fu
#define SC_D       0x20u
#define SC_F       0x21u
#define SC_G       0x22u
#define SC_H       0x23u
#define SC_I       0x17u
#define SC_J       0x24u
#define SC_K       0x25u
#define SC_L       0x26u
#define SC_SEMICOLON 0x27u
#define SC_APOSTROPHE 0x28u
#define SC_GRAVE   0x29u
#define SC_O       0x18u
#define SC_P       0x19u
#define SC_LBRACKET 0x1Au
#define SC_RBRACKET 0x1Bu
#define SC_BACKSLASH 0x2Bu
#define SC_Z       0x2Cu
#define SC_X       0x2Du
#define SC_C       0x2Eu
#define SC_V       0x2Fu
#define SC_B       0x30u
#define SC_N       0x31u
#define SC_M       0x32u
#define SC_COMMA   0x33u
#define SC_DOT     0x34u
#define SC_SLASH   0x35u
#define SC_ENTER   0x1Cu
#define SC_ESC     0x01u
#define SC_LSHIFT  0x2Au
#define SC_RSHIFT  0x36u
#define SC_SPACE   0x39u

#define SHELL_LAYOUT_DOCK 0u
#define SHELL_LAYOUT_TASKBAR 1u
#define UI_LEVEL_MAX 3u
#define FRAME_TARGET_CYCLES_60FPS 30000000ull

#define TASKBAR_H 18
#define TITLE_H 14
#define MOUSE_PACKET_CLAMP 64
#define MOUSE_SENS_FP_MIN 6
#define MOUSE_SENS_FP_MAX 40
#define CURSOR_FP_SHIFT 4
#define CURSOR_FP_ONE (1 << CURSOR_FP_SHIFT)
#define KEY_MOVE_CYCLES_NORMAL 7500000ull
#define KEY_MOVE_CYCLES_FAST 3200000ull
#define WALLPAPER_COUNT 3u
#define TERM_HISTORY_LINES 24u
#define TERM_LINE_MAX 80u
#define TERM_INPUT_MAX 56u
#define VFS_MAX_NODES 48u
#define VFS_NAME_MAX 16u
#define VFS_CONTENT_MAX 80u
#define VFS_TYPE_FILE 0u
#define VFS_TYPE_DIR 1u
#define VFS_INVALID_NODE 0xFFu
#define DESKTOP_ITEM_MAX 20u
#define DESKTOP_MENU_ITEM_COUNT 3u
#define PSTORE_PAYLOAD_MAGIC 0x50535631u /* "PSV1" */
#define PSTORE_PAYLOAD_VERSION 1u
#define PSTORE_AUTOSAVE_CYCLES 7000000000ull
#define CMOS_STATUS_A 0x0Au
#define CMOS_STATUS_B 0x0Bu
#define CMOS_HOUR    0x04u
#define CMOS_MINUTE  0x02u
#define CMOS_DAY     0x07u
#define CMOS_MONTH   0x08u

struct app_window {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    bool open;
    bool minimized;
    bool maximized;
    int32_t restore_x;
    int32_t restore_y;
    int32_t restore_w;
    int32_t restore_h;
};

struct vfs_node {
    bool used;
    uint8_t parent;
    uint8_t type;
    char name[VFS_NAME_MAX];
    char content[VFS_CONTENT_MAX];
};

struct desktop_item {
    bool used;
    bool is_dir;
    uint8_t node;
    char name[VFS_NAME_MAX];
};

struct terminal_state {
    char lines[TERM_HISTORY_LINES][TERM_LINE_MAX];
    uint8_t line_count;
    char input[TERM_INPUT_MAX];
    uint8_t input_len;
    uint8_t cwd_node;
};

struct notepad_state {
    uint8_t file_node;
    char file_name[VFS_NAME_MAX];
    char buffer[VFS_CONTENT_MAX];
    uint8_t len;
    bool dirty;
};

struct calc_state {
    char expr[24];
    uint8_t expr_len;
};

struct image_state {
    uint8_t sample_index;
};

struct video_state {
    bool playing;
    uint8_t frame_index;
    uint64_t frame_accum_cycles;
};

struct browser_state {
    char url[TERM_INPUT_MAX];
    uint8_t url_len;
    char page_title[TERM_LINE_MAX];
    char page_line1[TERM_LINE_MAX];
    char page_line2[TERM_LINE_MAX];
    char page_line3[TERM_LINE_MAX];
    char status_line[TERM_LINE_MAX];
    uint8_t last_ip[4];
    uint16_t last_port;
    bool input_focus;
};

struct gui_state {
    bool key_down[KEY_MAX];
    bool key_pressed[KEY_MAX];
    bool e0_prefix;
    bool mouse_left_down;
    bool mouse_left_pressed;
    bool mouse_right_down;
    bool mouse_right_pressed;
    bool dragging;
    int32_t drag_dx;
    int32_t drag_dy;
    int32_t cursor_x;
    int32_t cursor_y;
    int32_t cursor_fx;
    int32_t cursor_fy;
    int32_t active_app;
    bool settings_wifi_on;
    bool settings_wifi_connected;
    uint8_t settings_brightness;
    uint8_t wallpaper_index;
    bool settings_lock_enabled;
    uint16_t settings_lock_pin;
    uint8_t shell_layout;
    uint8_t ui_blur_level;
    uint8_t ui_motion_level;
    uint8_t ui_effects_quality;
    uint8_t mouse_packet[3];
    uint8_t mouse_index;
    uint64_t last_tsc;
    uint64_t key_move_accum;
    struct app_window windows[APP_COUNT];
};

struct pstore_vfs_node {
    uint8_t used;
    uint8_t parent;
    uint8_t type;
    char name[VFS_NAME_MAX];
    char content[VFS_CONTENT_MAX];
} __attribute__((packed));

struct pstore_payload_v1 {
    uint32_t magic;
    uint32_t version;
    struct wevoa_config cfg;
    uint8_t desktop_new_folder_id;
    uint8_t desktop_new_file_id;
    uint8_t desktop_root_node;
    uint8_t terminal_cwd_node;
    uint8_t ui_scale_override;
    uint8_t reserved_p0[3];
    struct pstore_vfs_node nodes[VFS_MAX_NODES];
};

static uint8_t* fb_base;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;
static uint32_t fb_bpp_mode;

static uint32_t color_lut32[COLOR_COUNT];

static uint8_t* draw_base;
static uint32_t draw_pitch;

static uint8_t desktop_buffer[BACKBUFFER_SIZE];
static uint8_t frame_buffer[BACKBUFFER_SIZE];

static struct gui_state state;
static struct vfs_node vfs_nodes[VFS_MAX_NODES];
static struct desktop_item desktop_items[DESKTOP_ITEM_MAX];
static struct terminal_state terminal_state;
static struct notepad_state notepad_state;
static struct calc_state calc_state;
static struct image_state image_state;
static struct video_state video_state;
static struct browser_state browser_state;
static struct pstore_payload_v1 pstore_payload_cache;

static int32_t desktop_icon_x[APP_COUNT] = {10, 10, 10};
static int32_t desktop_icon_y[APP_COUNT] = {18, 56, 94};
static int32_t taskbar_icon_x[APP_COUNT] = {0, 0, 0};
static int32_t taskbar_icon_y[APP_COUNT] = {0, 0, 0};
static int32_t taskbar_icon_size = 0;
static int32_t taskbar_tray_x = 0;
static int32_t taskbar_tray_y = 0;
static int32_t taskbar_tray_w = 0;
static int32_t taskbar_tray_h = 0;
static int32_t settings_btn_x = 0;
static int32_t settings_btn_y = 0;
static int32_t settings_btn_w = 0;
static int32_t settings_btn_h = 0;
static int32_t shell_search_x = 0;
static int32_t shell_search_y = 0;
static int32_t shell_search_w = 0;
static int32_t shell_search_h = 0;
static int32_t dock_panel_x = 0;
static int32_t dock_panel_y = 0;
static int32_t dock_panel_w = 0;
static int32_t dock_panel_h = 0;
static bool shell_launcher_open = false;
static int32_t shell_launcher_x = 0;
static int32_t shell_launcher_y = 0;
static int32_t shell_launcher_w = 0;
static int32_t shell_launcher_h = 0;
static int32_t shell_launcher_item_x[APP_COUNT] = {0};
static int32_t shell_launcher_item_y[APP_COUNT] = {0};
static int32_t shell_launcher_item_w[APP_COUNT] = {0};
static int32_t shell_launcher_item_h[APP_COUNT] = {0};
static int32_t set_wifi_btn_x = 0;
static int32_t set_wifi_btn_y = 0;
static int32_t set_wifi_btn_w = 0;
static int32_t set_wifi_btn_h = 0;
static int32_t set_link_btn_x = 0;
static int32_t set_link_btn_y = 0;
static int32_t set_link_btn_w = 0;
static int32_t set_link_btn_h = 0;
static int32_t set_bminus_btn_x = 0;
static int32_t set_bminus_btn_y = 0;
static int32_t set_bminus_btn_w = 0;
static int32_t set_bminus_btn_h = 0;
static int32_t set_bplus_btn_x = 0;
static int32_t set_bplus_btn_y = 0;
static int32_t set_bplus_btn_w = 0;
static int32_t set_bplus_btn_h = 0;
static int32_t set_wp_prev_btn_x = 0;
static int32_t set_wp_prev_btn_y = 0;
static int32_t set_wp_prev_btn_w = 0;
static int32_t set_wp_prev_btn_h = 0;
static int32_t set_wp_next_btn_x = 0;
static int32_t set_wp_next_btn_y = 0;
static int32_t set_wp_next_btn_w = 0;
static int32_t set_wp_next_btn_h = 0;
static int32_t set_scale_minus_btn_x = 0;
static int32_t set_scale_minus_btn_y = 0;
static int32_t set_scale_minus_btn_w = 0;
static int32_t set_scale_minus_btn_h = 0;
static int32_t set_scale_plus_btn_x = 0;
static int32_t set_scale_plus_btn_y = 0;
static int32_t set_scale_plus_btn_w = 0;
static int32_t set_scale_plus_btn_h = 0;
static int32_t set_scale_auto_btn_x = 0;
static int32_t set_scale_auto_btn_y = 0;
static int32_t set_scale_auto_btn_w = 0;
static int32_t set_scale_auto_btn_h = 0;
static int32_t note_btn_load_x = 0;
static int32_t note_btn_load_y = 0;
static int32_t note_btn_load_w = 0;
static int32_t note_btn_load_h = 0;
static int32_t note_btn_save_x = 0;
static int32_t note_btn_save_y = 0;
static int32_t note_btn_save_w = 0;
static int32_t note_btn_save_h = 0;
static int32_t note_btn_new_x = 0;
static int32_t note_btn_new_y = 0;
static int32_t note_btn_new_w = 0;
static int32_t note_btn_new_h = 0;
static int32_t calc_btn_x[16] = {0};
static int32_t calc_btn_y[16] = {0};
static int32_t calc_btn_w[16] = {0};
static int32_t calc_btn_h[16] = {0};
static int32_t video_btn_play_x = 0;
static int32_t video_btn_play_y = 0;
static int32_t video_btn_play_w = 0;
static int32_t video_btn_play_h = 0;
static int32_t video_btn_pause_x = 0;
static int32_t video_btn_pause_y = 0;
static int32_t video_btn_pause_w = 0;
static int32_t video_btn_pause_h = 0;
static int32_t video_btn_stop_x = 0;
static int32_t video_btn_stop_y = 0;
static int32_t video_btn_stop_w = 0;
static int32_t video_btn_stop_h = 0;
static int32_t browser_url_x = 0;
static int32_t browser_url_y = 0;
static int32_t browser_url_w = 0;
static int32_t browser_url_h = 0;
static int32_t browser_go_btn_x = 0;
static int32_t browser_go_btn_y = 0;
static int32_t browser_go_btn_w = 0;
static int32_t browser_go_btn_h = 0;
static int32_t browser_home_btn_x = 0;
static int32_t browser_home_btn_y = 0;
static int32_t browser_home_btn_w = 0;
static int32_t browser_home_btn_h = 0;
static int32_t browser_refresh_btn_x = 0;
static int32_t browser_refresh_btn_y = 0;
static int32_t browser_refresh_btn_w = 0;
static int32_t browser_refresh_btn_h = 0;
static int32_t set_lock_btn_x = 0;
static int32_t set_lock_btn_y = 0;
static int32_t set_lock_btn_w = 0;
static int32_t set_lock_btn_h = 0;
static int32_t set_pin_minus_btn_x = 0;
static int32_t set_pin_minus_btn_y = 0;
static int32_t set_pin_minus_btn_w = 0;
static int32_t set_pin_minus_btn_h = 0;
static int32_t set_pin_plus_btn_x = 0;
static int32_t set_pin_plus_btn_y = 0;
static int32_t set_pin_plus_btn_w = 0;
static int32_t set_pin_plus_btn_h = 0;
static int32_t set_lock_now_btn_x = 0;
static int32_t set_lock_now_btn_y = 0;
static int32_t set_lock_now_btn_w = 0;
static int32_t set_lock_now_btn_h = 0;
static int32_t set_net_up_btn_x = 0;
static int32_t set_net_up_btn_y = 0;
static int32_t set_net_up_btn_w = 0;
static int32_t set_net_up_btn_h = 0;
static int32_t set_net_down_btn_x = 0;
static int32_t set_net_down_btn_y = 0;
static int32_t set_net_down_btn_w = 0;
static int32_t set_net_down_btn_h = 0;
static int32_t set_dns_dev_btn_x = 0;
static int32_t set_dns_dev_btn_y = 0;
static int32_t set_dns_dev_btn_w = 0;
static int32_t set_dns_dev_btn_h = 0;
static int32_t set_dns_openai_btn_x = 0;
static int32_t set_dns_openai_btn_y = 0;
static int32_t set_dns_openai_btn_w = 0;
static int32_t set_dns_openai_btn_h = 0;
static int32_t set_fw80_btn_x = 0;
static int32_t set_fw80_btn_y = 0;
static int32_t set_fw80_btn_w = 0;
static int32_t set_fw80_btn_h = 0;
static int32_t set_fw443_btn_x = 0;
static int32_t set_fw443_btn_y = 0;
static int32_t set_fw443_btn_w = 0;
static int32_t set_fw443_btn_h = 0;
static int32_t set_shell_layout_btn_x = 0;
static int32_t set_shell_layout_btn_y = 0;
static int32_t set_shell_layout_btn_w = 0;
static int32_t set_shell_layout_btn_h = 0;
static int32_t set_blur_minus_btn_x = 0;
static int32_t set_blur_minus_btn_y = 0;
static int32_t set_blur_minus_btn_w = 0;
static int32_t set_blur_minus_btn_h = 0;
static int32_t set_blur_plus_btn_x = 0;
static int32_t set_blur_plus_btn_y = 0;
static int32_t set_blur_plus_btn_w = 0;
static int32_t set_blur_plus_btn_h = 0;
static int32_t set_motion_minus_btn_x = 0;
static int32_t set_motion_minus_btn_y = 0;
static int32_t set_motion_minus_btn_w = 0;
static int32_t set_motion_minus_btn_h = 0;
static int32_t set_motion_plus_btn_x = 0;
static int32_t set_motion_plus_btn_y = 0;
static int32_t set_motion_plus_btn_w = 0;
static int32_t set_motion_plus_btn_h = 0;
static int32_t set_quality_btn_x = 0;
static int32_t set_quality_btn_y = 0;
static int32_t set_quality_btn_w = 0;
static int32_t set_quality_btn_h = 0;
static int32_t win_btn_close_x[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_close_y[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_close_w[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_close_h[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_max_x[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_max_y[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_max_w[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_max_h[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_min_x[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_min_y[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_min_w[APP_COUNT] = {0, 0, 0};
static int32_t win_btn_min_h[APP_COUNT] = {0, 0, 0};
static int32_t desktop_item_x[DESKTOP_ITEM_MAX] = {0};
static int32_t desktop_item_y[DESKTOP_ITEM_MAX] = {0};
static uint8_t desktop_item_count = 0u;
static uint8_t desktop_root_node = VFS_INVALID_NODE;
static uint8_t waste_bin_node = VFS_INVALID_NODE;
static uint8_t desktop_new_folder_id = 1u;
static uint8_t desktop_new_file_id = 1u;
static bool desktop_menu_open = false;
static int32_t desktop_menu_x = 0;
static int32_t desktop_menu_y = 0;
static int32_t desktop_menu_w = 118;
static int32_t desktop_menu_h = 0;
static int32_t desktop_menu_item_h = 16;
static uint8_t theme_rgb6[COLOR_COUNT][3];
static bool pstore_available = false;
static bool pstore_dirty = false;
static uint64_t pstore_dirty_since_tsc = 0ull;
static uint32_t pstore_sequence = 0u;
static uint8_t ui_scale_override = 0u; /* 0=auto, 1..3 fixed */
static uint32_t browser_nav_token = 0u;
static bool lock_screen_active = false;
static bool lock_screen_bad_pin = false;
static char lock_screen_pin_input[8] = {0};
static uint8_t lock_screen_pin_len = 0u;
static char settings_net_status_line[TERM_LINE_MAX] = "NET READY";
static uint8_t ui_blur_runtime = 0u;
static struct wevoa_ui_perf_ring perf_ring = {0};
static struct wevoa_ui_perf_stats perf_stats = {0};
static struct {
    bool has_data;
    bool cut_mode;
    uint8_t node;
    char name[VFS_NAME_MAX];
} vfs_clipboard;

static int32_t desktop_scale(void);
static void u8_to_dec_text(uint8_t value, char out[4]);
static void u8_to_2digit_text(uint8_t value, char out[3]);
static void u32_to_dec_text(uint32_t value, char* out, uint32_t cap);
static void ipv4_to_text(const uint8_t ip[4], char out[20]);
static uint8_t cmos_read_reg(uint8_t reg);
static bool cmos_read_hour_minute(uint8_t* hour, uint8_t* minute);
static bool cmos_read_day_month(uint8_t* day, uint8_t* month);
static void build_desktop_template(void);
static uint8_t vfs_add_node(uint8_t parent, uint8_t type, const char* name, const char* content);
static uint8_t vfs_find_child(uint8_t parent, const char* name);
static void sync_desktop_items_from_vfs(void);
static void terminal_init(void);
static uint8_t vfs_ensure_waste_bin(void);
static bool vfs_is_descendant(uint8_t node, uint8_t ancestor);
static void vfs_reset_node(uint8_t node);
static void vfs_clear_subtree(uint8_t node);
static uint8_t vfs_clone_subtree(uint8_t src_node, uint8_t dst_parent, const char* override_name);
static void vfs_build_unique_copy_name(const char* base_name, char out[VFS_NAME_MAX]);
static int vfs_move_to_waste_bin(uint8_t node);
static int vfs_restore_from_waste_bin(uint8_t node, const char* override_name);
static void notepad_bind_default_file(void);
static void notepad_load_from_node(uint8_t node);
static void notepad_save_to_file(void);
static bool notepad_handle_key_input(void);
static bool handle_notepad_click(void);
static bool calc_handle_key_input(void);
static bool handle_calculator_click(void);
static bool handle_video_click(void);
static bool browser_handle_key_input(void);
static bool handle_browser_click(void);
static void browser_navigate(const char* input_url);
static void draw_browser_preview_panel(int32_t x, int32_t y, int32_t w, int32_t h);
static void draw_lock_screen(void);
static void draw_shell_launcher(void);
static void lock_screen_activate(void);
static void lock_screen_clear_input(void);
static bool lock_screen_handle_input(void);
static bool handle_shell_overlay_click(void);
static int32_t calc_eval_expr(const char* expr, bool* ok);
static void calc_set_display_value(int32_t value);
static char scancode_to_char(uint8_t scancode, bool shift);
static void focus_app(uint32_t app);
static void mark_pstore_dirty(void);
static void net_sync_link_from_state(void);
static void config_sync_from_state(void);
static void state_sync_from_config(void);
static void reset_runtime_data_defaults(void);
static int pstore_save_runtime(void);
static int pstore_load_runtime(void);
static int config_key_from_name(const char* key_name, enum wevoa_config_key* out_key);
static bool shell_is_dock_mode(void);
static int32_t shell_top_h(void);
static int32_t shell_bottom_h(void);
static int32_t taskbar_h(void);
static void draw_glass_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t base, uint8_t hi, uint8_t lo);
static void draw_text5x7(int32_t x, int32_t y, const char* text, uint8_t color, int32_t scale);
static void blur_region(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t passes);
static uint8_t ui_target_blur_level(void);
static void perf_record_frame(uint64_t frame_cycles);
static void perf_draw_overlay(void);

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static int32_t abs_i32(int32_t v) {
    return (v < 0) ? -v : v;
}

static bool point_in_rect(int32_t px, int32_t py, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (w <= 0 || h <= 0) {
        return false;
    }
    return px >= x && py >= y && px < x + w && py < y + h;
}

static uint32_t text_len(const char* s) {
    uint32_t n = 0u;
    if (s == 0) {
        return 0u;
    }
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

static void text_copy(char* dst, uint32_t cap, const char* src) {
    if (dst == 0 || cap == 0u) {
        return;
    }
    uint32_t i = 0u;
    if (src != 0) {
        while (i + 1u < cap && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static void text_append(char* dst, uint32_t cap, const char* suffix) {
    if (dst == 0 || cap == 0u || suffix == 0) {
        return;
    }

    uint32_t i = text_len(dst);
    uint32_t j = 0u;
    while (i + 1u < cap && suffix[j] != '\0') {
        dst[i++] = suffix[j++];
    }
    dst[i] = '\0';
}

static void text_copy_truncated(char* dst, uint32_t cap, const char* src, uint32_t max_chars) {
    if (dst == 0 || cap == 0u) {
        return;
    }
    if (src == 0) {
        dst[0] = '\0';
        return;
    }

    uint32_t limit = cap - 1u;
    if (max_chars < limit) {
        limit = max_chars;
    }

    uint32_t i = 0u;
    while (i < limit && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    if (src[i] != '\0' && i >= 3u) {
        dst[i - 3u] = '.';
        dst[i - 2u] = '.';
        dst[i - 1u] = '.';
    }
    dst[i] = '\0';
}

static bool text_equal(const char* a, const char* b) {
    if (a == 0 || b == 0) {
        return false;
    }
    uint32_t i = 0u;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return false;
        }
        i++;
    }
    return a[i] == b[i];
}

static bool parse_u32(const char* s, uint32_t* out) {
    if (s == 0 || out == 0 || s[0] == '\0') {
        return false;
    }
    uint32_t v = 0u;
    for (uint32_t i = 0u; s[i] != '\0'; ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
        v = v * 10u + (uint32_t)(s[i] - '0');
        if (v > 1000000u) {
            return false;
        }
    }
    *out = v;
    return true;
}

static void text_upper_inplace(char* s) {
    if (s == 0) {
        return;
    }
    for (uint32_t i = 0u; s[i] != '\0'; ++i) {
        if (s[i] >= 'a' && s[i] <= 'z') {
            s[i] = (char)(s[i] - ('a' - 'A'));
        }
    }
}

static bool shell_is_dock_mode(void) {
    return wevoa_ui_shell_is_dock(state.shell_layout);
}

static int32_t shell_top_h(void) {
    return wevoa_ui_shell_top_h(state.shell_layout, desktop_scale(), taskbar_h());
}

static int32_t shell_bottom_h(void) {
    return wevoa_ui_shell_bottom_h(state.shell_layout, desktop_scale());
}

static void blur_region(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t passes) {
    uint8_t* scratch = (draw_base == frame_buffer) ? desktop_buffer : frame_buffer;
    wevoa_ui_compositor_blur_region(draw_base, draw_pitch, fb_width, fb_height, x, y, w, h, passes, scratch);
}

static uint8_t ui_target_blur_level(void) {
    uint8_t max_blur = (uint8_t)clamp_i32((int32_t)state.ui_blur_level, 0, UI_LEVEL_MAX);
    uint8_t quality = (uint8_t)clamp_i32((int32_t)state.ui_effects_quality, 0, UI_LEVEL_MAX);

    if (quality == 0u) {
        return 0u;
    }
    if (quality == 1u && max_blur > 1u) {
        max_blur = 1u;
    }
    if (quality == 2u && max_blur > 2u) {
        max_blur = 2u;
    }
    return max_blur;
}

static void perf_record_frame(uint64_t frame_cycles) {
    wevoa_ui_compositor_record_frame(&perf_ring, frame_cycles, &perf_stats);

    uint8_t target_blur = ui_target_blur_level();
    if (ui_blur_runtime > target_blur) {
        ui_blur_runtime = target_blur;
    }
    if (ui_blur_runtime == 0u && target_blur > 0u) {
        ui_blur_runtime = target_blur;
    }
    if (frame_cycles > (FRAME_TARGET_CYCLES_60FPS * 13ull) / 10ull && ui_blur_runtime > 0u) {
        ui_blur_runtime--;
    } else if (frame_cycles < (FRAME_TARGET_CYCLES_60FPS * 7ull) / 10ull && ui_blur_runtime < target_blur) {
        ui_blur_runtime++;
    }
}

static void perf_draw_overlay(void) {
    if (state.ui_effects_quality < 3u) {
        return;
    }

    int32_t x = 6;
    int32_t y = shell_top_h() + 6;
    int32_t w = 124;
    int32_t h = 28;
    draw_glass_rect(x, y, w, h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);

    char avg[12];
    char p95[12];
    char p99[12];
    u32_to_dec_text((uint32_t)(perf_stats.avg_cycles / 1000ull), avg, sizeof(avg));
    u32_to_dec_text((uint32_t)(perf_stats.p95_cycles / 1000ull), p95, sizeof(p95));
    u32_to_dec_text((uint32_t)(perf_stats.p99_cycles / 1000ull), p99, sizeof(p99));

    char line1[TERM_LINE_MAX];
    char line2[TERM_LINE_MAX];
    text_copy(line1, sizeof(line1), "AVG ");
    text_append(line1, sizeof(line1), avg);
    text_append(line1, sizeof(line1), "K  P95 ");
    text_append(line1, sizeof(line1), p95);
    text_append(line1, sizeof(line1), "K");
    text_copy(line2, sizeof(line2), "P99 ");
    text_append(line2, sizeof(line2), p99);
    text_append(line2, sizeof(line2), "K BLUR ");
    char blur_now[4];
    u8_to_dec_text(ui_blur_runtime, blur_now);
    text_append(line2, sizeof(line2), blur_now);

    draw_text5x7(x + 4, y + 4, line1, COLOR_BORDER, 1);
    draw_text5x7(x + 4, y + 14, line2, COLOR_TITLE_INACTIVE, 1);
}

static void mark_pstore_dirty(void) {
    pstore_dirty = true;
    pstore_dirty_since_tsc = rdtsc();
}

static void net_sync_link_from_state(void) {
    uint8_t up = (state.settings_wifi_on && state.settings_wifi_connected) ? 1u : 0u;
    (void)net_set_link(up);
}

static void config_sync_from_state(void) {
    (void)config_set(WEVOA_CFG_BRIGHT, state.settings_brightness);
    (void)config_set(WEVOA_CFG_WALL, state.wallpaper_index);
    (void)config_set(WEVOA_CFG_WIFI, state.settings_wifi_on ? 1u : 0u);
    (void)config_set(WEVOA_CFG_NET_LINK, state.settings_wifi_connected ? 1u : 0u);
    (void)config_set(WEVOA_CFG_LOCK_ENABLED, state.settings_lock_enabled ? 1u : 0u);
    (void)config_set(WEVOA_CFG_LOCK_PIN, state.settings_lock_pin);
    (void)config_set(WEVOA_CFG_SHELL_LAYOUT, state.shell_layout);
    (void)config_set(WEVOA_CFG_UI_BLUR_LEVEL, state.ui_blur_level);
    (void)config_set(WEVOA_CFG_UI_MOTION_LEVEL, state.ui_motion_level);
    (void)config_set(WEVOA_CFG_UI_EFFECTS_QUALITY, state.ui_effects_quality);
}

static void state_sync_from_config(void) {
    uint32_t v = 0u;
    if (config_get(WEVOA_CFG_BRIGHT, &v) == W_OK) {
        state.settings_brightness = (uint8_t)v;
    }
    if (config_get(WEVOA_CFG_WALL, &v) == W_OK) {
        state.wallpaper_index = (uint8_t)(v % WALLPAPER_COUNT);
    }
    if (config_get(WEVOA_CFG_WIFI, &v) == W_OK) {
        state.settings_wifi_on = (v != 0u);
    }
    if (config_get(WEVOA_CFG_NET_LINK, &v) == W_OK) {
        state.settings_wifi_connected = (v != 0u);
    }
    if (config_get(WEVOA_CFG_LOCK_ENABLED, &v) == W_OK) {
        state.settings_lock_enabled = (v != 0u);
    }
    if (config_get(WEVOA_CFG_LOCK_PIN, &v) == W_OK) {
        state.settings_lock_pin = (uint16_t)v;
    }
    if (config_get(WEVOA_CFG_SHELL_LAYOUT, &v) == W_OK) {
        state.shell_layout = (uint8_t)(v > SHELL_LAYOUT_TASKBAR ? SHELL_LAYOUT_DOCK : v);
    }
    if (config_get(WEVOA_CFG_UI_BLUR_LEVEL, &v) == W_OK) {
        state.ui_blur_level = (uint8_t)clamp_i32((int32_t)v, 0, UI_LEVEL_MAX);
    }
    if (config_get(WEVOA_CFG_UI_MOTION_LEVEL, &v) == W_OK) {
        state.ui_motion_level = (uint8_t)clamp_i32((int32_t)v, 0, UI_LEVEL_MAX);
    }
    if (config_get(WEVOA_CFG_UI_EFFECTS_QUALITY, &v) == W_OK) {
        state.ui_effects_quality = (uint8_t)clamp_i32((int32_t)v, 0, UI_LEVEL_MAX);
    }
    ui_blur_runtime = ui_target_blur_level();
    net_sync_link_from_state();
}

static void reset_runtime_data_defaults(void) {
    config_reset_defaults();
    state_sync_from_config();
    ui_scale_override = 0u;
    terminal_init();
    notepad_bind_default_file();
    calc_state.expr_len = 0u;
    calc_state.expr[0] = '\0';
    image_state.sample_index = 0u;
    video_state.playing = false;
    video_state.frame_index = 0u;
    video_state.frame_accum_cycles = 0ull;
    browser_state.url_len = 0u;
    browser_state.url[0] = '\0';
    browser_state.input_focus = false;
    browser_nav_token = 0u;
    browser_navigate("WEVOA/HOME");
    text_copy(settings_net_status_line, TERM_LINE_MAX, "NET READY");
    lock_screen_active = false;
    lock_screen_bad_pin = false;
    lock_screen_pin_len = 0u;
    lock_screen_pin_input[0] = '\0';
}

static void pstore_fill_payload(struct pstore_payload_v1* payload) {
    if (payload == 0) {
        return;
    }

    config_sync_from_state();
    payload->magic = PSTORE_PAYLOAD_MAGIC;
    payload->version = PSTORE_PAYLOAD_VERSION;
    payload->cfg = *config_snapshot();
    payload->desktop_new_folder_id = desktop_new_folder_id;
    payload->desktop_new_file_id = desktop_new_file_id;
    payload->desktop_root_node = desktop_root_node;
    payload->terminal_cwd_node = terminal_state.cwd_node;
    payload->ui_scale_override = ui_scale_override;

    for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
        payload->nodes[i].used = vfs_nodes[i].used ? 1u : 0u;
        payload->nodes[i].parent = vfs_nodes[i].parent;
        payload->nodes[i].type = vfs_nodes[i].type;
        text_copy(payload->nodes[i].name, VFS_NAME_MAX, vfs_nodes[i].name);
        text_copy(payload->nodes[i].content, VFS_CONTENT_MAX, vfs_nodes[i].content);
    }
}

static int pstore_apply_payload(const struct pstore_payload_v1* payload) {
    if (payload == 0) {
        return W_ERR_INVALID_ARG;
    }
    if (payload->magic != PSTORE_PAYLOAD_MAGIC || payload->version != PSTORE_PAYLOAD_VERSION) {
        return W_ERR_CORRUPT;
    }

    if (config_apply(&payload->cfg) != W_OK) {
        config_reset_defaults();
    }
    state_sync_from_config();

    for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
        vfs_nodes[i].used = payload->nodes[i].used != 0u;
        vfs_nodes[i].parent = payload->nodes[i].parent;
        vfs_nodes[i].type = payload->nodes[i].type;
        text_copy(vfs_nodes[i].name, VFS_NAME_MAX, payload->nodes[i].name);
        text_copy(vfs_nodes[i].content, VFS_CONTENT_MAX, payload->nodes[i].content);
    }

    desktop_new_folder_id = payload->desktop_new_folder_id == 0u ? 1u : payload->desktop_new_folder_id;
    desktop_new_file_id = payload->desktop_new_file_id == 0u ? 1u : payload->desktop_new_file_id;

    desktop_root_node = payload->desktop_root_node;
    if (desktop_root_node >= VFS_MAX_NODES || !vfs_nodes[desktop_root_node].used || vfs_nodes[desktop_root_node].type != VFS_TYPE_DIR) {
        uint8_t fallback = vfs_find_child(0u, "DESKTOP");
        if (fallback == VFS_INVALID_NODE || !vfs_nodes[fallback].used) {
            reset_runtime_data_defaults();
            return W_ERR_CORRUPT;
        }
        desktop_root_node = fallback;
    }

    terminal_state.cwd_node = payload->terminal_cwd_node;
    if (terminal_state.cwd_node >= VFS_MAX_NODES || !vfs_nodes[terminal_state.cwd_node].used || vfs_nodes[terminal_state.cwd_node].type != VFS_TYPE_DIR) {
        terminal_state.cwd_node = desktop_root_node;
    }
    ui_scale_override = (payload->ui_scale_override <= 3u) ? payload->ui_scale_override : 0u;

    waste_bin_node = vfs_find_child(0u, "WASTEBIN");
    if (waste_bin_node == VFS_INVALID_NODE) {
        waste_bin_node = vfs_add_node(0u, VFS_TYPE_DIR, "WASTEBIN", 0);
        if (waste_bin_node == VFS_INVALID_NODE) {
            return W_ERR_NO_SPACE;
        }
    }
    vfs_clipboard.has_data = false;
    vfs_clipboard.cut_mode = false;
    vfs_clipboard.node = VFS_INVALID_NODE;
    vfs_clipboard.name[0] = '\0';
    notepad_bind_default_file();
    calc_state.expr_len = 0u;
    calc_state.expr[0] = '\0';

    sync_desktop_items_from_vfs();
    return W_OK;
}

static int pstore_save_runtime(void) {
    if (!pstore_available) {
        return W_ERR_UNSUPPORTED;
    }
    pstore_fill_payload(&pstore_payload_cache);
    return pstore_save(&pstore_payload_cache, (uint32_t)sizeof(pstore_payload_cache), &pstore_sequence);
}

static int pstore_load_runtime(void) {
    if (!pstore_available) {
        return W_ERR_UNSUPPORTED;
    }
    int rc = pstore_load(&pstore_payload_cache, (uint32_t)sizeof(pstore_payload_cache), &pstore_sequence);
    if (rc != W_OK) {
        return rc;
    }
    return pstore_apply_payload(&pstore_payload_cache);
}

static int config_key_from_name(const char* key_name, enum wevoa_config_key* out_key) {
    if (key_name == 0 || out_key == 0) {
        return W_ERR_INVALID_ARG;
    }
    if (text_equal(key_name, "BRIGHT") || text_equal(key_name, "BRIGHTNESS")) {
        *out_key = WEVOA_CFG_BRIGHT;
        return W_OK;
    }
    if (text_equal(key_name, "WALL") || text_equal(key_name, "WALLPAPER")) {
        *out_key = WEVOA_CFG_WALL;
        return W_OK;
    }
    if (text_equal(key_name, "WIFI")) {
        *out_key = WEVOA_CFG_WIFI;
        return W_OK;
    }
    if (text_equal(key_name, "NET") || text_equal(key_name, "NETWORK")) {
        *out_key = WEVOA_CFG_NET_LINK;
        return W_OK;
    }
    if (text_equal(key_name, "CURSOR_SPEED")) {
        *out_key = WEVOA_CFG_CURSOR_SPEED;
        return W_OK;
    }
    if (text_equal(key_name, "KEY_REPEAT_DELAY")) {
        *out_key = WEVOA_CFG_KEY_REPEAT_DELAY;
        return W_OK;
    }
    if (text_equal(key_name, "KEY_REPEAT_RATE")) {
        *out_key = WEVOA_CFG_KEY_REPEAT_RATE;
        return W_OK;
    }
    if (text_equal(key_name, "LOCK") || text_equal(key_name, "LOCK_ENABLED")) {
        *out_key = WEVOA_CFG_LOCK_ENABLED;
        return W_OK;
    }
    if (text_equal(key_name, "LOCK_PIN") || text_equal(key_name, "PIN")) {
        *out_key = WEVOA_CFG_LOCK_PIN;
        return W_OK;
    }
    if (text_equal(key_name, "SHELL_LAYOUT") || text_equal(key_name, "SHELL") || text_equal(key_name, "LAYOUT")) {
        *out_key = WEVOA_CFG_SHELL_LAYOUT;
        return W_OK;
    }
    if (text_equal(key_name, "UI_BLUR_LEVEL") || text_equal(key_name, "BLUR")) {
        *out_key = WEVOA_CFG_UI_BLUR_LEVEL;
        return W_OK;
    }
    if (text_equal(key_name, "UI_MOTION_LEVEL") || text_equal(key_name, "MOTION")) {
        *out_key = WEVOA_CFG_UI_MOTION_LEVEL;
        return W_OK;
    }
    if (text_equal(key_name, "UI_EFFECTS_QUALITY") || text_equal(key_name, "QUALITY")) {
        *out_key = WEVOA_CFG_UI_EFFECTS_QUALITY;
        return W_OK;
    }
    return W_ERR_NOT_FOUND;
}

static uint8_t vfs_add_node(uint8_t parent, uint8_t type, const char* name, const char* content) {
    if (name == 0 || name[0] == '\0') {
        return VFS_INVALID_NODE;
    }
    if (parent != VFS_INVALID_NODE) {
        if (parent >= VFS_MAX_NODES || !vfs_nodes[parent].used || vfs_nodes[parent].type != VFS_TYPE_DIR) {
            return VFS_INVALID_NODE;
        }
    }

    for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
        if (!vfs_nodes[i].used) {
            vfs_nodes[i].used = true;
            vfs_nodes[i].parent = parent;
            vfs_nodes[i].type = type;
            text_copy(vfs_nodes[i].name, VFS_NAME_MAX, name);
            text_upper_inplace(vfs_nodes[i].name);
            if (type == VFS_TYPE_FILE) {
                text_copy(vfs_nodes[i].content, VFS_CONTENT_MAX, content == 0 ? "" : content);
            } else {
                vfs_nodes[i].content[0] = '\0';
            }
            return (uint8_t)i;
        }
    }
    return VFS_INVALID_NODE;
}

static uint8_t vfs_find_child(uint8_t parent, const char* name) {
    char wanted[VFS_NAME_MAX];
    text_copy(wanted, VFS_NAME_MAX, name);
    text_upper_inplace(wanted);

    for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
        if (!vfs_nodes[i].used) {
            continue;
        }
        if (vfs_nodes[i].parent == parent && text_equal(vfs_nodes[i].name, wanted)) {
            return (uint8_t)i;
        }
    }
    return VFS_INVALID_NODE;
}

static uint8_t vfs_ensure_waste_bin(void) {
    if (waste_bin_node < VFS_MAX_NODES &&
        vfs_nodes[waste_bin_node].used &&
        vfs_nodes[waste_bin_node].type == VFS_TYPE_DIR &&
        vfs_nodes[waste_bin_node].parent == 0u) {
        return waste_bin_node;
    }

    uint8_t found = vfs_find_child(0u, "WASTEBIN");
    if (found != VFS_INVALID_NODE) {
        waste_bin_node = found;
        return waste_bin_node;
    }

    uint8_t created = vfs_add_node(0u, VFS_TYPE_DIR, "WASTEBIN", 0);
    if (created != VFS_INVALID_NODE) {
        waste_bin_node = created;
    }
    return waste_bin_node;
}

static bool vfs_is_descendant(uint8_t node, uint8_t ancestor) {
    if (node >= VFS_MAX_NODES || ancestor >= VFS_MAX_NODES) {
        return false;
    }
    uint8_t cur = node;
    uint8_t depth = 0u;
    while (cur < VFS_MAX_NODES && depth < VFS_MAX_NODES) {
        if (cur == ancestor) {
            return true;
        }
        depth++;
        uint8_t parent = vfs_nodes[cur].parent;
        if (parent == VFS_INVALID_NODE || parent == cur) {
            break;
        }
        cur = parent;
    }
    return false;
}

static void vfs_reset_node(uint8_t node) {
    if (node >= VFS_MAX_NODES) {
        return;
    }
    vfs_nodes[node].used = false;
    vfs_nodes[node].parent = VFS_INVALID_NODE;
    vfs_nodes[node].type = VFS_TYPE_FILE;
    vfs_nodes[node].name[0] = '\0';
    vfs_nodes[node].content[0] = '\0';
}

static void vfs_clear_subtree(uint8_t node) {
    if (node >= VFS_MAX_NODES || !vfs_nodes[node].used) {
        return;
    }
    for (;;) {
        bool changed = false;
        for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
            if (!vfs_nodes[i].used) {
                continue;
            }
            if ((uint8_t)i == node || vfs_is_descendant((uint8_t)i, node)) {
                vfs_reset_node((uint8_t)i);
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }
}

static void vfs_build_unique_copy_name(const char* base_name, char out[VFS_NAME_MAX]) {
    text_copy(out, VFS_NAME_MAX, base_name);
    if (text_len(out) <= 11u) {
        text_append(out, VFS_NAME_MAX, "_CP");
    } else {
        out[11] = '\0';
        text_append(out, VFS_NAME_MAX, "_CP");
    }
    text_upper_inplace(out);
}

static uint8_t vfs_clone_subtree(uint8_t src_node, uint8_t dst_parent, const char* override_name) {
    if (src_node >= VFS_MAX_NODES || !vfs_nodes[src_node].used) {
        return VFS_INVALID_NODE;
    }
    if (dst_parent >= VFS_MAX_NODES || !vfs_nodes[dst_parent].used || vfs_nodes[dst_parent].type != VFS_TYPE_DIR) {
        return VFS_INVALID_NODE;
    }

    const char* name = (override_name != 0 && override_name[0] != '\0') ? override_name : vfs_nodes[src_node].name;
    uint8_t new_node = vfs_add_node(dst_parent, vfs_nodes[src_node].type, name, vfs_nodes[src_node].content);
    if (new_node == VFS_INVALID_NODE) {
        return VFS_INVALID_NODE;
    }

    if (vfs_nodes[src_node].type == VFS_TYPE_DIR) {
        for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
            if (!vfs_nodes[i].used || vfs_nodes[i].parent != src_node) {
                continue;
            }
            if (vfs_clone_subtree((uint8_t)i, new_node, 0) == VFS_INVALID_NODE) {
                vfs_clear_subtree(new_node);
                return VFS_INVALID_NODE;
            }
        }
    }
    return new_node;
}

static int vfs_move_to_waste_bin(uint8_t node) {
    if (node >= VFS_MAX_NODES || !vfs_nodes[node].used) {
        return W_ERR_NOT_FOUND;
    }
    if (node == 0u) {
        return W_ERR_UNSUPPORTED;
    }
    uint8_t wb = vfs_ensure_waste_bin();
    if (wb == VFS_INVALID_NODE) {
        return W_ERR_NO_SPACE;
    }
    if (node == wb || vfs_is_descendant(wb, node)) {
        return W_ERR_UNSUPPORTED;
    }

    char new_name[VFS_NAME_MAX];
    text_copy(new_name, VFS_NAME_MAX, vfs_nodes[node].name);
    if (vfs_find_child(wb, new_name) != VFS_INVALID_NODE) {
        vfs_build_unique_copy_name(vfs_nodes[node].name, new_name);
    }
    if (vfs_find_child(wb, new_name) != VFS_INVALID_NODE) {
        return W_ERR_EXISTS;
    }

    vfs_nodes[node].parent = wb;
    text_copy(vfs_nodes[node].name, VFS_NAME_MAX, new_name);
    return W_OK;
}

static int vfs_restore_from_waste_bin(uint8_t node, const char* override_name) {
    uint8_t wb = vfs_ensure_waste_bin();
    if (wb == VFS_INVALID_NODE) {
        return W_ERR_NOT_FOUND;
    }
    if (node >= VFS_MAX_NODES || !vfs_nodes[node].used || vfs_nodes[node].parent != wb) {
        return W_ERR_NOT_FOUND;
    }

    char final_name[VFS_NAME_MAX];
    if (override_name != 0 && override_name[0] != '\0') {
        text_copy(final_name, VFS_NAME_MAX, override_name);
    } else {
        text_copy(final_name, VFS_NAME_MAX, vfs_nodes[node].name);
    }
    text_upper_inplace(final_name);

    if (vfs_find_child(terminal_state.cwd_node, final_name) != VFS_INVALID_NODE) {
        return W_ERR_EXISTS;
    }

    vfs_nodes[node].parent = terminal_state.cwd_node;
    text_copy(vfs_nodes[node].name, VFS_NAME_MAX, final_name);
    return W_OK;
}

static void sync_desktop_items_from_vfs(void) {
    desktop_item_count = 0u;
    for (uint32_t i = 0u; i < DESKTOP_ITEM_MAX; ++i) {
        desktop_items[i].used = false;
        desktop_item_x[i] = 0;
        desktop_item_y[i] = 0;
    }

    if (desktop_root_node == VFS_INVALID_NODE) {
        return;
    }

    for (uint32_t i = 0u; i < VFS_MAX_NODES && desktop_item_count < DESKTOP_ITEM_MAX; ++i) {
        if (!vfs_nodes[i].used || vfs_nodes[i].parent != desktop_root_node) {
            continue;
        }
        struct desktop_item* it = &desktop_items[desktop_item_count];
        it->used = true;
        it->is_dir = (vfs_nodes[i].type == VFS_TYPE_DIR);
        it->node = (uint8_t)i;
        text_copy(it->name, VFS_NAME_MAX, vfs_nodes[i].name);
        desktop_item_count++;
    }
}

static void terminal_push_line(const char* text) {
    if (text == 0) {
        return;
    }

    if (terminal_state.line_count < TERM_HISTORY_LINES) {
        text_copy(terminal_state.lines[terminal_state.line_count], TERM_LINE_MAX, text);
        terminal_state.line_count++;
        return;
    }

    for (uint32_t i = 1u; i < TERM_HISTORY_LINES; ++i) {
        text_copy(terminal_state.lines[i - 1u], TERM_LINE_MAX, terminal_state.lines[i]);
    }
    text_copy(terminal_state.lines[TERM_HISTORY_LINES - 1u], TERM_LINE_MAX, text);
}

static void terminal_push_ok(const char* msg) {
    char line[TERM_LINE_MAX];
    text_copy(line, TERM_LINE_MAX, "OK ");
    text_append(line, TERM_LINE_MAX, msg);
    terminal_push_line(line);
}

static void terminal_push_err(const char* msg) {
    char line[TERM_LINE_MAX];
    text_copy(line, TERM_LINE_MAX, "ERROR ");
    text_append(line, TERM_LINE_MAX, msg);
    terminal_push_line(line);
}

static void terminal_clear_history(void) {
    terminal_state.line_count = 0u;
    for (uint32_t i = 0u; i < TERM_HISTORY_LINES; ++i) {
        terminal_state.lines[i][0] = '\0';
    }
}

static void vfs_build_path(uint8_t node, char out[TERM_LINE_MAX]) {
    uint8_t chain[16];
    uint8_t depth = 0u;
    uint8_t cur = node;

    if (cur == VFS_INVALID_NODE || !vfs_nodes[cur].used) {
        text_copy(out, TERM_LINE_MAX, "/");
        return;
    }

    while (depth < 16u && cur != VFS_INVALID_NODE && vfs_nodes[cur].used) {
        chain[depth++] = cur;
        cur = vfs_nodes[cur].parent;
    }

    out[0] = '/';
    out[1] = '\0';
    for (int32_t i = (int32_t)depth - 2; i >= 0; --i) {
        text_append(out, TERM_LINE_MAX, vfs_nodes[chain[i]].name);
        if (i != 0) {
            text_append(out, TERM_LINE_MAX, "/");
        }
    }
}

static uint8_t split_tokens(char* line, char* argv[8]) {
    uint8_t argc = 0u;
    if (line == 0) {
        return 0u;
    }

    char* p = line;
    while (*p != '\0' && argc < 8u) {
        while (*p == ' ') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        argv[argc++] = p;
        while (*p != '\0' && *p != ' ') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        *p = '\0';
        p++;
    }

    return argc;
}

static void terminal_init(void) {
    for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
        vfs_nodes[i].used = false;
        vfs_nodes[i].name[0] = '\0';
        vfs_nodes[i].content[0] = '\0';
        vfs_nodes[i].parent = VFS_INVALID_NODE;
        vfs_nodes[i].type = VFS_TYPE_FILE;
    }

    uint8_t root = vfs_add_node(VFS_INVALID_NODE, VFS_TYPE_DIR, "ROOT", 0);
    uint8_t desktop = vfs_add_node(root, VFS_TYPE_DIR, "DESKTOP", 0);
    uint8_t docs = vfs_add_node(root, VFS_TYPE_DIR, "DOCS", 0);
    uint8_t waste_bin = vfs_add_node(root, VFS_TYPE_DIR, "WASTEBIN", 0);
    (void)docs;
    (void)waste_bin;
    (void)vfs_add_node(desktop, VFS_TYPE_DIR, "PROJECTS", 0);
    (void)vfs_add_node(desktop, VFS_TYPE_FILE, "README.TXT", "WELCOME TO WEVOA");
    (void)vfs_add_node(root, VFS_TYPE_FILE, "SYSTEM.LOG", "WEVOA GUI READY");

    desktop_root_node = desktop;
    waste_bin_node = waste_bin;
    desktop_new_folder_id = 1u;
    desktop_new_file_id = 1u;
    vfs_clipboard.has_data = false;
    vfs_clipboard.cut_mode = false;
    vfs_clipboard.node = VFS_INVALID_NODE;
    vfs_clipboard.name[0] = '\0';
    sync_desktop_items_from_vfs();

    terminal_clear_history();
    terminal_state.cwd_node = desktop;
    terminal_state.input_len = 0u;
    terminal_state.input[0] = '\0';
    notepad_state.file_node = VFS_INVALID_NODE;
    notepad_state.file_name[0] = '\0';
    notepad_state.buffer[0] = '\0';
    notepad_state.len = 0u;
    notepad_state.dirty = false;
    calc_state.expr_len = 0u;
    calc_state.expr[0] = '\0';
    terminal_push_line("WEVOA TERMINAL");
    terminal_push_line("TYPE HELP FOR COMMANDS");
    terminal_push_line("LS DIR CD MD TOUCH EDIT TYPE CLS");
}

static void notepad_load_from_node(uint8_t node) {
    if (node >= VFS_MAX_NODES || !vfs_nodes[node].used || vfs_nodes[node].type != VFS_TYPE_FILE) {
        return;
    }
    notepad_state.file_node = node;
    text_copy(notepad_state.file_name, VFS_NAME_MAX, vfs_nodes[node].name);
    text_copy(notepad_state.buffer, VFS_CONTENT_MAX, vfs_nodes[node].content);
    notepad_state.len = (uint8_t)text_len(notepad_state.buffer);
    notepad_state.dirty = false;
}

static void notepad_bind_default_file(void) {
    uint8_t base = desktop_root_node;
    if (base == VFS_INVALID_NODE || base >= VFS_MAX_NODES || !vfs_nodes[base].used) {
        base = vfs_find_child(0u, "DESKTOP");
    }
    if (base == VFS_INVALID_NODE) {
        notepad_state.file_node = VFS_INVALID_NODE;
        notepad_state.file_name[0] = '\0';
        notepad_state.buffer[0] = '\0';
        notepad_state.len = 0u;
        notepad_state.dirty = false;
        return;
    }

    uint8_t node = vfs_find_child(base, "MAIN.WV");
    bool created = false;
    if (node == VFS_INVALID_NODE) {
        node = vfs_add_node(base, VFS_TYPE_FILE, "MAIN.WV",
                            "INT MAIN() {\n"
                            "    RETURN 0;\n"
                            "}");
        created = (node != VFS_INVALID_NODE);
    }
    if (node == VFS_INVALID_NODE) {
        notepad_state.file_node = VFS_INVALID_NODE;
        return;
    }
    if (created && base == desktop_root_node) {
        sync_desktop_items_from_vfs();
    }
    notepad_load_from_node(node);
}

static void notepad_save_to_file(void) {
    if (notepad_state.file_node == VFS_INVALID_NODE || notepad_state.file_node >= VFS_MAX_NODES ||
        !vfs_nodes[notepad_state.file_node].used || vfs_nodes[notepad_state.file_node].type != VFS_TYPE_FILE) {
        notepad_bind_default_file();
    }
    if (notepad_state.file_node == VFS_INVALID_NODE) {
        return;
    }
    text_copy(vfs_nodes[notepad_state.file_node].content, VFS_CONTENT_MAX, notepad_state.buffer);
    notepad_state.dirty = false;
    mark_pstore_dirty();
    if (vfs_nodes[notepad_state.file_node].parent == desktop_root_node) {
        sync_desktop_items_from_vfs();
        build_desktop_template();
    }
}

static bool notepad_handle_key_input(void) {
    if (state.active_app != (int32_t)APP_NOTEPAD ||
        !state.windows[APP_NOTEPAD].open ||
        state.windows[APP_NOTEPAD].minimized) {
        return false;
    }

    bool changed = false;
    bool shift = state.key_down[SC_LSHIFT] || state.key_down[SC_RSHIFT];
    if (state.key_pressed[SC_BACKSPACE]) {
        if (notepad_state.len > 0u) {
            notepad_state.len--;
            notepad_state.buffer[notepad_state.len] = '\0';
            notepad_state.dirty = true;
            changed = true;
        }
    }
    if (state.key_pressed[SC_ENTER]) {
        if (notepad_state.len + 1u < VFS_CONTENT_MAX) {
            notepad_state.buffer[notepad_state.len++] = '\n';
            notepad_state.buffer[notepad_state.len] = '\0';
            notepad_state.dirty = true;
            changed = true;
        }
    }
    if (state.key_pressed[SC_TAB]) {
        for (uint32_t i = 0u; i < 4u; ++i) {
            if (notepad_state.len + 1u >= VFS_CONTENT_MAX) {
                break;
            }
            notepad_state.buffer[notepad_state.len++] = ' ';
            notepad_state.buffer[notepad_state.len] = '\0';
            notepad_state.dirty = true;
            changed = true;
        }
    }
    for (uint32_t sc = 0u; sc < KEY_MAX; ++sc) {
        if (!state.key_pressed[sc]) {
            continue;
        }
        if (sc == SC_ENTER || sc == SC_BACKSPACE || sc == SC_TAB || sc == SC_ESC) {
            continue;
        }
        char ch = scancode_to_char((uint8_t)sc, shift);
        if (ch == '\0') {
            continue;
        }
        if (notepad_state.len + 1u < VFS_CONTENT_MAX) {
            notepad_state.buffer[notepad_state.len++] = ch;
            notepad_state.buffer[notepad_state.len] = '\0';
            notepad_state.dirty = true;
            changed = true;
        }
    }
    return changed;
}

static int32_t calc_eval_expr(const char* expr, bool* ok) {
    if (ok != 0) {
        *ok = false;
    }
    if (expr == 0 || expr[0] == '\0') {
        return 0;
    }

    uint32_t i = 0u;
    int32_t acc = 0;
    bool has_digit = false;
    while (expr[i] >= '0' && expr[i] <= '9') {
        has_digit = true;
        acc = acc * 10 + (int32_t)(expr[i] - '0');
        i++;
    }
    if (!has_digit) {
        return 0;
    }

    while (expr[i] != '\0') {
        char op = expr[i++];
        if (!(op == '+' || op == '-' || op == '*' || op == '/')) {
            return 0;
        }
        int32_t rhs = 0;
        has_digit = false;
        while (expr[i] >= '0' && expr[i] <= '9') {
            has_digit = true;
            rhs = rhs * 10 + (int32_t)(expr[i] - '0');
            i++;
        }
        if (!has_digit) {
            return 0;
        }
        if (op == '+') {
            acc += rhs;
        } else if (op == '-') {
            acc -= rhs;
        } else if (op == '*') {
            acc *= rhs;
        } else {
            if (rhs == 0) {
                return 0;
            }
            acc /= rhs;
        }
    }

    if (ok != 0) {
        *ok = true;
    }
    return acc;
}

static void calc_set_display_value(int32_t value) {
    char n[24];
    if (value < 0) {
        n[0] = '-';
        u32_to_dec_text((uint32_t)(-value), &n[1], sizeof(n) - 1u);
    } else {
        u32_to_dec_text((uint32_t)value, n, sizeof(n));
    }
    text_copy(calc_state.expr, sizeof(calc_state.expr), n);
    calc_state.expr_len = (uint8_t)text_len(calc_state.expr);
}

static bool calc_handle_key_input(void) {
    if (state.active_app != (int32_t)APP_CALCULATOR ||
        !state.windows[APP_CALCULATOR].open ||
        state.windows[APP_CALCULATOR].minimized) {
        return false;
    }
    bool changed = false;
    if (state.key_pressed[SC_BACKSPACE] && calc_state.expr_len > 0u) {
        calc_state.expr_len--;
        calc_state.expr[calc_state.expr_len] = '\0';
        changed = true;
    }
    if (state.key_pressed[SC_C]) {
        calc_state.expr_len = 0u;
        calc_state.expr[0] = '\0';
        changed = true;
    }

    if (state.key_pressed[SC_ENTER] || state.key_pressed[SC_EQUAL]) {
        bool ok = false;
        int32_t v = calc_eval_expr(calc_state.expr, &ok);
        if (ok) {
            calc_set_display_value(v);
        } else {
            text_copy(calc_state.expr, sizeof(calc_state.expr), "ERR");
            calc_state.expr_len = 3u;
        }
        changed = true;
    }

    for (uint32_t sc = 0u; sc < KEY_MAX; ++sc) {
        if (!state.key_pressed[sc]) {
            continue;
        }
        char token = '\0';
        if (sc == SC_EQUAL) {
            token = '+';
        } else if (sc == SC_X) {
            token = '*';
        } else {
            token = scancode_to_char((uint8_t)sc, false);
            if (!(token >= '0' && token <= '9') &&
                token != '-' && token != '/' && token != '+') {
                token = '\0';
            }
        }
        if (token == '\0') {
            continue;
        }
        if (calc_state.expr_len + 1u < sizeof(calc_state.expr)) {
            calc_state.expr[calc_state.expr_len++] = token;
            calc_state.expr[calc_state.expr_len] = '\0';
            changed = true;
        }
    }
    return changed;
}

static bool text_contains(const char* haystack, const char* needle) {
    if (haystack == 0 || needle == 0 || needle[0] == '\0') {
        return false;
    }
    uint32_t hlen = text_len(haystack);
    uint32_t nlen = text_len(needle);
    if (nlen > hlen) {
        return false;
    }
    for (uint32_t i = 0u; i + nlen <= hlen; ++i) {
        uint32_t j = 0u;
        while (j < nlen && haystack[i + j] == needle[j]) {
            j++;
        }
        if (j == nlen) {
            return true;
        }
    }
    return false;
}

static void browser_build_request_url(const char* raw_url, char request_url[TERM_INPUT_MAX + 16u]) {
    char normalized[TERM_INPUT_MAX];
    uint32_t host_end = 0u;
    bool host_has_dot = false;

    text_copy(normalized, sizeof(normalized), raw_url);
    if (normalized[0] == '\0') {
        text_copy(request_url, TERM_INPUT_MAX + 16u, "HTTPS://DEVDOPZ.COM");
        return;
    }

    if (text_contains(normalized, "HTTPS://") || text_contains(normalized, "HTTP://")) {
        text_copy(request_url, TERM_INPUT_MAX + 16u, normalized);
        return;
    }

    while (normalized[host_end] != '\0' && normalized[host_end] != '/') {
        if (normalized[host_end] == '.') {
            host_has_dot = true;
        }
        host_end++;
    }

    if (host_end > 0u && !host_has_dot) {
        char host[TERM_INPUT_MAX];
        char tail[TERM_INPUT_MAX];
        uint32_t i = 0u;
        for (; i < host_end && i + 1u < sizeof(host); ++i) {
            host[i] = normalized[i];
        }
        host[i] = '\0';
        if (normalized[host_end] != '\0') {
            text_copy(tail, sizeof(tail), &normalized[host_end]);
        } else {
            tail[0] = '\0';
        }
        text_copy(normalized, sizeof(normalized), host);
        text_append(normalized, sizeof(normalized), ".COM");
        text_append(normalized, sizeof(normalized), tail);
    }

    text_copy(request_url, TERM_INPUT_MAX + 16u, "HTTPS://");
    text_append(request_url, TERM_INPUT_MAX + 16u, normalized);
}

static void browser_navigate(const char* input_url) {
    char url[TERM_INPUT_MAX];
    char request_url[TERM_INPUT_MAX + 16u];
    struct wevoa_http_page page;
    uint8_t ip[4] = {0u, 0u, 0u, 0u};
    uint16_t port = 0u;
    int net_rc = W_OK;
    text_copy(url, sizeof(url), input_url);
    text_upper_inplace(url);
    if (url[0] == '\0') {
        text_copy(url, sizeof(url), "WEVOA/HOME");
    }

    text_copy(browser_state.url, sizeof(browser_state.url), url);
    browser_state.url_len = (uint8_t)text_len(browser_state.url);
    browser_nav_token++;
    browser_state.last_ip[0] = browser_state.last_ip[1] = browser_state.last_ip[2] = browser_state.last_ip[3] = 0u;
    browser_state.last_port = 0u;
    text_copy(browser_state.status_line, TERM_LINE_MAX, "READY");

    if (text_contains(url, "WEVOA/HOME") || text_contains(url, "HOME")) {
        text_copy(browser_state.page_title, TERM_LINE_MAX, "WEVOA START PAGE");
        text_copy(browser_state.page_line1, TERM_LINE_MAX, "WELCOME TO WEVOA BROWSER");
        text_copy(browser_state.page_line2, TERM_LINE_MAX, "TRY: WEB DEVDOPZ.COM");
        text_copy(browser_state.page_line3, TERM_LINE_MAX, "DNS/FIREWALL/PORT LAYER ACTIVE");
        text_copy(browser_state.status_line, TERM_LINE_MAX, "LOCAL PAGE");
        return;
    }

    if (text_contains(url, "WEVOA/HELP") || text_contains(url, "ABOUT")) {
        text_copy(browser_state.page_title, TERM_LINE_MAX, "BROWSER HELP");
        text_copy(browser_state.page_line1, TERM_LINE_MAX, "ENTER URL THEN CLICK GO OR PRESS ENTER");
        text_copy(browser_state.page_line2, TERM_LINE_MAX, "WEB <URL>, DNS <HOST>, PORT <P> ALLOW/BLOCK");
        text_copy(browser_state.page_line3, TERM_LINE_MAX, "HTTPS PREVIEW MODE (FULL TCP/TLS NEXT)");
        text_copy(browser_state.status_line, TERM_LINE_MAX, "LOCAL HELP");
        return;
    }

    browser_build_request_url(url, request_url);
    net_rc = net_http_get(request_url, &page, ip, &port);
    if (net_rc == W_ERR_IO) {
        int rc_wifi = config_set(WEVOA_CFG_WIFI, 1u);
        int rc_link = config_set(WEVOA_CFG_NET_LINK, 1u);
        state_sync_from_config();
        if (rc_wifi == W_OK && rc_link == W_OK) {
            text_copy(settings_net_status_line, TERM_LINE_MAX, "AUTO LINK ONLINE");
            mark_pstore_dirty();
            net_rc = net_http_get(request_url, &page, ip, &port);
        }
    }
    if (net_rc == W_OK) {
        text_copy(browser_state.page_title, TERM_LINE_MAX, page.title);
        text_copy(browser_state.page_line1, TERM_LINE_MAX, page.line1);
        text_copy(browser_state.page_line2, TERM_LINE_MAX, page.line2);
        text_copy(browser_state.page_line3, TERM_LINE_MAX, page.line3);
        browser_state.last_ip[0] = ip[0];
        browser_state.last_ip[1] = ip[1];
        browser_state.last_ip[2] = ip[2];
        browser_state.last_ip[3] = ip[3];
        browser_state.last_port = port;

        char ip_text[20];
        char port_text[12];
        text_copy(browser_state.status_line, TERM_LINE_MAX, "DNS OK IP ");
        ipv4_to_text(browser_state.last_ip, ip_text);
        text_append(browser_state.status_line, TERM_LINE_MAX, ip_text);
        text_append(browser_state.status_line, TERM_LINE_MAX, " PORT ");
        u32_to_dec_text((uint32_t)browser_state.last_port, port_text, sizeof(port_text));
        text_append(browser_state.status_line, TERM_LINE_MAX, port_text);
        return;
    }

    if (net_rc == W_ERR_NOT_FOUND) {
        text_copy(browser_state.page_title, TERM_LINE_MAX, "DNS NOT FOUND");
        text_copy(browser_state.page_line1, TERM_LINE_MAX, "HOST UNKNOWN");
        text_copy(browser_state.page_line2, TERM_LINE_MAX, "TRY: DEVDOPZ.COM OR OPENAI.COM");
        text_copy(browser_state.page_line3, TERM_LINE_MAX, "HTTPS AUTO-ADDED (NO COLON NEEDED)");
        text_copy(browser_state.status_line, TERM_LINE_MAX, "DNS MISS");
        return;
    }

    text_copy(browser_state.page_title, TERM_LINE_MAX, "NETWORK ERROR");
    text_copy(browser_state.page_line1, TERM_LINE_MAX, "REQUEST FAILED");
    text_copy(browser_state.page_line2, TERM_LINE_MAX, wstatus_str(net_rc));
    text_copy(browser_state.page_line3, TERM_LINE_MAX, "CHECK WIFI/NET LINK, DNS, FIREWALL PORT");
    text_copy(browser_state.status_line, TERM_LINE_MAX, "NET FAIL");
}

static bool browser_handle_key_input(void) {
    if (state.active_app != (int32_t)APP_BROWSER ||
        !state.windows[APP_BROWSER].open ||
        state.windows[APP_BROWSER].minimized) {
        return false;
    }

    bool changed = false;
    bool shift = state.key_down[SC_LSHIFT] || state.key_down[SC_RSHIFT];
    if (state.key_pressed[SC_BACKSPACE]) {
        if (browser_state.url_len > 0u) {
            browser_state.url_len--;
            browser_state.url[browser_state.url_len] = '\0';
            changed = true;
        }
    }

    if (state.key_pressed[SC_ENTER]) {
        browser_navigate(browser_state.url);
        changed = true;
    }

    for (uint32_t sc = 0u; sc < KEY_MAX; ++sc) {
        if (!state.key_pressed[sc]) {
            continue;
        }
        if (sc == SC_ENTER || sc == SC_BACKSPACE || sc == SC_TAB || sc == SC_ESC) {
            continue;
        }
        char ch = scancode_to_char((uint8_t)sc, shift);
        if (ch == '\0') {
            continue;
        }
        if (browser_state.url_len + 1u < sizeof(browser_state.url)) {
            browser_state.url[browser_state.url_len++] = ch;
            browser_state.url[browser_state.url_len] = '\0';
            changed = true;
        }
    }
    return changed;
}

static void build_desktop_generated_name(bool is_dir, char out[VFS_NAME_MAX]) {
    char id[4];
    if (is_dir) {
        u8_to_dec_text(desktop_new_folder_id, id);
        text_copy(out, VFS_NAME_MAX, "FOLDER");
        text_append(out, VFS_NAME_MAX, id);
        desktop_new_folder_id++;
    } else {
        u8_to_dec_text(desktop_new_file_id, id);
        text_copy(out, VFS_NAME_MAX, "FILE");
        text_append(out, VFS_NAME_MAX, id);
        text_append(out, VFS_NAME_MAX, ".TXT");
        desktop_new_file_id++;
    }
}

static void palette_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x03C8u, index);
    outb(0x03C9u, r);
    outb(0x03C9u, g);
    outb(0x03C9u, b);
}

static uint32_t rgb6_to_rgb32(uint8_t r6, uint8_t g6, uint8_t b6) {
    uint32_t r8 = ((uint32_t)r6 * 255u) / 63u;
    uint32_t g8 = ((uint32_t)g6 * 255u) / 63u;
    uint32_t b8 = ((uint32_t)b6 * 255u) / 63u;
    return (r8 << 16) | (g8 << 8) | b8;
}

static void apply_theme_brightness(void) {
    uint32_t b = (uint32_t)state.settings_brightness;
    if (b < 10u) {
        b = 10u;
    }
    if (b > 100u) {
        b = 100u;
    }

    for (uint32_t i = 0; i < COLOR_COUNT; ++i) {
        uint8_t r6 = (uint8_t)((theme_rgb6[i][0] * b) / 100u);
        uint8_t g6 = (uint8_t)((theme_rgb6[i][1] * b) / 100u);
        uint8_t b6 = (uint8_t)((theme_rgb6[i][2] * b) / 100u);
        if (fb_bpp_mode == 8u) {
            palette_set((uint8_t)i, r6, g6, b6);
        }
        color_lut32[i] = rgb6_to_rgb32(r6, g6, b6);
    }
}

static void palette_init(void) {
    wevoa_ui_style_load_default_theme(theme_rgb6);
    apply_theme_brightness();
}

static bool ps2_wait_write(void) {
    for (uint32_t i = 0; i < 100000u; ++i) {
        if ((inb(0x64u) & 0x02u) == 0u) {
            return true;
        }
    }
    return false;
}

static bool ps2_wait_read(void) {
    for (uint32_t i = 0; i < 100000u; ++i) {
        if ((inb(0x64u) & 0x01u) != 0u) {
            return true;
        }
    }
    return false;
}

static void mouse_write_cmd(uint8_t cmd) {
    if (!ps2_wait_write()) {
        return;
    }
    outb(0x64u, 0xD4u);
    if (!ps2_wait_write()) {
        return;
    }
    outb(0x60u, cmd);
}

static uint8_t mouse_read_data(void) {
    if (!ps2_wait_read()) {
        return 0xFFu;
    }
    return inb(0x60u);
}

static void mouse_init(void) {
    while ((inb(0x64u) & 0x01u) != 0u) {
        (void)inb(0x60u);
    }

    if (ps2_wait_write()) {
        outb(0x64u, 0xA8u);
    }

    if (ps2_wait_write()) {
        outb(0x64u, 0x20u);
        uint8_t status = mouse_read_data();
        status = (uint8_t)(status | 0x02u);
        if (ps2_wait_write()) {
            outb(0x64u, 0x60u);
            if (ps2_wait_write()) {
                outb(0x60u, status);
            }
        }
    }

    mouse_write_cmd(0xF6u);
    (void)mouse_read_data();
    mouse_write_cmd(0xF4u);
    (void)mouse_read_data();

    state.mouse_index = 0;
    state.mouse_left_down = false;
    state.mouse_left_pressed = false;
    state.mouse_right_down = false;
    state.mouse_right_pressed = false;
}

static void put_px(int32_t x, int32_t y, uint8_t c) {
    if ((uint32_t)x >= fb_width || (uint32_t)y >= fb_height) {
        return;
    }
    draw_base[(uint32_t)y * draw_pitch + (uint32_t)x] = c;
}

static void fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t c) {
    if (w <= 0 || h <= 0) {
        return;
    }

    int32_t x0 = x;
    int32_t y0 = y;
    int32_t x1 = x + w;
    int32_t y1 = y + h;

    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 > (int32_t)fb_width) {
        x1 = (int32_t)fb_width;
    }
    if (y1 > (int32_t)fb_height) {
        y1 = (int32_t)fb_height;
    }

    if (x1 <= x0 || y1 <= y0) {
        return;
    }

    for (int32_t py = y0; py < y1; ++py) {
        uint32_t row = (uint32_t)py * draw_pitch;
        for (int32_t px = x0; px < x1; ++px) {
            draw_base[row + (uint32_t)px] = c;
        }
    }
}

static void stroke_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t c) {
    if (w <= 1 || h <= 1) {
        return;
    }
    fill_rect(x, y, w, 1, c);
    fill_rect(x, y + h - 1, w, 1, c);
    fill_rect(x, y, 1, h, c);
    fill_rect(x + w - 1, y, 1, h, c);
}

static void draw_glass_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t base, uint8_t hi, uint8_t lo) {
    wevoa_ui_style_draw_glass_rect(
        x, y, w, h, base, hi, lo, COLOR_BORDER, fill_rect, stroke_rect
    );
}

static const uint8_t font_digits_5x7[10][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}
};

static const uint8_t font_upper_5x7[26][7] = {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A},
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}
};

static const uint8_t font_lower_5x7[26][7] = {
    {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F},
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E},
    {0x00, 0x00, 0x0E, 0x10, 0x10, 0x10, 0x0E},
    {0x01, 0x01, 0x0F, 0x11, 0x11, 0x11, 0x0F},
    {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E},
    {0x06, 0x08, 0x1E, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E},
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x11},
    {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E},
    {0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C},
    {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12},
    {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x00, 0x00, 0x1A, 0x15, 0x15, 0x15, 0x15},
    {0x00, 0x00, 0x1E, 0x11, 0x11, 0x11, 0x11},
    {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E},
    {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10},
    {0x00, 0x00, 0x0F, 0x11, 0x0F, 0x01, 0x01},
    {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10},
    {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E},
    {0x08, 0x08, 0x1E, 0x08, 0x08, 0x08, 0x06},
    {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D},
    {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04},
    {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A},
    {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11},
    {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E},
    {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F}
};

static int32_t ui_text_scale(void) {
    return 1;
}

static uint8_t glyph_row_bits(char c, uint8_t row) {
    if (c >= 'A' && c <= 'Z') {
        return font_upper_5x7[(uint8_t)(c - 'A')][row];
    }
    if (c >= 'a' && c <= 'z') {
        return font_lower_5x7[(uint8_t)(c - 'a')][row];
    }
    if (c >= '0' && c <= '9') {
        return font_digits_5x7[(uint8_t)(c - '0')][row];
    }

    switch (c) {
        case ' ':
            return 0x00;
        case '.':
            return (row == 6u) ? 0x04 : 0x00;
        case '-':
            return (row == 3u) ? 0x0E : 0x00;
        case '_':
            return (row == 6u) ? 0x1F : 0x00;
        case '+':
            return (row == 3u) ? 0x0E : ((row >= 1u && row <= 5u) ? 0x04 : 0x00);
        case '*':
            return (uint8_t[]){0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00}[row];
        case '=':
            return (row == 2u || row == 4u) ? 0x1F : 0x00;
        case ':':
            return (row == 2u || row == 5u) ? 0x04 : 0x00;
        case '/':
            return (uint8_t[]){0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}[row];
        case ',':
            return (row >= 5u) ? 0x04 : 0x00;
        case ';':
            return (row == 2u || row >= 5u) ? 0x04 : 0x00;
        case '\'':
            return (row <= 2u) ? 0x04 : 0x00;
        case '"':
            return (uint8_t[]){0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00}[row];
        case '`':
            return (uint8_t[]){0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}[row];
        case '\\':
            return (uint8_t[]){0x10, 0x08, 0x08, 0x04, 0x02, 0x02, 0x01}[row];
        case '(':
            return (uint8_t[]){0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}[row];
        case ')':
            return (uint8_t[]){0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}[row];
        case '[':
            return (uint8_t[]){0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}[row];
        case ']':
            return (uint8_t[]){0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}[row];
        case '{':
            return (uint8_t[]){0x03, 0x04, 0x04, 0x18, 0x04, 0x04, 0x03}[row];
        case '}':
            return (uint8_t[]){0x18, 0x04, 0x04, 0x03, 0x04, 0x04, 0x18}[row];
        case '<':
            return (uint8_t[]){0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}[row];
        case '>':
            return (uint8_t[]){0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}[row];
        case '!':
            return (row == 6u) ? 0x04 : ((row <= 4u) ? 0x04 : 0x00);
        case '?':
            return (uint8_t[]){0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}[row];
        case '#':
            return (uint8_t[]){0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A}[row];
        case '$':
            return (uint8_t[]){0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}[row];
        case '%':
            return (uint8_t[]){0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03}[row];
        case '^':
            return (uint8_t[]){0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00}[row];
        case '&':
            return (uint8_t[]){0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D}[row];
        case '@':
            return (uint8_t[]){0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E}[row];
        case '|':
            return (uint8_t[]){0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}[row];
        case '~':
            return (uint8_t[]){0x00, 0x00, 0x09, 0x16, 0x00, 0x00, 0x00}[row];
        default:
            return 0x00;
    }
}

static void draw_char5x7(int32_t x, int32_t y, char c, uint8_t color, int32_t scale) {
    if (scale <= 0) {
        return;
    }

    for (uint8_t row = 0; row < 7u; ++row) {
        uint8_t bits = glyph_row_bits(c, row);
        for (uint8_t col = 0; col < 5u; ++col) {
            if ((bits & (1u << (4u - col))) == 0u) {
                continue;
            }
            fill_rect(x + (int32_t)col * scale, y + (int32_t)row * scale, scale, scale, color);
        }
    }
}

static void draw_text5x7(int32_t x, int32_t y, const char* text, uint8_t color, int32_t scale) {
    if (text == 0) {
        return;
    }

    int32_t cx = x;
    int32_t cy = y;
    while (*text != '\0') {
        char c = *text++;
        if (c == '\n') {
            cx = x;
            cy += 8 * scale;
            continue;
        }
        draw_char5x7(cx, cy, c, color, scale);
        cx += 6 * scale;
    }
}

static void draw_button(int32_t x, int32_t y, int32_t w, int32_t h, const char* label, bool active) {
    uint8_t bg = active ? COLOR_ACCENT : COLOR_PANEL_BG;
    uint8_t hi = active ? COLOR_TASKBAR_HI : COLOR_WINDOW_BG;
    uint8_t lo = active ? COLOR_BORDER : COLOR_TITLE_INACTIVE;
    draw_glass_rect(x, y, w, h, bg, hi, lo);
    int32_t tscale = ui_text_scale();
    int32_t label_w = (int32_t)text_len(label) * 6 * tscale;
    int32_t tx = x + (w - label_w) / 2;
    int32_t ty = y + (h - 7 * tscale) / 2;
    if (tx < x + 2) {
        tx = x + 2;
    }
    if (ty < y + 1) {
        ty = y + 1;
    }
    draw_text5x7(tx, ty, label, active ? COLOR_WINDOW_BG : COLOR_BORDER, tscale);
}

static void u8_to_dec_text(uint8_t value, char out[4]) {
    if (value >= 100u) {
        out[0] = '1';
        out[1] = (char)('0' + ((value - 100u) / 10u));
        out[2] = (char)('0' + ((value - 100u) % 10u));
        out[3] = '\0';
        return;
    }

    if (value >= 10u) {
        out[0] = (char)('0' + (value / 10u));
        out[1] = (char)('0' + (value % 10u));
        out[2] = '\0';
        out[3] = '\0';
        return;
    }

    out[0] = (char)('0' + value);
    out[1] = '\0';
    out[2] = '\0';
    out[3] = '\0';
}

static void u8_to_2digit_text(uint8_t value, char out[3]) {
    if (value > 99u) {
        value = (uint8_t)(value % 100u);
    }
    out[0] = (char)('0' + (value / 10u));
    out[1] = (char)('0' + (value % 10u));
    out[2] = '\0';
}

static uint8_t cmos_read_reg(uint8_t reg) {
    outb(0x70u, (uint8_t)(reg | 0x80u));
    return inb(0x71u);
}

static bool cmos_read_hour_minute(uint8_t* hour, uint8_t* minute) {
    if (hour == 0 || minute == 0) {
        return false;
    }

    for (uint32_t i = 0u; i < 50000u; ++i) {
        if ((cmos_read_reg(CMOS_STATUS_A) & 0x80u) == 0u) {
            break;
        }
    }

    uint8_t hh = cmos_read_reg(CMOS_HOUR);
    uint8_t mm = cmos_read_reg(CMOS_MINUTE);
    uint8_t reg_b = cmos_read_reg(CMOS_STATUS_B);

    if ((reg_b & 0x04u) == 0u) {
        hh = (uint8_t)((hh & 0x0Fu) + ((hh >> 4) * 10u));
        mm = (uint8_t)((mm & 0x0Fu) + ((mm >> 4) * 10u));
    }

    if ((reg_b & 0x02u) == 0u) {
        bool pm = (hh & 0x80u) != 0u;
        hh = (uint8_t)(hh & 0x7Fu);
        if (hh == 12u) {
            hh = 0u;
        }
        if (pm) {
            hh = (uint8_t)(hh + 12u);
        }
    }

    if (hh > 23u || mm > 59u) {
        return false;
    }

    *hour = hh;
    *minute = mm;
    return true;
}

static bool cmos_read_day_month(uint8_t* day, uint8_t* month) {
    if (day == 0 || month == 0) {
        return false;
    }

    for (uint32_t i = 0u; i < 50000u; ++i) {
        if ((cmos_read_reg(CMOS_STATUS_A) & 0x80u) == 0u) {
            break;
        }
    }

    uint8_t dd = cmos_read_reg(CMOS_DAY);
    uint8_t mo = cmos_read_reg(CMOS_MONTH);
    uint8_t reg_b = cmos_read_reg(CMOS_STATUS_B);

    if ((reg_b & 0x04u) == 0u) {
        dd = (uint8_t)((dd & 0x0Fu) + ((dd >> 4) * 10u));
        mo = (uint8_t)((mo & 0x0Fu) + ((mo >> 4) * 10u));
    }

    if (dd == 0u || dd > 31u || mo == 0u || mo > 12u) {
        return false;
    }

    *day = dd;
    *month = mo;
    return true;
}

static void u32_to_dec_text(uint32_t value, char* out, uint32_t cap) {
    if (out == 0 || cap == 0u) {
        return;
    }
    if (value == 0u) {
        if (cap > 1u) {
            out[0] = '0';
            out[1] = '\0';
        } else {
            out[0] = '\0';
        }
        return;
    }

    char tmp[11];
    uint32_t n = 0u;
    while (value > 0u && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    uint32_t j = 0u;
    while (n > 0u && j + 1u < cap) {
        out[j++] = tmp[--n];
    }
    out[j] = '\0';
}

static void ipv4_to_text(const uint8_t ip[4], char out[20]) {
    char part[4];
    if (ip == 0 || out == 0) {
        return;
    }
    out[0] = '\0';
    for (uint32_t i = 0u; i < 4u; ++i) {
        u8_to_dec_text(ip[i], part);
        text_append(out, 20u, part);
        if (i + 1u < 4u) {
            text_append(out, 20u, ".");
        }
    }
}

static char scancode_to_char(uint8_t scancode, bool shift) {
    switch (scancode) {
        case SC_A: return shift ? 'A' : 'a';
        case SC_B: return shift ? 'B' : 'b';
        case SC_C: return shift ? 'C' : 'c';
        case SC_D: return shift ? 'D' : 'd';
        case SC_E: return shift ? 'E' : 'e';
        case SC_F: return shift ? 'F' : 'f';
        case SC_G: return shift ? 'G' : 'g';
        case SC_H: return shift ? 'H' : 'h';
        case SC_I: return shift ? 'I' : 'i';
        case SC_J: return shift ? 'J' : 'j';
        case SC_K: return shift ? 'K' : 'k';
        case SC_L: return shift ? 'L' : 'l';
        case SC_M: return shift ? 'M' : 'm';
        case SC_N: return shift ? 'N' : 'n';
        case SC_O: return shift ? 'O' : 'o';
        case SC_P: return shift ? 'P' : 'p';
        case SC_Q: return shift ? 'Q' : 'q';
        case SC_R: return shift ? 'R' : 'r';
        case SC_S: return shift ? 'S' : 's';
        case SC_T: return shift ? 'T' : 't';
        case SC_U: return shift ? 'U' : 'u';
        case SC_V: return shift ? 'V' : 'v';
        case SC_W: return shift ? 'W' : 'w';
        case SC_X: return shift ? 'X' : 'x';
        case SC_Y: return shift ? 'Y' : 'y';
        case SC_Z: return shift ? 'Z' : 'z';
        case SC_1: return shift ? '!' : '1';
        case SC_2: return shift ? '@' : '2';
        case SC_3: return shift ? '#' : '3';
        case SC_4: return shift ? '$' : '4';
        case SC_5: return shift ? '%' : '5';
        case SC_6: return shift ? '^' : '6';
        case SC_7: return shift ? '&' : '7';
        case SC_8: return shift ? '*' : '8';
        case SC_9: return shift ? '(' : '9';
        case SC_0: return shift ? ')' : '0';
        case SC_SPACE: return ' ';
        case SC_DOT: return shift ? '>' : '.';
        case SC_COMMA: return shift ? '<' : ',';
        case SC_MINUS: return shift ? '_' : '-';
        case SC_EQUAL: return shift ? '+' : '=';
        case SC_SLASH: return shift ? '?' : '/';
        case SC_SEMICOLON: return shift ? ':' : ';';
        case SC_APOSTROPHE: return shift ? '"' : '\'';
        case SC_LBRACKET: return shift ? '{' : '[';
        case SC_RBRACKET: return shift ? '}' : ']';
        case SC_BACKSLASH: return shift ? '|' : '\\';
        case SC_GRAVE: return shift ? '~' : '`';
        default: return '\0';
    }
}

static void terminal_join_args(char* out, uint32_t cap, uint8_t argc, char* argv[8], uint8_t start) {
    out[0] = '\0';
    for (uint8_t i = start; i < argc; ++i) {
        if (i != start) {
            text_append(out, cap, " ");
        }
        text_append(out, cap, argv[i]);
    }
}

static void terminal_print_path(void) {
    char path[TERM_LINE_MAX];
    char line[TERM_LINE_MAX];
    vfs_build_path(terminal_state.cwd_node, path);
    text_copy(line, TERM_LINE_MAX, "PWD ");
    text_append(line, TERM_LINE_MAX, path);
    terminal_push_line(line);
}

static int terminal_cmd_ls(void) {
    bool any = false;
    for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
        if (!vfs_nodes[i].used || vfs_nodes[i].parent != terminal_state.cwd_node) {
            continue;
        }
        char line[TERM_LINE_MAX];
        text_copy(line, TERM_LINE_MAX, vfs_nodes[i].type == VFS_TYPE_DIR ? "DIR  " : "FILE ");
        text_append(line, TERM_LINE_MAX, vfs_nodes[i].name);
        terminal_push_line(line);
        any = true;
    }
    if (!any) {
        terminal_push_line("EMPTY");
    }
    return W_OK;
}

static int terminal_cmd_cd(uint8_t argc, char* argv[8]) {
    if (argc < 2u) {
        terminal_print_path();
        return W_OK;
    }

    if (text_equal(argv[1], "/")) {
        terminal_state.cwd_node = 0u;
        mark_pstore_dirty();
        return W_OK;
    }
    if (text_equal(argv[1], "..")) {
        uint8_t p = vfs_nodes[terminal_state.cwd_node].parent;
        if (p != VFS_INVALID_NODE) {
            terminal_state.cwd_node = p;
        }
        mark_pstore_dirty();
        return W_OK;
    }

    uint8_t node = vfs_find_child(terminal_state.cwd_node, argv[1]);
    if (node == VFS_INVALID_NODE || vfs_nodes[node].type != VFS_TYPE_DIR) {
        return W_ERR_NOT_FOUND;
    }
    terminal_state.cwd_node = node;
    mark_pstore_dirty();
    return W_OK;
}

static int terminal_cmd_mkdir(uint8_t argc, char* argv[8]) {
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    if (vfs_find_child(terminal_state.cwd_node, argv[1]) != VFS_INVALID_NODE) {
        return W_ERR_EXISTS;
    }
    uint8_t node = vfs_add_node(terminal_state.cwd_node, VFS_TYPE_DIR, argv[1], 0);
    if (node == VFS_INVALID_NODE) {
        return W_ERR_NO_SPACE;
    }
    terminal_push_ok("DIR CREATED");
    if (terminal_state.cwd_node == desktop_root_node) {
        sync_desktop_items_from_vfs();
    }
    mark_pstore_dirty();
    return W_OK;
}

static int terminal_cmd_touch(uint8_t argc, char* argv[8]) {
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    if (vfs_find_child(terminal_state.cwd_node, argv[1]) != VFS_INVALID_NODE) {
        return W_ERR_EXISTS;
    }
    uint8_t node = vfs_add_node(terminal_state.cwd_node, VFS_TYPE_FILE, argv[1], "");
    if (node == VFS_INVALID_NODE) {
        return W_ERR_NO_SPACE;
    }
    terminal_push_ok("FILE CREATED");
    if (terminal_state.cwd_node == desktop_root_node) {
        sync_desktop_items_from_vfs();
    }
    mark_pstore_dirty();
    return W_OK;
}

static int terminal_cmd_cat(uint8_t argc, char* argv[8]) {
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    uint8_t node = vfs_find_child(terminal_state.cwd_node, argv[1]);
    if (node == VFS_INVALID_NODE || vfs_nodes[node].type != VFS_TYPE_FILE) {
        return W_ERR_NOT_FOUND;
    }
    if (vfs_nodes[node].content[0] == '\0') {
        terminal_push_line("EMPTY FILE");
        return W_OK;
    }
    terminal_push_line(vfs_nodes[node].content);
    return W_OK;
}

static int terminal_cmd_write(uint8_t argc, char* argv[8]) {
    if (argc < 3u) {
        return W_ERR_INVALID_ARG;
    }
    uint8_t node = vfs_find_child(terminal_state.cwd_node, argv[1]);
    if (node == VFS_INVALID_NODE || vfs_nodes[node].type != VFS_TYPE_FILE) {
        return W_ERR_NOT_FOUND;
    }

    char payload[VFS_CONTENT_MAX];
    terminal_join_args(payload, VFS_CONTENT_MAX, argc, argv, 2u);
    text_copy(vfs_nodes[node].content, VFS_CONTENT_MAX, payload);
    terminal_push_ok("WRITE");
    mark_pstore_dirty();
    return W_OK;
}

static int terminal_cmd_edit(uint8_t argc, char* argv[8]) {
    if (argc < 3u) {
        return W_ERR_INVALID_ARG;
    }
    return terminal_cmd_write(argc, argv);
}

static int terminal_cmd_append(uint8_t argc, char* argv[8]) {
    if (argc < 3u) {
        return W_ERR_INVALID_ARG;
    }
    uint8_t node = vfs_find_child(terminal_state.cwd_node, argv[1]);
    if (node == VFS_INVALID_NODE || vfs_nodes[node].type != VFS_TYPE_FILE) {
        return W_ERR_NOT_FOUND;
    }

    char payload[VFS_CONTENT_MAX];
    text_copy(payload, VFS_CONTENT_MAX, vfs_nodes[node].content);
    if (payload[0] != '\0') {
        text_append(payload, VFS_CONTENT_MAX, " ");
    }
    char extra[VFS_CONTENT_MAX];
    terminal_join_args(extra, VFS_CONTENT_MAX, argc, argv, 2u);
    text_append(payload, VFS_CONTENT_MAX, extra);
    text_copy(vfs_nodes[node].content, VFS_CONTENT_MAX, payload);
    terminal_push_ok("APPEND");
    mark_pstore_dirty();
    return W_OK;
}

static int terminal_cmd_copy(uint8_t argc, char* argv[8]) {
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    uint8_t node = vfs_find_child(terminal_state.cwd_node, argv[1]);
    if (node == VFS_INVALID_NODE || !vfs_nodes[node].used) {
        return W_ERR_NOT_FOUND;
    }
    if (node == 0u || node == waste_bin_node) {
        return W_ERR_UNSUPPORTED;
    }
    vfs_clipboard.has_data = true;
    vfs_clipboard.cut_mode = false;
    vfs_clipboard.node = node;
    text_copy(vfs_clipboard.name, VFS_NAME_MAX, vfs_nodes[node].name);
    terminal_push_ok("COPIED");
    return W_OK;
}

static int terminal_cmd_cut(uint8_t argc, char* argv[8]) {
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    uint8_t node = vfs_find_child(terminal_state.cwd_node, argv[1]);
    if (node == VFS_INVALID_NODE || !vfs_nodes[node].used) {
        return W_ERR_NOT_FOUND;
    }
    if (node == 0u || node == waste_bin_node || node == desktop_root_node) {
        return W_ERR_UNSUPPORTED;
    }
    vfs_clipboard.has_data = true;
    vfs_clipboard.cut_mode = true;
    vfs_clipboard.node = node;
    text_copy(vfs_clipboard.name, VFS_NAME_MAX, vfs_nodes[node].name);
    terminal_push_ok("CUT");
    return W_OK;
}

static int terminal_cmd_paste(uint8_t argc, char* argv[8]) {
    if (!vfs_clipboard.has_data || vfs_clipboard.node == VFS_INVALID_NODE) {
        return W_ERR_NOT_FOUND;
    }
    if (vfs_clipboard.node >= VFS_MAX_NODES || !vfs_nodes[vfs_clipboard.node].used) {
        vfs_clipboard.has_data = false;
        return W_ERR_NOT_FOUND;
    }

    char target_name[VFS_NAME_MAX];
    if (argc >= 2u) {
        text_copy(target_name, VFS_NAME_MAX, argv[1]);
        text_upper_inplace(target_name);
    } else {
        text_copy(target_name, VFS_NAME_MAX, vfs_nodes[vfs_clipboard.node].name);
    }
    if (target_name[0] == '\0') {
        return W_ERR_INVALID_ARG;
    }

    if (vfs_clipboard.cut_mode) {
        if (vfs_is_descendant(terminal_state.cwd_node, vfs_clipboard.node)) {
            return W_ERR_INVALID_ARG;
        }
        if (vfs_find_child(terminal_state.cwd_node, target_name) != VFS_INVALID_NODE &&
            !text_equal(target_name, vfs_nodes[vfs_clipboard.node].name)) {
            return W_ERR_EXISTS;
        }
        vfs_nodes[vfs_clipboard.node].parent = terminal_state.cwd_node;
        text_copy(vfs_nodes[vfs_clipboard.node].name, VFS_NAME_MAX, target_name);
        vfs_clipboard.has_data = false;
        vfs_clipboard.cut_mode = false;
        vfs_clipboard.node = VFS_INVALID_NODE;
        vfs_clipboard.name[0] = '\0';
        terminal_push_ok("MOVED");
        mark_pstore_dirty();
        if (terminal_state.cwd_node == desktop_root_node) {
            sync_desktop_items_from_vfs();
            build_desktop_template();
        }
        return W_OK;
    }

    if (vfs_find_child(terminal_state.cwd_node, target_name) != VFS_INVALID_NODE) {
        return W_ERR_EXISTS;
    }

    uint8_t created = vfs_clone_subtree(vfs_clipboard.node, terminal_state.cwd_node, target_name);
    if (created == VFS_INVALID_NODE) {
        return W_ERR_NO_SPACE;
    }
    terminal_push_ok("PASTED");
    mark_pstore_dirty();
    if (terminal_state.cwd_node == desktop_root_node) {
        sync_desktop_items_from_vfs();
        build_desktop_template();
    }
    return W_OK;
}

static int terminal_cmd_delete(uint8_t argc, char* argv[8]) {
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    uint8_t node = vfs_find_child(terminal_state.cwd_node, argv[1]);
    if (node == VFS_INVALID_NODE || !vfs_nodes[node].used) {
        return W_ERR_NOT_FOUND;
    }
    if (node == 0u || node == desktop_root_node || node == waste_bin_node) {
        return W_ERR_UNSUPPORTED;
    }
    if (vfs_nodes[node].parent == waste_bin_node) {
        vfs_clear_subtree(node);
        terminal_push_ok("PERMANENT DELETE");
        mark_pstore_dirty();
        return W_OK;
    }
    int rc = vfs_move_to_waste_bin(node);
    if (rc != W_OK) {
        return rc;
    }
    terminal_push_ok("MOVED TO WASTEBIN");
    mark_pstore_dirty();
    if (terminal_state.cwd_node == desktop_root_node) {
        sync_desktop_items_from_vfs();
        build_desktop_template();
    }
    return W_OK;
}

static int terminal_cmd_wastebin_list(void) {
    uint8_t wb = vfs_ensure_waste_bin();
    if (wb == VFS_INVALID_NODE) {
        return W_ERR_NO_SPACE;
    }
    bool any = false;
    for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
        if (!vfs_nodes[i].used || vfs_nodes[i].parent != wb) {
            continue;
        }
        char line[TERM_LINE_MAX];
        text_copy(line, TERM_LINE_MAX, vfs_nodes[i].type == VFS_TYPE_DIR ? "BIN DIR  " : "BIN FILE ");
        text_append(line, TERM_LINE_MAX, vfs_nodes[i].name);
        terminal_push_line(line);
        any = true;
    }
    if (!any) {
        terminal_push_line("WASTEBIN EMPTY");
    }
    return W_OK;
}

static int terminal_cmd_restore(uint8_t argc, char* argv[8]) {
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    uint8_t wb = vfs_ensure_waste_bin();
    if (wb == VFS_INVALID_NODE) {
        return W_ERR_NO_SPACE;
    }
    uint8_t node = vfs_find_child(wb, argv[1]);
    if (node == VFS_INVALID_NODE) {
        return W_ERR_NOT_FOUND;
    }
    const char* override_name = (argc >= 3u) ? argv[2] : 0;
    int rc = vfs_restore_from_waste_bin(node, override_name);
    if (rc != W_OK) {
        return rc;
    }
    terminal_push_ok("RESTORED");
    mark_pstore_dirty();
    if (terminal_state.cwd_node == desktop_root_node) {
        sync_desktop_items_from_vfs();
        build_desktop_template();
    }
    return W_OK;
}

static int terminal_cmd_emptybin(void) {
    uint8_t wb = vfs_ensure_waste_bin();
    if (wb == VFS_INVALID_NODE) {
        return W_ERR_NO_SPACE;
    }
    for (uint32_t i = 0u; i < VFS_MAX_NODES; ++i) {
        if (!vfs_nodes[i].used || vfs_nodes[i].parent != wb) {
            continue;
        }
        vfs_clear_subtree((uint8_t)i);
    }
    terminal_push_ok("WASTEBIN CLEARED");
    mark_pstore_dirty();
    return W_OK;
}

static int terminal_cmd_desk_ls(void) {
    if (desktop_item_count == 0u) {
        terminal_push_line("DESKTOP EMPTY");
        return W_OK;
    }
    for (uint32_t i = 0u; i < desktop_item_count; ++i) {
        char line[TERM_LINE_MAX];
        text_copy(line, TERM_LINE_MAX, desktop_items[i].is_dir ? "DESK DIR  " : "DESK FILE ");
        text_append(line, TERM_LINE_MAX, desktop_items[i].name);
        terminal_push_line(line);
    }
    return W_OK;
}

static int terminal_cmd_desk_create(uint8_t argc, char* argv[8], bool is_dir) {
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    if (vfs_find_child(desktop_root_node, argv[1]) != VFS_INVALID_NODE) {
        return W_ERR_EXISTS;
    }
    uint8_t node = vfs_add_node(desktop_root_node, is_dir ? VFS_TYPE_DIR : VFS_TYPE_FILE, argv[1], is_dir ? 0 : "WEVOA FILE");
    if (node == VFS_INVALID_NODE) {
        return W_ERR_NO_SPACE;
    }
    sync_desktop_items_from_vfs();
    build_desktop_template();
    terminal_push_ok(is_dir ? "DESKTOP DIR CREATED" : "DESKTOP FILE CREATED");
    mark_pstore_dirty();
    return W_OK;
}

static int terminal_cmd_wget(uint8_t argc, char* argv[8]) {
    char line[TERM_LINE_MAX];
    uint32_t v = 0u;

    if (argc >= 2u) {
        if (text_equal(argv[1], "SCALE")) {
            char n[12];
            u32_to_dec_text((uint32_t)desktop_scale(), n, sizeof(n));
            text_copy(line, TERM_LINE_MAX, "SCALE=");
            text_append(line, TERM_LINE_MAX, n);
            terminal_push_line(line);
            return W_OK;
        }
        if (text_equal(argv[1], "RES") || text_equal(argv[1], "RESOLUTION")) {
            char rx[12];
            char ry[12];
            u32_to_dec_text(fb_width, rx, sizeof(rx));
            u32_to_dec_text(fb_height, ry, sizeof(ry));
            text_copy(line, TERM_LINE_MAX, "RES=");
            text_append(line, TERM_LINE_MAX, rx);
            text_append(line, TERM_LINE_MAX, "X");
            text_append(line, TERM_LINE_MAX, ry);
            terminal_push_line(line);
            return W_OK;
        }
        enum wevoa_config_key key = WEVOA_CFG_BRIGHT;
        int rc = config_key_from_name(argv[1], &key);
        if (rc != W_OK) {
            return rc;
        }
        rc = config_get(key, &v);
        if (rc != W_OK) {
            return rc;
        }
        if (key == WEVOA_CFG_SHELL_LAYOUT) {
            text_copy(line, TERM_LINE_MAX, "SHELL_LAYOUT=");
            text_append(line, TERM_LINE_MAX, (v == SHELL_LAYOUT_TASKBAR) ? "TASKBAR" : "DOCK");
            terminal_push_line(line);
            return W_OK;
        }
        if (key == WEVOA_CFG_CURSOR_SPEED) {
            v *= 10u;
        }
        char n[12];
        u32_to_dec_text(v, n, sizeof(n));
        text_copy(line, TERM_LINE_MAX, argv[1]);
        text_append(line, TERM_LINE_MAX, "=");
        text_append(line, TERM_LINE_MAX, n);
        terminal_push_line(line);
        return W_OK;
    }

    text_copy(line, TERM_LINE_MAX, "BRIGHT ");
    char n[6];
    if (config_get(WEVOA_CFG_BRIGHT, &v) != W_OK) {
        return W_ERR_IO;
    }
    u8_to_dec_text((uint8_t)v, n);
    text_append(line, TERM_LINE_MAX, n);
    terminal_push_line(line);

    text_copy(line, TERM_LINE_MAX, "WALL ");
    char w[6];
    if (config_get(WEVOA_CFG_WALL, &v) != W_OK) {
        return W_ERR_IO;
    }
    u8_to_dec_text((uint8_t)(v % WALLPAPER_COUNT), w);
    text_append(line, TERM_LINE_MAX, w);
    terminal_push_line(line);

    if (config_get(WEVOA_CFG_WIFI, &v) != W_OK) {
        return W_ERR_IO;
    }
    terminal_push_line(v ? "WIFI ON" : "WIFI OFF");

    if (config_get(WEVOA_CFG_NET_LINK, &v) != W_OK) {
        return W_ERR_IO;
    }
    terminal_push_line(v ? "NET CONNECTED" : "NET OFFLINE");

    text_copy(line, TERM_LINE_MAX, "CURSOR_SPEED ");
    if (config_get(WEVOA_CFG_CURSOR_SPEED, &v) != W_OK) {
        return W_ERR_IO;
    }
    char cursor_txt[12];
    u32_to_dec_text(v * 10u, cursor_txt, sizeof(cursor_txt));
    text_append(line, TERM_LINE_MAX, cursor_txt);
    terminal_push_line(line);

    if (config_get(WEVOA_CFG_LOCK_ENABLED, &v) != W_OK) {
        return W_ERR_IO;
    }
    terminal_push_line(v ? "LOCK ON" : "LOCK OFF");

    text_copy(line, TERM_LINE_MAX, "LOCK_PIN ");
    if (config_get(WEVOA_CFG_LOCK_PIN, &v) != W_OK) {
        return W_ERR_IO;
    }
    char pin_txt[12];
    u32_to_dec_text(v, pin_txt, sizeof(pin_txt));
    text_append(line, TERM_LINE_MAX, pin_txt);
    terminal_push_line(line);

    if (config_get(WEVOA_CFG_SHELL_LAYOUT, &v) != W_OK) {
        return W_ERR_IO;
    }
    terminal_push_line(v == SHELL_LAYOUT_TASKBAR ? "SHELL_LAYOUT TASKBAR" : "SHELL_LAYOUT DOCK");

    text_copy(line, TERM_LINE_MAX, "UI_BLUR_LEVEL ");
    if (config_get(WEVOA_CFG_UI_BLUR_LEVEL, &v) != W_OK) {
        return W_ERR_IO;
    }
    u8_to_dec_text((uint8_t)v, n);
    text_append(line, TERM_LINE_MAX, n);
    terminal_push_line(line);

    text_copy(line, TERM_LINE_MAX, "UI_MOTION_LEVEL ");
    if (config_get(WEVOA_CFG_UI_MOTION_LEVEL, &v) != W_OK) {
        return W_ERR_IO;
    }
    u8_to_dec_text((uint8_t)v, n);
    text_append(line, TERM_LINE_MAX, n);
    terminal_push_line(line);

    text_copy(line, TERM_LINE_MAX, "UI_EFFECTS_QUALITY ");
    if (config_get(WEVOA_CFG_UI_EFFECTS_QUALITY, &v) != W_OK) {
        return W_ERR_IO;
    }
    u8_to_dec_text((uint8_t)v, n);
    text_append(line, TERM_LINE_MAX, n);
    terminal_push_line(line);

    char nscale[12];
    u32_to_dec_text((uint32_t)desktop_scale(), nscale, sizeof(nscale));
    text_copy(line, TERM_LINE_MAX, "SCALE ");
    text_append(line, TERM_LINE_MAX, nscale);
    terminal_push_line(line);

    char rx[12];
    char ry[12];
    u32_to_dec_text(fb_width, rx, sizeof(rx));
    u32_to_dec_text(fb_height, ry, sizeof(ry));
    text_copy(line, TERM_LINE_MAX, "RES ");
    text_append(line, TERM_LINE_MAX, rx);
    text_append(line, TERM_LINE_MAX, "X");
    text_append(line, TERM_LINE_MAX, ry);
    terminal_push_line(line);
    return W_OK;
}

static int terminal_cmd_wset(uint8_t argc, char* argv[8]) {
    if (argc < 3u) {
        return W_ERR_INVALID_ARG;
    }

    if (text_equal(argv[1], "SCALE")) {
        uint32_t value = 0u;
        if (!parse_u32(argv[2], &value)) {
            return W_ERR_INVALID_ARG;
        }
        if (value > 3u) {
            return W_ERR_INVALID_ARG;
        }
        ui_scale_override = (uint8_t)value;
        mark_pstore_dirty();
        build_desktop_template();
        terminal_push_ok("SCALE UPDATED");
        return W_OK;
    }

    enum wevoa_config_key key = WEVOA_CFG_BRIGHT;
    int rc = config_key_from_name(argv[1], &key);
    if (rc != W_OK) {
        return rc;
    }

    uint32_t value = 0u;
    if (key == WEVOA_CFG_WIFI || key == WEVOA_CFG_NET_LINK || key == WEVOA_CFG_LOCK_ENABLED) {
        if (text_equal(argv[2], "ON")) {
            value = 1u;
        } else if (text_equal(argv[2], "OFF")) {
            value = 0u;
        } else {
            return W_ERR_INVALID_ARG;
        }
    } else if (key == WEVOA_CFG_SHELL_LAYOUT) {
        if (text_equal(argv[2], "DOCK")) {
            value = SHELL_LAYOUT_DOCK;
        } else if (text_equal(argv[2], "TASKBAR") || text_equal(argv[2], "BAR")) {
            value = SHELL_LAYOUT_TASKBAR;
        } else if (!parse_u32(argv[2], &value) || value > SHELL_LAYOUT_TASKBAR) {
            return W_ERR_INVALID_ARG;
        }
    } else {
        if (!parse_u32(argv[2], &value)) {
            return W_ERR_INVALID_ARG;
        }
        if (key == WEVOA_CFG_CURSOR_SPEED) {
            if (value >= 400u && value <= 2400u) {
                value /= 10u;
            } else if (value < 40u || value > 240u) {
                return W_ERR_INVALID_ARG;
            }
        }
    }

    rc = config_set(key, value);
    if (rc != W_OK) {
        return rc;
    }

    state_sync_from_config();
    if (key == WEVOA_CFG_BRIGHT) {
        apply_theme_brightness();
    }
    if (key == WEVOA_CFG_WALL) {
        build_desktop_template();
    }
    if (key == WEVOA_CFG_SHELL_LAYOUT || key == WEVOA_CFG_UI_BLUR_LEVEL || key == WEVOA_CFG_UI_EFFECTS_QUALITY) {
        build_desktop_template();
    }
    mark_pstore_dirty();
    terminal_push_ok("CONFIG UPDATED");
    return W_OK;
}

static int terminal_cmd_wlist(void) {
    struct {
        const char* name;
        enum wevoa_config_key key;
    } rows[] = {
        {"BRIGHT", WEVOA_CFG_BRIGHT},
        {"WALL", WEVOA_CFG_WALL},
        {"WIFI", WEVOA_CFG_WIFI},
        {"NET", WEVOA_CFG_NET_LINK},
        {"CURSOR_SPEED", WEVOA_CFG_CURSOR_SPEED},
        {"KEY_REPEAT_DELAY", WEVOA_CFG_KEY_REPEAT_DELAY},
        {"KEY_REPEAT_RATE", WEVOA_CFG_KEY_REPEAT_RATE},
        {"LOCK", WEVOA_CFG_LOCK_ENABLED},
        {"LOCK_PIN", WEVOA_CFG_LOCK_PIN},
        {"SHELL_LAYOUT", WEVOA_CFG_SHELL_LAYOUT},
        {"UI_BLUR_LEVEL", WEVOA_CFG_UI_BLUR_LEVEL},
        {"UI_MOTION_LEVEL", WEVOA_CFG_UI_MOTION_LEVEL},
        {"UI_EFFECTS_QUALITY", WEVOA_CFG_UI_EFFECTS_QUALITY},
    };

    for (uint32_t i = 0u; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        uint32_t value = 0u;
        int rc = config_get(rows[i].key, &value);
        if (rc != W_OK) {
            return rc;
        }
        if (rows[i].key == WEVOA_CFG_CURSOR_SPEED) {
            value *= 10u;
        }
        if (rows[i].key == WEVOA_CFG_SHELL_LAYOUT) {
            char line[TERM_LINE_MAX];
            text_copy(line, TERM_LINE_MAX, rows[i].name);
            text_append(line, TERM_LINE_MAX, "=");
            text_append(line, TERM_LINE_MAX, value == SHELL_LAYOUT_TASKBAR ? "TASKBAR" : "DOCK");
            terminal_push_line(line);
            continue;
        }
        char line[TERM_LINE_MAX];
        char n[12];
        u32_to_dec_text(value, n, sizeof(n));
        text_copy(line, TERM_LINE_MAX, rows[i].name);
        text_append(line, TERM_LINE_MAX, "=");
        text_append(line, TERM_LINE_MAX, n);
        terminal_push_line(line);
    }

    char scale_line[TERM_LINE_MAX];
    char n[12];
    u32_to_dec_text((uint32_t)desktop_scale(), n, sizeof(n));
    text_copy(scale_line, TERM_LINE_MAX, "SCALE=");
    text_append(scale_line, TERM_LINE_MAX, n);
    terminal_push_line(scale_line);

    char rx[12];
    char ry[12];
    char res_line[TERM_LINE_MAX];
    u32_to_dec_text(fb_width, rx, sizeof(rx));
    u32_to_dec_text(fb_height, ry, sizeof(ry));
    text_copy(res_line, TERM_LINE_MAX, "RES=");
    text_append(res_line, TERM_LINE_MAX, rx);
    text_append(res_line, TERM_LINE_MAX, "X");
    text_append(res_line, TERM_LINE_MAX, ry);
    terminal_push_line(res_line);
    return W_OK;
}

static int terminal_cmd_wsave(void) {
    int rc = pstore_save_runtime();
    if (rc != W_OK) {
        return rc;
    }
    pstore_dirty = false;
    terminal_push_ok("STORE SAVED");
    return W_OK;
}

static int terminal_cmd_wload(void) {
    int rc = pstore_load_runtime();
    if (rc == W_ERR_NOT_FOUND) {
        return W_ERR_NOT_FOUND;
    }
    if (rc != W_OK) {
        return rc;
    }
    apply_theme_brightness();
    build_desktop_template();
    pstore_dirty = false;
    terminal_push_ok("STORE LOADED");
    return W_OK;
}

static int terminal_cmd_wreset(void) {
    int rc = pstore_reset();
    if (rc != W_OK) {
        return rc;
    }
    reset_runtime_data_defaults();
    apply_theme_brightness();
    build_desktop_template();
    pstore_dirty = false;
    terminal_push_ok("STORE RESET");
    return W_OK;
}

static int terminal_cmd_dns(uint8_t argc, char* argv[8]) {
    uint8_t ip[4];
    char line[TERM_LINE_MAX];
    char ip_text[20];
    int rc;
    if (argc < 2u) {
        return W_ERR_INVALID_ARG;
    }
    rc = net_dns_resolve(argv[1], ip);
    if (rc != W_OK) {
        return rc;
    }
    ipv4_to_text(ip, ip_text);
    text_copy(line, TERM_LINE_MAX, "DNS ");
    text_append(line, TERM_LINE_MAX, argv[1]);
    text_append(line, TERM_LINE_MAX, " = ");
    text_append(line, TERM_LINE_MAX, ip_text);
    terminal_push_line(line);
    return W_OK;
}

static int terminal_cmd_port(uint8_t argc, char* argv[8]) {
    uint32_t p = 0u;
    uint16_t port = 0u;
    uint8_t allow = 0u;
    char n[12];
    char line[TERM_LINE_MAX];
    int rc;

    if (argc < 3u) {
        return W_ERR_INVALID_ARG;
    }
    if (!parse_u32(argv[1], &p) || p == 0u || p > 65535u) {
        return W_ERR_INVALID_ARG;
    }
    port = (uint16_t)p;

    if (text_equal(argv[2], "STATUS")) {
        rc = net_firewall_get_port(port, &allow);
        if (rc != W_OK) {
            return rc;
        }
    } else {
        if (text_equal(argv[2], "ALLOW")) {
            allow = 1u;
        } else if (text_equal(argv[2], "BLOCK") || text_equal(argv[2], "DENY")) {
            allow = 0u;
        } else {
            return W_ERR_INVALID_ARG;
        }
        rc = net_firewall_set_port(port, allow);
        if (rc != W_OK) {
            return rc;
        }
    }

    text_copy(line, TERM_LINE_MAX, "PORT ");
    u32_to_dec_text((uint32_t)port, n, sizeof(n));
    text_append(line, TERM_LINE_MAX, n);
    text_append(line, TERM_LINE_MAX, allow ? " ALLOW" : " BLOCK");
    terminal_push_line(line);
    return W_OK;
}

static int terminal_cmd_net_link(uint8_t up) {
    int rc;
    if (up) {
        rc = config_set(WEVOA_CFG_WIFI, 1u);
        if (rc != W_OK) {
            return rc;
        }
        rc = config_set(WEVOA_CFG_NET_LINK, 1u);
        if (rc != W_OK) {
            return rc;
        }
        state_sync_from_config();
        mark_pstore_dirty();
        terminal_push_ok("NETWORK ONLINE");
        return W_OK;
    }

    rc = config_set(WEVOA_CFG_NET_LINK, 0u);
    if (rc != W_OK) {
        return rc;
    }
    state_sync_from_config();
    mark_pstore_dirty();
    terminal_push_ok("NETWORK OFFLINE");
    return W_OK;
}

static void terminal_report_status(int rc) {
    if (rc == W_OK) {
        return;
    }
    terminal_push_err(wstatus_str(rc));
}

static void terminal_execute_command(const char* input) {
    char cmd_line[TERM_LINE_MAX];
    char work[TERM_INPUT_MAX];
    char* argv[8] = {0};
    int rc = W_OK;

    text_copy(cmd_line, TERM_LINE_MAX, "CMD ");
    text_append(cmd_line, TERM_LINE_MAX, input);
    terminal_push_line(cmd_line);

    text_copy(work, TERM_INPUT_MAX, input);
    text_upper_inplace(work);
    uint8_t argc = split_tokens(work, argv);
    if (argc == 0u) {
        return;
    }

    if (text_equal(argv[0], "HELP") || text_equal(argv[0], "WHELP")) {
        terminal_push_line("WEVOA SEC OPS COMMANDS");
        terminal_push_line("CORE: LS DIR CD PWD MKDIR MD");
        terminal_push_line("FILE: TOUCH CAT TYPE WRITE EDIT APPEND");
        terminal_push_line("OPS: COPY CUT PASTE DEL RESTORE BIN EMPTYBIN");
        terminal_push_line("DESK: DESKLS DESKDIR DESKFILE WALL");
        terminal_push_line("TOOLS: CODE NOTE SCAN LOGS PACKET RECON WEB LOCK");
        terminal_push_line("SYS: WEVOA WVER WHOAMI CLS LOCK");
        terminal_push_line("NET: DNS <HOST> PORT <P> ALLOW/BLOCK/STATUS");
        terminal_push_line("NET: NETUP NETDOWN WEB <URL>");
        terminal_push_line("CFG: WGET WSET WLIST (SCALE 0-3 LOCK LOCK_PIN SHELL BLUR MOTION QUALITY)");
        terminal_push_line("PERSIST: WSAVE WLOAD WRESET");
        terminal_push_line("ETHICAL USE ONLY: AUTHORIZED SECURITY WORK");
        return;
    }
    if (text_equal(argv[0], "LS") || text_equal(argv[0], "DIR")) {
        rc = terminal_cmd_ls();
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "PWD")) {
        terminal_print_path();
        return;
    }
    if (text_equal(argv[0], "CD")) {
        rc = terminal_cmd_cd(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "MKDIR") || text_equal(argv[0], "MKIDR") || text_equal(argv[0], "MD")) {
        rc = terminal_cmd_mkdir(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "TOUCH")) {
        rc = terminal_cmd_touch(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "CAT") || text_equal(argv[0], "TYPE")) {
        rc = terminal_cmd_cat(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "WRITE")) {
        rc = terminal_cmd_write(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "EDIT")) {
        rc = terminal_cmd_edit(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "APPEND")) {
        rc = terminal_cmd_append(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "COPY") || text_equal(argv[0], "CP")) {
        rc = terminal_cmd_copy(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "CUT") || text_equal(argv[0], "MV")) {
        rc = terminal_cmd_cut(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "PASTE")) {
        rc = terminal_cmd_paste(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "DEL") || text_equal(argv[0], "DELETE") || text_equal(argv[0], "RM")) {
        rc = terminal_cmd_delete(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "BIN") || text_equal(argv[0], "WASTEBIN")) {
        rc = terminal_cmd_wastebin_list();
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "RESTORE")) {
        rc = terminal_cmd_restore(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "EMPTYBIN")) {
        rc = terminal_cmd_emptybin();
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "ECHO")) {
        char payload[TERM_LINE_MAX];
        terminal_join_args(payload, TERM_LINE_MAX, argc, argv, 1u);
        terminal_push_line(payload[0] == '\0' ? "ECHO" : payload);
        return;
    }
    if (text_equal(argv[0], "CLEAR") || text_equal(argv[0], "CLS")) {
        terminal_clear_history();
        terminal_push_ok("TERMINAL CLEARED");
        return;
    }
    if (text_equal(argv[0], "WEVOA")) {
        terminal_push_line("WEVOA OS GUI ALPHA");
        terminal_push_line("FATHER OF WEVOA ABDULAHAD IBIN SHIHABUDHEEN");
        terminal_push_line("CUSTOM COMMANDLINE READY");
        return;
    }
    if (text_equal(argv[0], "WVER")) {
        terminal_push_line("WEVOA SEC TERMINAL V0.7");
        return;
    }
    if (text_equal(argv[0], "WHOAMI")) {
        terminal_push_line("WEVOA SEC OPERATOR");
        return;
    }
    if (text_equal(argv[0], "NOTEPAD") || text_equal(argv[0], "NOTE") || text_equal(argv[0], "CODE")) {
        focus_app(APP_NOTEPAD);
        terminal_push_ok("WEVOA CODE OPEN");
        return;
    }
    if (text_equal(argv[0], "CALC") || text_equal(argv[0], "CALCULATOR") || text_equal(argv[0], "SCAN")) {
        focus_app(APP_CALCULATOR);
        terminal_push_ok("PORT SCAN PANEL OPEN");
        return;
    }
    if (text_equal(argv[0], "IMG") || text_equal(argv[0], "IMAGE") || text_equal(argv[0], "IMAGES") || text_equal(argv[0], "LOGS")) {
        focus_app(APP_IMAGE_VIEWER);
        terminal_push_ok("LOG VIEWER OPEN");
        return;
    }
    if (text_equal(argv[0], "VIDEO") || text_equal(argv[0], "VID") || text_equal(argv[0], "PLAYER") || text_equal(argv[0], "PACKET")) {
        focus_app(APP_VIDEO_PLAYER);
        terminal_push_ok("PACKET ANALYZER OPEN");
        return;
    }
    if (text_equal(argv[0], "BROWSER") || text_equal(argv[0], "WEB") || text_equal(argv[0], "RECON")) {
        focus_app(APP_BROWSER);
        if (argc >= 2u) {
            char target[TERM_INPUT_MAX];
            terminal_join_args(target, sizeof(target), argc, argv, 1u);
            browser_navigate(target);
        }
        terminal_push_ok("BROWSER OPEN");
        return;
    }
    if (text_equal(argv[0], "DNS")) {
        rc = terminal_cmd_dns(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "PORT")) {
        rc = terminal_cmd_port(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "NETUP")) {
        rc = terminal_cmd_net_link(1u);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "NETDOWN")) {
        rc = terminal_cmd_net_link(0u);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "LOCK")) {
        if (argc >= 2u && (text_equal(argv[1], "OFF") || text_equal(argv[1], "DISABLE"))) {
            int rclock = config_set(WEVOA_CFG_LOCK_ENABLED, 0u);
            state_sync_from_config();
            terminal_report_status(rclock);
            if (rclock == W_OK) {
                mark_pstore_dirty();
                terminal_push_ok("LOCK DISABLED");
            }
            return;
        }
        if (argc >= 2u && (text_equal(argv[1], "ON") || text_equal(argv[1], "ENABLE"))) {
            int rclock = config_set(WEVOA_CFG_LOCK_ENABLED, 1u);
            state_sync_from_config();
            terminal_report_status(rclock);
            if (rclock == W_OK) {
                mark_pstore_dirty();
                terminal_push_ok("LOCK ENABLED");
            }
            return;
        }
        lock_screen_activate();
        terminal_push_ok("LOCKED");
        return;
    }
    if (text_equal(argv[0], "WALL")) {
        int rcm = config_set(WEVOA_CFG_WALL, state.wallpaper_index + 1u);
        if (rcm == W_OK) {
            state_sync_from_config();
            mark_pstore_dirty();
        }
        build_desktop_template();
        terminal_report_status(rcm);
        if (rcm == W_OK) {
            terminal_push_ok("WALLPAPER CHANGED");
        }
        return;
    }
    if (text_equal(argv[0], "DESKLS")) {
        rc = terminal_cmd_desk_ls();
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "DESKDIR")) {
        rc = terminal_cmd_desk_create(argc, argv, true);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "DESKFILE")) {
        rc = terminal_cmd_desk_create(argc, argv, false);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "WGET")) {
        rc = terminal_cmd_wget(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "WLIST")) {
        rc = terminal_cmd_wlist();
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "WSET")) {
        rc = terminal_cmd_wset(argc, argv);
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "WSAVE")) {
        rc = terminal_cmd_wsave();
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "WLOAD")) {
        rc = terminal_cmd_wload();
        terminal_report_status(rc);
        return;
    }
    if (text_equal(argv[0], "WRESET")) {
        rc = terminal_cmd_wreset();
        terminal_report_status(rc);
        return;
    }
    terminal_push_err(wstatus_str(W_ERR_NOT_FOUND));
}

static void terminal_handle_key_input(void) {
    bool shift = state.key_down[SC_LSHIFT] || state.key_down[SC_RSHIFT];

    if (state.key_pressed[SC_BACKSPACE]) {
        if (terminal_state.input_len > 0u) {
            terminal_state.input_len--;
            terminal_state.input[terminal_state.input_len] = '\0';
        }
    }

    if (state.key_pressed[SC_ENTER]) {
        terminal_execute_command(terminal_state.input);
        terminal_state.input_len = 0u;
        terminal_state.input[0] = '\0';
        return;
    }

    for (uint32_t sc = 0u; sc < KEY_MAX; ++sc) {
        if (!state.key_pressed[sc]) {
            continue;
        }
        if (sc == SC_ENTER || sc == SC_BACKSPACE || sc == SC_TAB || sc == SC_ESC) {
            continue;
        }
        char ch = scancode_to_char((uint8_t)sc, shift);
        if (ch == '\0') {
            continue;
        }
        if (terminal_state.input_len + 1u < TERM_INPUT_MAX) {
            terminal_state.input[terminal_state.input_len++] = ch;
            terminal_state.input[terminal_state.input_len] = '\0';
        }
    }
}

static bool terminal_input_key_pressed(void) {
    if (state.key_pressed[SC_ENTER] || state.key_pressed[SC_BACKSPACE]) {
        return true;
    }
    for (uint32_t sc = 0u; sc < KEY_MAX; ++sc) {
        if (!state.key_pressed[sc]) {
            continue;
        }
        if (scancode_to_char((uint8_t)sc, false) != '\0') {
            return true;
        }
    }
    return false;
}

static void draw_icon_glyph(uint8_t app, int32_t x, int32_t y, int32_t s) {
    const struct wevoa_ui_icon_painter painter = {
        .fill_rect = fill_rect,
        .stroke_rect = stroke_rect,
        .draw_glass_rect = draw_glass_rect,
    };
    wevoa_ui_icons_draw_app(app, x, y, s, &painter);
}

static void draw_desktop_item_glyph(bool is_dir, int32_t x, int32_t y, int32_t s) {
    const struct wevoa_ui_icon_painter painter = {
        .fill_rect = fill_rect,
        .stroke_rect = stroke_rect,
        .draw_glass_rect = draw_glass_rect,
    };
    wevoa_ui_icons_draw_desktop_item(is_dir, x, y, s, &painter);
}

static int32_t desktop_scale(void) {
    if (ui_scale_override >= 1u && ui_scale_override <= 3u) {
        return (int32_t)ui_scale_override;
    }
    if (fb_width >= 1200u || fb_height >= 900u) {
        return 3;
    }
    if (fb_width >= 800u || fb_height >= 600u) {
        return 2;
    }
    return 1;
}

static int32_t taskbar_h(void) {
    int32_t scale = desktop_scale();
    return 22 + scale * 8;
}

static int32_t desktop_icon_glyph_size(void) {
    return 18 * desktop_scale();
}

static int32_t desktop_icon_box_w(void) {
    return desktop_icon_glyph_size() + 6 * desktop_scale();
}

static int32_t desktop_icon_box_h(void) {
    return desktop_icon_glyph_size() + 9 * desktop_scale();
}

static void draw_wallpaper(void) {
    uint8_t mode = (uint8_t)(state.wallpaper_index % WALLPAPER_COUNT);
    int32_t h = (int32_t)fb_height;
    int32_t w = (int32_t)fb_width;

    if (mode == 0u) {
        /* Photo-style daylight wallpaper */
        for (int32_t y = 0; y < h; ++y) {
            uint8_t c = (y < (h * 32) / 100) ? COLOR_WALL_SKY :
                        (y < (h * 58) / 100) ? COLOR_WALL_HORIZON :
                        (y < (h * 74) / 100) ? COLOR_DESKTOP :
                        COLOR_WALL_GROUND;
            fill_rect(0, y, w, 1, c);
        }

        int32_t sun_x = w - w / 8;
        int32_t sun_y = h / 6;
        int32_t sun_r = h / 10;
        for (int32_t py = -sun_r; py <= sun_r; ++py) {
            for (int32_t px = -sun_r; px <= sun_r; ++px) {
                if (px * px + py * py <= sun_r * sun_r) {
                    put_px(sun_x + px, sun_y + py, COLOR_WALL_SUN);
                }
            }
        }

        int32_t ridge_base = (h * 60) / 100;
        int32_t front_base = (h * 70) / 100;
        for (int32_t x = 0; x < w; ++x) {
            int32_t ridge1 = ridge_base - abs_i32(x - (w / 4)) / 3;
            int32_t ridge2 = ridge_base + 8 - abs_i32(x - (w * 3 / 5)) / 4;
            int32_t ridge = (ridge1 < ridge2) ? ridge1 : ridge2;
            ridge = clamp_i32(ridge, h / 3, h - 1);
            fill_rect(x, ridge, 1, h - ridge, COLOR_TITLE_INACTIVE);
        }

        for (int32_t x = 0; x < w; ++x) {
            int32_t ridge = front_base - abs_i32(x - (w / 2)) / 5;
            ridge = clamp_i32(ridge, h / 2, h - 1);
            fill_rect(x, ridge, 1, h - ridge, COLOR_BORDER);
        }

        for (int32_t y = (h * 74) / 100; y < h; y += 6) {
            fill_rect(0, y, w, 1, COLOR_TASKBAR_HI);
        }
        return;
    }

    if (mode == 1u) {
        /* Sunset wallpaper */
        for (int32_t y = 0; y < h; ++y) {
            uint8_t c = (y < h / 5) ? COLOR_WALL_SKY :
                        (y < h / 3) ? COLOR_WALL_SUN :
                        (y < (h * 2) / 3) ? COLOR_WALL_HORIZON :
                        COLOR_WALL_GROUND;
            fill_rect(0, y, w, 1, c);
        }
        int32_t sun_x = w / 2;
        int32_t sun_y = h / 3;
        int32_t sun_r = h / 8;
        for (int32_t py = -sun_r; py <= sun_r; ++py) {
            for (int32_t px = -sun_r; px <= sun_r; ++px) {
                if (px * px + py * py <= sun_r * sun_r) {
                    put_px(sun_x + px, sun_y + py, COLOR_WALL_SUN);
                }
            }
        }
        for (int32_t y = (h * 2) / 3; y < h; y += 10) {
            fill_rect(0, y, w, 1, COLOR_BORDER);
        }
        return;
    }

    /* Clean two-zone modern wallpaper */
    int32_t split = (h * 27) / 100;
    fill_rect(0, 0, w, split, COLOR_WINDOW_BG);
    fill_rect(0, split, w, h - split, COLOR_DESKTOP);

    /* subtle grid only in lower zone */
    for (int32_t y = split + 20; y < h; y += 48) {
        fill_rect(0, y, w, 1, COLOR_TASKBAR_HI);
    }
    for (int32_t x = 16; x < w; x += 76) {
        fill_rect(x, split, 1, h - split, COLOR_TASKBAR_HI);
    }

    /* soft atmospheric bands */
    fill_rect(0, split + 2, w, 2, COLOR_PANEL_BG);
    fill_rect(0, split + (h - split) / 2, w, 1, COLOR_PANEL_BG);
}

static const char* app_desktop_label(uint32_t app) {
    if (app == APP_FILE_MANAGER) {
        return "LAB";
    }
    if (app == APP_TERMINAL) {
        return "SHELL";
    }
    if (app == APP_SETTINGS) {
        return "OPS";
    }
    if (app == APP_NOTEPAD) {
        return "CODE";
    }
    if (app == APP_CALCULATOR) {
        return "SCAN";
    }
    if (app == APP_IMAGE_VIEWER) {
        return "LOGS";
    }
    if (app == APP_VIDEO_PLAYER) {
        return "PACKET";
    }
    return "RECON";
}

static const char* app_window_title(uint32_t app) {
    if (app == APP_FILE_MANAGER) {
        return "LAB FILES";
    }
    if (app == APP_TERMINAL) {
        return "SECURE SHELL";
    }
    if (app == APP_SETTINGS) {
        return "OPS CENTER";
    }
    if (app == APP_NOTEPAD) {
        return "WEVOA CODE";
    }
    if (app == APP_CALCULATOR) {
        return "PORT SCAN";
    }
    if (app == APP_IMAGE_VIEWER) {
        return "LOG VIEWER";
    }
    if (app == APP_VIDEO_PLAYER) {
        return "PACKET ANALYZER";
    }
    return "WEB RECON";
}

static const char* app_launcher_label(uint32_t app) {
    if (app == APP_FILE_MANAGER) {
        return "File Manager";
    }
    if (app == APP_TERMINAL) {
        return "Terminal";
    }
    if (app == APP_SETTINGS) {
        return "Settings";
    }
    if (app == APP_NOTEPAD) {
        return "Notepad";
    }
    if (app == APP_CALCULATOR) {
        return "Calculator";
    }
    if (app == APP_IMAGE_VIEWER) {
        return "Image Viewer";
    }
    if (app == APP_VIDEO_PLAYER) {
        return "Video Player";
    }
    return "Browser";
}

static void build_desktop_template(void) {
    draw_base = desktop_buffer;
    draw_pitch = fb_width;

    int32_t scale = desktop_scale();
    int32_t top_h = shell_top_h();
    int32_t bottom_h = shell_bottom_h();
    int32_t text_scale = ui_text_scale();
    int32_t icon_size = desktop_icon_glyph_size();
    int32_t icon_box_w = desktop_icon_box_w();
    int32_t icon_box_h = desktop_icon_box_h();
    int32_t desktop_y0 = top_h + 8 * scale;
    int32_t desktop_y1 = (int32_t)fb_height - bottom_h - 4;
    uint8_t blur_passes = ui_target_blur_level();

    draw_wallpaper();

    if (shell_is_dock_mode()) {
        if (blur_passes > 0u) {
            blur_region(0, 0, (int32_t)fb_width, top_h, blur_passes);
            blur_region(0, (int32_t)fb_height - bottom_h, (int32_t)fb_width, bottom_h, blur_passes);
        }

        fill_rect(0, 0, (int32_t)fb_width, top_h, COLOR_TASKBAR);
        fill_rect(0, top_h - 1, (int32_t)fb_width, 1, COLOR_BORDER);

        int32_t search_h = top_h - 6;
        if (search_h < 10) {
            search_h = 10;
        }
        int32_t search_x = 8;
        int32_t search_y = (top_h - search_h) / 2;
        int32_t search_w = clamp_i32((int32_t)fb_width / 4, 120, 340);
        shell_search_x = search_x;
        shell_search_y = search_y;
        shell_search_w = search_w;
        shell_search_h = search_h;
        draw_glass_rect(search_x, search_y, search_w, search_h, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_text5x7(search_x + 8, search_y + 4, "WEVOA SEARCH", COLOR_TITLE_INACTIVE, text_scale);

        taskbar_tray_w = clamp_i32(120 + 42 * scale, 140, 250);
        if (taskbar_tray_w > (int32_t)fb_width - search_w - 30) {
            taskbar_tray_w = (int32_t)fb_width - search_w - 30;
        }
        taskbar_tray_h = search_h;
        taskbar_tray_x = (int32_t)fb_width - taskbar_tray_w - 8;
        taskbar_tray_y = search_y;
        draw_glass_rect(taskbar_tray_x, taskbar_tray_y, taskbar_tray_w, taskbar_tray_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);

        settings_btn_h = taskbar_tray_h - 2;
        if (settings_btn_h < 10) {
            settings_btn_h = 10;
        }
        settings_btn_w = settings_btn_h + 4;
        settings_btn_x = taskbar_tray_x + 2;
        settings_btn_y = taskbar_tray_y + 1;
        draw_glass_rect(settings_btn_x, settings_btn_y, settings_btn_w, settings_btn_h, COLOR_TITLE_ACTIVE, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_icon_glyph(APP_SETTINGS, settings_btn_x + 1, settings_btn_y + 1, settings_btn_h - 2);

        dock_panel_h = clamp_i32(bottom_h - 8, 20, 40);
        taskbar_icon_size = clamp_i32(dock_panel_h - 6, 14, 26);
        int32_t dock_gap = 4;
        dock_panel_w = APP_COUNT * taskbar_icon_size + (int32_t)(APP_COUNT - 1u) * dock_gap + 16;
        dock_panel_w = clamp_i32(dock_panel_w, 160, (int32_t)fb_width - 20);
        dock_panel_x = ((int32_t)fb_width - dock_panel_w) / 2;
        dock_panel_y = (int32_t)fb_height - bottom_h + (bottom_h - dock_panel_h) / 2;
        draw_glass_rect(dock_panel_x, dock_panel_y, dock_panel_w, dock_panel_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);

        int32_t icon_stride = taskbar_icon_size + dock_gap;
        int32_t icons_w = (int32_t)APP_COUNT * taskbar_icon_size + (int32_t)(APP_COUNT - 1u) * dock_gap;
        int32_t icon_x = dock_panel_x + (dock_panel_w - icons_w) / 2;
        int32_t icon_y = dock_panel_y + (dock_panel_h - taskbar_icon_size) / 2;
        for (uint32_t i = 0u; i < APP_COUNT; ++i) {
            taskbar_icon_x[i] = icon_x + (int32_t)i * icon_stride;
            taskbar_icon_y[i] = icon_y;
        }
    } else {
        dock_panel_x = dock_panel_y = dock_panel_w = dock_panel_h = 0;
        int32_t task_y = 0;
        int32_t tb_h = top_h;
        int32_t search_x = 10;
        int32_t search_h = tb_h - 10;
        int32_t search_y = task_y + (tb_h - search_h) / 2;
        int32_t search_w = clamp_i32((int32_t)fb_width / 4, 110, 300);
        shell_search_x = search_x;
        shell_search_y = search_y;
        shell_search_w = search_w;
        shell_search_h = search_h;

        if (blur_passes > 0u) {
            blur_region(0, 0, (int32_t)fb_width, tb_h, blur_passes);
        }
        fill_rect(0, task_y, (int32_t)fb_width, tb_h, COLOR_TASKBAR);
        fill_rect(0, task_y, (int32_t)fb_width, 1, COLOR_TASKBAR_HI);
        fill_rect(0, task_y + 1, (int32_t)fb_width, 1, COLOR_TITLE_INACTIVE);
        fill_rect(0, task_y + tb_h - 1, (int32_t)fb_width, 1, COLOR_BORDER);

        if (search_h < 10) {
            search_h = 10;
        }
        draw_glass_rect(search_x, search_y, search_w, search_h, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
        int32_t mag_x = search_x + 6;
        int32_t mag_y = search_y + (search_h / 2) - 3;
        fill_rect(mag_x + 1, mag_y + 1, 4, 4, COLOR_PANEL_BG);
        stroke_rect(mag_x, mag_y, 6, 6, COLOR_BORDER);
        fill_rect(mag_x + 5, mag_y + 5, 3, 1, COLOR_BORDER);
        draw_text5x7(search_x + 18, search_y + 5, "SEARCH WEVOA", COLOR_TITLE_INACTIVE, text_scale);

        taskbar_tray_w = clamp_i32(120 + 58 * scale, 160, 300);
        if (taskbar_tray_w > (int32_t)fb_width - 24) {
            taskbar_tray_w = (int32_t)fb_width - 24;
        }
        taskbar_tray_h = tb_h - 8;
        taskbar_tray_x = (int32_t)fb_width - taskbar_tray_w - 6;
        taskbar_tray_y = task_y + (tb_h - taskbar_tray_h) / 2;
        draw_glass_rect(taskbar_tray_x, taskbar_tray_y, taskbar_tray_w, taskbar_tray_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);

        settings_btn_h = taskbar_tray_h - 4;
        if (settings_btn_h < 10) {
            settings_btn_h = 10;
        }
        settings_btn_w = settings_btn_h + 6;
        settings_btn_x = taskbar_tray_x + 2;
        settings_btn_y = taskbar_tray_y + 2;
        draw_glass_rect(settings_btn_x, settings_btn_y, settings_btn_w, settings_btn_h, COLOR_TITLE_ACTIVE, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_icon_glyph(APP_SETTINGS, settings_btn_x + 1, settings_btn_y + 1, settings_btn_h - 2);

        taskbar_icon_size = clamp_i32(tb_h - 10, 14, 24);
        int32_t task_icon_y = task_y + (tb_h - taskbar_icon_size) / 2;
        int32_t task_icon_x0 = search_x + search_w + 10;
        int32_t task_icon_stride = taskbar_icon_size + 4;
        int32_t max_icon_right = taskbar_tray_x - 8;
        int32_t icon_end = task_icon_x0 + (int32_t)(APP_COUNT - 1u) * task_icon_stride + taskbar_icon_size;
        if (icon_end > max_icon_right) {
            task_icon_stride = taskbar_icon_size + 1;
            icon_end = task_icon_x0 + (int32_t)(APP_COUNT - 1u) * task_icon_stride + taskbar_icon_size;
            if (icon_end > max_icon_right) {
                int32_t avail = max_icon_right - task_icon_x0;
                int32_t compact = avail / (int32_t)APP_COUNT;
                compact = clamp_i32(compact, 8, taskbar_icon_size);
                taskbar_icon_size = compact;
                task_icon_stride = compact;
                task_icon_y = task_y + (tb_h - taskbar_icon_size) / 2;
            }
        }
        for (uint32_t i = 0; i < APP_COUNT; ++i) {
            taskbar_icon_x[i] = task_icon_x0 + (int32_t)i * task_icon_stride;
            taskbar_icon_y[i] = task_icon_y;
        }
    }

    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        const char* label = app_desktop_label(i);
        int32_t ix = 10 * scale;
        int32_t iy = desktop_y0 + (int32_t)i * (icon_box_h + 8 * scale);
        desktop_icon_x[i] = ix;
        desktop_icon_y[i] = iy;

        draw_glass_rect(ix - 2 * scale, iy - 2 * scale, icon_box_w, icon_box_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_icon_glyph((uint8_t)i, ix, iy, icon_size);

        draw_text5x7(ix - scale, iy + icon_size + 2 * scale, label, COLOR_BORDER, text_scale);
    }

    int32_t dyn_x0 = 64 * scale;
    int32_t dyn_y0 = desktop_y0;
    int32_t dyn_stride_y = icon_box_h + 8 * scale;
    int32_t usable_h = desktop_y1 - dyn_y0 - icon_box_h - 2;
    int32_t rows_per_col = usable_h / dyn_stride_y;
    if (rows_per_col < 1) {
        rows_per_col = 1;
    }

    for (uint32_t i = 0u; i < DESKTOP_ITEM_MAX; ++i) {
        desktop_item_x[i] = 0;
        desktop_item_y[i] = 0;
    }

    for (uint32_t i = 0u; i < desktop_item_count && i < DESKTOP_ITEM_MAX; ++i) {
        int32_t col = (int32_t)i / rows_per_col;
        int32_t row = (int32_t)i % rows_per_col;
        int32_t ix = dyn_x0 + col * (icon_box_w + 10 * scale);
        int32_t iy = dyn_y0 + row * dyn_stride_y;
        desktop_item_x[i] = ix;
        desktop_item_y[i] = iy;

        draw_glass_rect(ix - 2 * scale, iy - 2 * scale, icon_box_w, icon_box_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_desktop_item_glyph(desktop_items[i].is_dir, ix, iy, icon_size);
        draw_text5x7(ix - scale, iy + icon_size + 2 * scale, desktop_items[i].name, COLOR_BORDER, text_scale);
    }
}

static void copy_desktop_to_frame(void) {
    for (uint32_t y = 0; y < fb_height; ++y) {
        uint32_t row = y * fb_width;
        for (uint32_t x = 0; x < fb_width; ++x) {
            frame_buffer[row + x] = desktop_buffer[row + x];
        }
    }

    draw_base = frame_buffer;
    draw_pitch = fb_width;
}

static void present_frame(void) {
    if (fb_bpp_mode == 8u) {
        for (uint32_t y = 0; y < fb_height; ++y) {
            uint32_t src_row = y * fb_width;
            uint32_t dst_row = y * fb_pitch;
            for (uint32_t x = 0; x < fb_width; ++x) {
                fb_base[dst_row + x] = frame_buffer[src_row + x];
            }
        }
        return;
    }

    if (fb_bpp_mode == 32u) {
        for (uint32_t y = 0; y < fb_height; ++y) {
            uint32_t src_row = y * fb_width;
            uint32_t* dst32 = (uint32_t*)(fb_base + y * fb_pitch);
            for (uint32_t x = 0; x < fb_width; ++x) {
                dst32[x] = color_lut32[frame_buffer[src_row + x]];
            }
        }
        return;
    }

    if (fb_bpp_mode == 24u) {
        for (uint32_t y = 0; y < fb_height; ++y) {
            uint32_t src_row = y * fb_width;
            uint8_t* dst = fb_base + y * fb_pitch;
            for (uint32_t x = 0; x < fb_width; ++x) {
                uint32_t c = color_lut32[frame_buffer[src_row + x]];
                dst[x * 3u + 0u] = (uint8_t)(c & 0xFFu);
                dst[x * 3u + 1u] = (uint8_t)((c >> 8) & 0xFFu);
                dst[x * 3u + 2u] = (uint8_t)((c >> 16) & 0xFFu);
            }
        }
    }
}

static void draw_taskbar_icons(void) {
    int32_t scale = desktop_scale();
    int32_t slot = taskbar_icon_size;
    int32_t top_h = shell_top_h();
    int32_t text_scale = ui_text_scale();

    if (ui_blur_runtime > 0u) {
        blur_region(0, 0, (int32_t)fb_width, top_h, ui_blur_runtime);
        if (shell_is_dock_mode() && dock_panel_w > 0 && dock_panel_h > 0) {
            blur_region(dock_panel_x, dock_panel_y, dock_panel_w, dock_panel_h, ui_blur_runtime);
        }
    }

    if (shell_is_dock_mode()) {
        fill_rect(0, 0, (int32_t)fb_width, top_h, COLOR_TASKBAR);
        fill_rect(0, top_h - 1, (int32_t)fb_width, 1, COLOR_BORDER);
        draw_glass_rect(dock_panel_x, dock_panel_y, dock_panel_w, dock_panel_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_glass_rect(taskbar_tray_x, taskbar_tray_y, taskbar_tray_w, taskbar_tray_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_glass_rect(settings_btn_x, settings_btn_y, settings_btn_w, settings_btn_h, COLOR_TITLE_ACTIVE, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_icon_glyph(APP_SETTINGS, settings_btn_x + 1, settings_btn_y + 1, settings_btn_h - 2);
    }

    if (shell_search_w > 0 && shell_search_h > 0) {
        uint8_t search_bg = shell_launcher_open ? COLOR_TASKBAR_HI : COLOR_WINDOW_BG;
        draw_glass_rect(shell_search_x, shell_search_y, shell_search_w, shell_search_h, search_bg, COLOR_TASKBAR_HI, COLOR_BORDER);
        if (shell_is_dock_mode()) {
            draw_text5x7(shell_search_x + 8, shell_search_y + 4, "WEVOA SEARCH", COLOR_TITLE_INACTIVE, text_scale);
        } else {
            int32_t mag_x = shell_search_x + 6;
            int32_t mag_y = shell_search_y + (shell_search_h / 2) - 3;
            fill_rect(mag_x + 1, mag_y + 1, 4, 4, COLOR_PANEL_BG);
            stroke_rect(mag_x, mag_y, 6, 6, COLOR_BORDER);
            fill_rect(mag_x + 5, mag_y + 5, 3, 1, COLOR_BORDER);
            draw_text5x7(shell_search_x + 18, shell_search_y + 5, "SEARCH WEVOA", COLOR_TITLE_INACTIVE, text_scale);
        }
    }

    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        int32_t x = taskbar_icon_x[i];
        int32_t y = taskbar_icon_y[i];
        bool open = state.windows[i].open;
        bool minimized = state.windows[i].minimized;
        bool active = state.active_app == (int32_t)i && open && !minimized;

        uint8_t bg = (open && !minimized) ? COLOR_TASKBAR_HI : (open ? COLOR_PANEL_BG : COLOR_TASKBAR);
        if (shell_is_dock_mode()) {
            bg = (open && !minimized) ? COLOR_ACCENT : COLOR_PANEL_BG;
        }
        draw_glass_rect(x, y, slot, slot, bg, COLOR_TASKBAR_HI, COLOR_BORDER);
        if (active) {
            stroke_rect(x - 1, y - 1, slot + 2, slot + 2, COLOR_ACCENT);
        }
        draw_icon_glyph((uint8_t)i, x + scale, y + scale, slot - 2 * scale);

        if (open) {
            fill_rect(x + 2 * scale, y + slot - scale, slot - 4 * scale, scale, active ? COLOR_ACCENT : COLOR_PANEL_BG);
        }
    }

    if (!shell_is_dock_mode()) {
        draw_glass_rect(taskbar_tray_x, taskbar_tray_y, taskbar_tray_w, taskbar_tray_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
    }
    if (state.active_app == (int32_t)APP_SETTINGS && state.windows[APP_SETTINGS].open && !state.windows[APP_SETTINGS].minimized) {
        stroke_rect(settings_btn_x - 1, settings_btn_y - 1, settings_btn_w + 2, settings_btn_h + 2, COLOR_ACCENT);
    }

    uint8_t link_up = 0u;
    (void)net_get_link(&link_up);
    bool tray_net_on = (link_up != 0u) && state.settings_wifi_on;
    int32_t tray_icon_x = settings_btn_x + settings_btn_w + 6;
    int32_t tray_center_y = taskbar_tray_y + taskbar_tray_h / 2;

    char hh[3];
    char mm[3];
    char dd[3];
    char mo[3];
    char clock_text[8];
    char date_text[8];
    uint8_t hour = 0u;
    uint8_t minute = 0u;
    uint8_t day = 0u;
    uint8_t month = 0u;
    if (cmos_read_hour_minute(&hour, &minute)) {
        u8_to_2digit_text(hour, hh);
        u8_to_2digit_text(minute, mm);
        text_copy(clock_text, sizeof(clock_text), hh);
        text_append(clock_text, sizeof(clock_text), ":");
        text_append(clock_text, sizeof(clock_text), mm);
    } else {
        text_copy(clock_text, sizeof(clock_text), "--:--");
    }
    if (cmos_read_day_month(&day, &month)) {
        u8_to_2digit_text(month, mo);
        u8_to_2digit_text(day, dd);
        text_copy(date_text, sizeof(date_text), mo);
        text_append(date_text, sizeof(date_text), "/");
        text_append(date_text, sizeof(date_text), dd);
    } else {
        text_copy(date_text, sizeof(date_text), "--/--");
    }

    int32_t tray_time_scale = (taskbar_tray_h >= 30) ? 2 : 1;
    int32_t clock_w = (int32_t)text_len(clock_text) * 6 * tray_time_scale;
    int32_t date_w = (int32_t)text_len(date_text) * 6;
    int32_t text_w = (clock_w > date_w) ? clock_w : date_w;
    int32_t text_x = taskbar_tray_x + taskbar_tray_w - text_w - 6;
    int32_t text_y = taskbar_tray_y + 2;
    draw_text5x7(text_x, text_y, clock_text, COLOR_BORDER, tray_time_scale);
    draw_text5x7(text_x, text_y + 8 * tray_time_scale + 1, date_text, COLOR_TITLE_INACTIVE, 1);

    int32_t wifi_x = text_x - 18;
    int32_t wifi_y = tray_center_y + 5;
    for (int32_t i = 0; i < 4; ++i) {
        int32_t bar_h = i + 2;
        int32_t bx = wifi_x + i * 3;
        int32_t by = wifi_y - bar_h;
        uint8_t c = tray_net_on ? COLOR_ACCENT : COLOR_TITLE_INACTIVE;
        fill_rect(bx, by, 2, bar_h, c);
    }

    int32_t spk_x = wifi_x - 14;
    int32_t spk_y = tray_center_y - 4;
    fill_rect(spk_x, spk_y + 2, 3, 5, COLOR_BORDER);
    fill_rect(spk_x + 3, spk_y + 1, 2, 7, COLOR_BORDER);
    fill_rect(spk_x + 6, spk_y + 2, 1, 1, COLOR_BORDER);
    fill_rect(spk_x + 7, spk_y + 3, 1, 1, COLOR_BORDER);
    fill_rect(spk_x + 8, spk_y + 4, 1, 1, COLOR_BORDER);
    fill_rect(spk_x + 7, spk_y + 5, 1, 1, COLOR_BORDER);
    fill_rect(spk_x + 6, spk_y + 6, 1, 1, COLOR_BORDER);

    int32_t chevron_x = tray_icon_x;
    int32_t chevron_y = tray_center_y - 2;
    fill_rect(chevron_x, chevron_y, 2, 1, COLOR_TITLE_INACTIVE);
    fill_rect(chevron_x + 2, chevron_y + 1, 2, 1, COLOR_TITLE_INACTIVE);
    fill_rect(chevron_x + 4, chevron_y, 2, 1, COLOR_TITLE_INACTIVE);

    int32_t net_mark_x = chevron_x + 8;
    int32_t net_mark_y = tray_center_y - 1;
    fill_rect(net_mark_x, net_mark_y, 2, 2, tray_net_on ? COLOR_ACCENT : COLOR_TITLE_INACTIVE);
}

static void draw_window_content_file(const struct app_window* w) {
    int32_t text_scale = ui_text_scale();
    int32_t x = w->x + 3;
    int32_t y = w->y + TITLE_H + 2;
    int32_t ww = w->w - 6;
    int32_t hh = w->h - TITLE_H - 5;

    fill_rect(x, y, ww, hh, COLOR_WINDOW_BG);
    stroke_rect(x, y, ww, hh, COLOR_BORDER);

    if (ww < 160 || hh < 96) {
        draw_text5x7(x + 6, y + 8, "LAB FILES", COLOR_BORDER, text_scale);
        draw_text5x7(x + 6, y + 20, "WINDOW TOO SMALL", COLOR_TITLE_INACTIVE, text_scale);
        return;
    }

    int32_t tab_h = 12;
    int32_t nav_h = 14;
    int32_t cmd_h = 13;
    int32_t status_h = 11;

    /* Tab strip */
    fill_rect(x + 1, y + 1, ww - 2, tab_h, COLOR_TASKBAR);
    fill_rect(x + 1, y + tab_h, ww - 2, 1, COLOR_BORDER);
    int32_t home_tab_w = clamp_i32(ww / 5, 44, 84);
    draw_glass_rect(x + 4, y + 2, home_tab_w, tab_h - 2, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_text5x7(x + 8, y + 4, "HOME", COLOR_BORDER, text_scale);
    draw_text5x7(x + 4 + home_tab_w + 6, y + 4, "+", COLOR_BORDER, text_scale);

    /* Navigation + address + search row */
    int32_t nav_row_y = y + tab_h + 2;
    fill_rect(x + 1, nav_row_y, ww - 2, nav_h, COLOR_PANEL_BG);
    fill_rect(x + 1, nav_row_y + nav_h, ww - 2, 1, COLOR_BORDER);

    int32_t nbtn = 10;
    int32_t nbg = 2;
    int32_t nav_btn_x = x + 4;
    draw_glass_rect(nav_btn_x, nav_row_y + nbg, nbtn, nav_h - 2 * nbg, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_text5x7(nav_btn_x + 3, nav_row_y + 4, "<", COLOR_BORDER, text_scale);
    nav_btn_x += nbtn + 2;
    draw_glass_rect(nav_btn_x, nav_row_y + nbg, nbtn, nav_h - 2 * nbg, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_text5x7(nav_btn_x + 3, nav_row_y + 4, ">", COLOR_BORDER, text_scale);
    nav_btn_x += nbtn + 2;
    draw_glass_rect(nav_btn_x, nav_row_y + nbg, nbtn, nav_h - 2 * nbg, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_text5x7(nav_btn_x + 3, nav_row_y + 4, "^", COLOR_BORDER, text_scale);
    nav_btn_x += nbtn + 2;
    draw_glass_rect(nav_btn_x, nav_row_y + nbg, nbtn + 2, nav_h - 2 * nbg, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_text5x7(nav_btn_x + 3, nav_row_y + 4, "R", COLOR_BORDER, text_scale);
    nav_btn_x += nbtn + 5;

    int32_t addr_x = nav_btn_x + 3;
    int32_t row_right = x + ww - 4;
    int32_t free_w = row_right - addr_x;
    int32_t search_w = clamp_i32(free_w / 3, 42, 128);
    if (search_w > free_w - 44) {
        search_w = free_w - 44;
    }
    if (search_w < 36) {
        search_w = 36;
    }
    int32_t addr_w = free_w - search_w - 4;
    if (addr_w < 40) {
        addr_w = 40;
    }
    int32_t search_x = addr_x + addr_w + 4;

    draw_glass_rect(addr_x, nav_row_y + nbg, addr_w, nav_h - 2 * nbg, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_glass_rect(search_x, nav_row_y + nbg, search_w, nav_h - 2 * nbg, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);

    char path[TERM_LINE_MAX];
    char addr_text[TERM_LINE_MAX];
    char addr_trim[32];
    vfs_build_path(terminal_state.cwd_node, path);
    text_copy(addr_text, sizeof(addr_text), "HOME ");
    text_append(addr_text, sizeof(addr_text), path);
    text_copy_truncated(addr_trim, sizeof(addr_trim), addr_text, (uint32_t)((addr_w - 10) / 6));
    draw_text5x7(addr_x + 4, nav_row_y + 4, addr_trim, COLOR_BORDER, text_scale);
    draw_text5x7(search_x + 4, nav_row_y + 4, "SEARCH HOME", COLOR_TITLE_INACTIVE, text_scale);

    /* Command bar */
    int32_t cmd_y = nav_row_y + nav_h + 2;
    fill_rect(x + 1, cmd_y, ww - 2, cmd_h, COLOR_WINDOW_BG);
    fill_rect(x + 1, cmd_y + cmd_h, ww - 2, 1, COLOR_BORDER);
    const char* commands[] = {"NEW", "COPY", "PASTE", "DELETE", "SORT", "VIEW", "FILTER"};
    int32_t btn_x = x + 5;
    int32_t btn_h = cmd_h - 3;
    for (uint32_t i = 0u; i < 7u; ++i) {
        int32_t bw = 32;
        if (i == 3u || i == 6u) {
            bw = 40;
        }
        if (btn_x + bw > x + ww - 4) {
            break;
        }
        draw_button(btn_x, cmd_y + 1, bw, btn_h, commands[i], i == 0u);
        btn_x += bw + 2;
    }

    int32_t body_y = cmd_y + cmd_h + 2;
    int32_t body_h = hh - (body_y - y) - status_h - 2;
    if (body_h < 36) {
        body_h = 36;
    }

    /* Left navigation tree */
    int32_t left_x = x + 4;
    int32_t left_y = body_y;
    int32_t left_w = clamp_i32(ww / 5, 70, 124);
    if (left_w > ww - 78) {
        left_w = ww - 78;
    }
    int32_t left_h = body_h;

    fill_rect(left_x, left_y, left_w, left_h, COLOR_PANEL_BG);
    stroke_rect(left_x, left_y, left_w, left_h, COLOR_BORDER);
    draw_text5x7(left_x + 6, left_y + 6, "QUICK ACCESS", COLOR_BORDER, text_scale);

    const char* nav_items[] = {"HOME", "DESKTOP", "DOCS", "DOWNLOADS", "WASTEBIN", "THIS PC"};
    for (uint32_t i = 0u; i < 6u; ++i) {
        int32_t iy = left_y + 18 + (int32_t)i * 12;
        if (iy + 10 > left_y + left_h - 2) {
            break;
        }
        bool active = (i == 0u);
        fill_rect(left_x + 4, iy, left_w - 8, 10, active ? COLOR_TASKBAR_HI : COLOR_PANEL_BG);
        if (active) {
            fill_rect(left_x + 4, iy, 2, 10, COLOR_ACCENT);
        }
        draw_text5x7(left_x + 8, iy + 2, nav_items[i], COLOR_BORDER, text_scale);
    }

    /* Right content pane */
    int32_t content_x = left_x + left_w + 4;
    int32_t content_y = left_y;
    int32_t content_w = (x + ww - 4) - content_x;
    int32_t content_h = left_h;

    fill_rect(content_x, content_y, content_w, content_h, COLOR_WINDOW_BG);
    stroke_rect(content_x, content_y, content_w, content_h, COLOR_BORDER);

    draw_text5x7(content_x + 6, content_y + 6, "QUICK ACCESS", COLOR_BORDER, text_scale);

    int32_t cards_y = content_y + 16;
    int32_t cards_h = clamp_i32(content_h / 4, 20, 34);
    int32_t card_gap = 3;
    int32_t card_count = 4;
    int32_t card_w = (content_w - 8 - (card_count - 1) * card_gap) / card_count;
    if (card_w < 24) {
        card_count = 3;
        card_w = (content_w - 8 - (card_count - 1) * card_gap) / card_count;
    }

    int32_t shown_cards = 0;
    for (uint32_t i = 0u; i < desktop_item_count && shown_cards < card_count; ++i) {
        int32_t cx = content_x + 4 + shown_cards * (card_w + card_gap);
        draw_glass_rect(cx, cards_y, card_w, cards_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_desktop_item_glyph(desktop_items[i].is_dir, cx + 3, cards_y + 3, 10);
        char card_label[16];
        text_copy_truncated(card_label, sizeof(card_label), desktop_items[i].name, (uint32_t)((card_w - 16) / 6));
        draw_text5x7(cx + 14, cards_y + cards_h / 2 - 3, card_label, COLOR_BORDER, text_scale);
        shown_cards++;
    }
    while (shown_cards < card_count) {
        int32_t cx = content_x + 4 + shown_cards * (card_w + card_gap);
        draw_glass_rect(cx, cards_y, card_w, cards_h, COLOR_PANEL_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_text5x7(cx + 6, cards_y + cards_h / 2 - 3, "EMPTY", COLOR_TITLE_INACTIVE, text_scale);
        shown_cards++;
    }

    int32_t table_y = cards_y + cards_h + 6;
    int32_t table_h = content_h - (table_y - content_y) - 4;
    fill_rect(content_x + 2, table_y, content_w - 4, table_h, COLOR_WINDOW_BG);
    stroke_rect(content_x + 2, table_y, content_w - 4, table_h, COLOR_BORDER);

    int32_t name_x = content_x + 8;
    int32_t type_x = content_x + (content_w * 57) / 100;
    int32_t loc_x = content_x + (content_w * 76) / 100;
    fill_rect(content_x + 3, table_y + 1, content_w - 6, 11, COLOR_TASKBAR);
    draw_text5x7(name_x, table_y + 3, "NAME", COLOR_WINDOW_BG, text_scale);
    draw_text5x7(type_x, table_y + 3, "TYPE", COLOR_WINDOW_BG, text_scale);
    draw_text5x7(loc_x, table_y + 3, "LOCATION", COLOR_WINDOW_BG, text_scale);

    int32_t row_h = 12;
    int32_t row_y = table_y + 13;
    int32_t max_rows = (table_h - 15) / row_h;
    int32_t shown_rows = 0;
    for (uint32_t i = 0u; i < desktop_item_count && shown_rows < max_rows; ++i) {
        int32_t ry = row_y + shown_rows * row_h;
        fill_rect(content_x + 3, ry, content_w - 6, row_h - 1, (shown_rows % 2 == 0) ? COLOR_WINDOW_BG : COLOR_PANEL_BG);
        draw_desktop_item_glyph(desktop_items[i].is_dir, name_x - 2, ry + 1, 9);

        char name_cell[20];
        char type_cell[10];
        char loc_cell[20];
        text_copy_truncated(name_cell, sizeof(name_cell), desktop_items[i].name, (uint32_t)((type_x - (name_x + 12)) / 6));
        text_copy(type_cell, sizeof(type_cell), desktop_items[i].is_dir ? "FOLDER" : "FILE");
        text_copy(loc_cell, sizeof(loc_cell), "DESKTOP");
        if (desktop_items[i].node < VFS_MAX_NODES && vfs_nodes[desktop_items[i].node].used) {
            uint8_t parent = vfs_nodes[desktop_items[i].node].parent;
            if (parent < VFS_MAX_NODES && vfs_nodes[parent].used) {
                char parent_path[TERM_LINE_MAX];
                vfs_build_path(parent, parent_path);
                text_copy_truncated(loc_cell, sizeof(loc_cell), parent_path, (uint32_t)((content_x + content_w - 6 - loc_x) / 6));
            }
        }

        draw_text5x7(name_x + 10, ry + 2, name_cell, COLOR_BORDER, text_scale);
        draw_text5x7(type_x, ry + 2, type_cell, COLOR_BORDER, text_scale);
        draw_text5x7(loc_x, ry + 2, loc_cell, COLOR_TITLE_INACTIVE, text_scale);
        shown_rows++;
    }

    if (shown_rows == 0) {
        draw_text5x7(name_x, row_y + 3, "NO ITEMS", COLOR_TITLE_INACTIVE, text_scale);
    }

    /* Status bar */
    int32_t status_y = y + hh - status_h;
    fill_rect(x + 1, status_y, ww - 2, status_h - 1, COLOR_TASKBAR);
    fill_rect(x + 1, status_y, ww - 2, 1, COLOR_BORDER);
    char count_num[4];
    char status_line[24];
    u8_to_dec_text((uint8_t)desktop_item_count, count_num);
    text_copy(status_line, sizeof(status_line), count_num);
    text_append(status_line, sizeof(status_line), " ITEMS");
    draw_text5x7(x + 6, status_y + 2, status_line, COLOR_WINDOW_BG, text_scale);
    draw_text5x7(x + ww - 68, status_y + 2, "DETAILS VIEW", COLOR_TITLE_INACTIVE, text_scale);
}

static void draw_window_content_terminal(const struct app_window* w) {
    int32_t text_scale = ui_text_scale();
    int32_t x = w->x + 6;
    int32_t y = w->y + TITLE_H + 4;
    int32_t ww = w->w - 12;
    int32_t hh = w->h - TITLE_H - 9;
    int32_t line_h = 8 * text_scale;

    fill_rect(x, y, ww, hh, COLOR_BLACK);
    stroke_rect(x, y, ww, hh, COLOR_BORDER);

    char path[TERM_LINE_MAX];
    char prompt[TERM_LINE_MAX];
    vfs_build_path(terminal_state.cwd_node, path);
    text_copy(prompt, TERM_LINE_MAX, "PATH ");
    text_append(prompt, TERM_LINE_MAX, path);
    draw_text5x7(x + 6, y + 4, prompt, COLOR_ICON_TERMINAL, text_scale);

    int32_t input_y = y + hh - line_h - 6;
    fill_rect(x + 4, input_y - 2, ww - 8, line_h + 3, COLOR_PANEL_BG);
    stroke_rect(x + 4, input_y - 2, ww - 8, line_h + 3, COLOR_BORDER);

    char input_line[TERM_LINE_MAX];
    text_copy(input_line, TERM_LINE_MAX, "W ");
    text_append(input_line, TERM_LINE_MAX, path);
    text_append(input_line, TERM_LINE_MAX, " > ");
    text_append(input_line, TERM_LINE_MAX, terminal_state.input);
    text_append(input_line, TERM_LINE_MAX, "_");
    draw_text5x7(x + 7, input_y, input_line, COLOR_BORDER, text_scale);

    int32_t history_y = y + 16;
    int32_t history_h = input_y - history_y - 2;
    int32_t rows = history_h / line_h;
    if (rows < 1) {
        rows = 1;
    }

    int32_t start = (int32_t)terminal_state.line_count - rows;
    if (start < 0) {
        start = 0;
    }
    int32_t row = 0;
    for (int32_t i = start; i < (int32_t)terminal_state.line_count; ++i) {
        draw_text5x7(x + 7, history_y + row * line_h, terminal_state.lines[(uint32_t)i], COLOR_ICON_TERMINAL, text_scale);
        row++;
    }
}

static void draw_window_content_settings(const struct app_window* w) {
    int32_t text_scale = ui_text_scale();
    int32_t x = w->x + 6;
    int32_t y = w->y + TITLE_H + 4;
    int32_t ww = w->w - 12;
    int32_t hh = w->h - TITLE_H - 9;

    fill_rect(x, y, ww, hh, COLOR_WINDOW_BG);
    stroke_rect(x, y, ww, hh, COLOR_BORDER);

    int32_t nav_w = clamp_i32(ww / 4, 44, 112);
    fill_rect(x + 2, y + 2, nav_w - 4, hh - 4, COLOR_PANEL_BG);
    stroke_rect(x + 2, y + 2, nav_w - 4, hh - 4, COLOR_BORDER);

    draw_text5x7(x + 7, y + 8, "SYSTEM", COLOR_ACCENT, text_scale);
    draw_text5x7(x + 7, y + 20, "NETWORK", COLOR_BORDER, text_scale);
    draw_text5x7(x + 7, y + 32, "DISPLAY", COLOR_BORDER, text_scale);
    draw_text5x7(x + 7, y + 44, "AUDIO", COLOR_BORDER, text_scale);
    draw_text5x7(x + 7, y + 56, "ABOUT", COLOR_BORDER, text_scale);

    int32_t cx = x + nav_w + 4;
    int32_t cw = ww - nav_w - 6;
    int32_t card_h = 24;
    char brightness_text[4];
    u8_to_dec_text(state.settings_brightness, brightness_text);
    uint8_t fw80_allow = 1u;
    uint8_t fw443_allow = 1u;
    uint8_t link_up = 0u;
    (void)net_firewall_get_port(80u, &fw80_allow);
    (void)net_firewall_get_port(443u, &fw443_allow);
    (void)net_get_link(&link_up);

    /* Wi-Fi card */
    fill_rect(cx, y + 2, cw, card_h, COLOR_PANEL_BG);
    stroke_rect(cx, y + 2, cw, card_h, COLOR_BORDER);
    draw_text5x7(cx + 6, y + 8, "WIFI ADAPTER", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 6, y + 16, state.settings_wifi_on ? "STATE ON" : "STATE OFF", COLOR_BORDER, text_scale);
    set_wifi_btn_w = 52;
    set_wifi_btn_h = 12;
    set_wifi_btn_x = cx + cw - set_wifi_btn_w - 8;
    set_wifi_btn_y = y + 7;
    draw_button(set_wifi_btn_x, set_wifi_btn_y, set_wifi_btn_w, set_wifi_btn_h, state.settings_wifi_on ? "ON" : "OFF", state.settings_wifi_on);

    /* Network card (advanced controls) */
    fill_rect(cx, y + 30, cw, card_h + 14, COLOR_PANEL_BG);
    stroke_rect(cx, y + 30, cw, card_h + 14, COLOR_BORDER);
    draw_text5x7(cx + 6, y + 36, "NETWORK ADVANCED", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 6, y + 44, link_up ? "LINK ONLINE" : "LINK OFFLINE", COLOR_BORDER, text_scale);
    char fw_line[TERM_LINE_MAX];
    text_copy(fw_line, TERM_LINE_MAX, "80 ");
    text_append(fw_line, TERM_LINE_MAX, fw80_allow ? "ALLOW  " : "BLOCK  ");
    text_append(fw_line, TERM_LINE_MAX, "443 ");
    text_append(fw_line, TERM_LINE_MAX, fw443_allow ? "ALLOW" : "BLOCK");
    draw_text5x7(cx + 62, y + 44, fw_line, COLOR_BORDER, text_scale);
    draw_text5x7(cx + 6, y + 52, settings_net_status_line, COLOR_BORDER, text_scale);

    int32_t btn_gap = 2;
    int32_t row_btn_y = y + 57;
    set_net_up_btn_w = 18;
    set_net_up_btn_h = 10;
    set_net_down_btn_w = 20;
    set_net_down_btn_h = 10;
    set_dns_dev_btn_w = 20;
    set_dns_dev_btn_h = 10;
    set_dns_openai_btn_w = 20;
    set_dns_openai_btn_h = 10;
    set_fw80_btn_w = 16;
    set_fw80_btn_h = 10;
    set_fw443_btn_w = 20;
    set_fw443_btn_h = 10;
    int32_t total_net_btn_w = set_net_up_btn_w + set_net_down_btn_w + set_dns_dev_btn_w +
                              set_dns_openai_btn_w + set_fw80_btn_w + set_fw443_btn_w + btn_gap * 5;
    int32_t net_btn_x = cx + cw - total_net_btn_w - 6;
    if (net_btn_x < cx + 6) {
        net_btn_x = cx + 6;
    }

    set_net_up_btn_x = net_btn_x;
    set_net_up_btn_y = row_btn_y;
    draw_button(set_net_up_btn_x, set_net_up_btn_y, set_net_up_btn_w, set_net_up_btn_h, "UP", link_up != 0u);

    set_net_down_btn_x = set_net_up_btn_x + set_net_up_btn_w + btn_gap;
    set_net_down_btn_y = row_btn_y;
    draw_button(set_net_down_btn_x, set_net_down_btn_y, set_net_down_btn_w, set_net_down_btn_h, "OFF", link_up == 0u);

    set_dns_dev_btn_x = set_net_down_btn_x + set_net_down_btn_w + btn_gap;
    set_dns_dev_btn_y = row_btn_y;
    draw_button(set_dns_dev_btn_x, set_dns_dev_btn_y, set_dns_dev_btn_w, set_dns_dev_btn_h, "DEV", false);

    set_dns_openai_btn_x = set_dns_dev_btn_x + set_dns_dev_btn_w + btn_gap;
    set_dns_openai_btn_y = row_btn_y;
    draw_button(set_dns_openai_btn_x, set_dns_openai_btn_y, set_dns_openai_btn_w, set_dns_openai_btn_h, "OAI", false);

    set_fw80_btn_x = set_dns_openai_btn_x + set_dns_openai_btn_w + btn_gap;
    set_fw80_btn_y = row_btn_y;
    draw_button(set_fw80_btn_x, set_fw80_btn_y, set_fw80_btn_w, set_fw80_btn_h, "80", fw80_allow != 0u);

    set_fw443_btn_x = set_fw80_btn_x + set_fw80_btn_w + btn_gap;
    set_fw443_btn_y = row_btn_y;
    draw_button(set_fw443_btn_x, set_fw443_btn_y, set_fw443_btn_w, set_fw443_btn_h, "443", fw443_allow != 0u);

    /* Legacy link button not used in advanced layout */
    set_link_btn_x = set_link_btn_y = set_link_btn_w = set_link_btn_h = 0;

    /* Brightness card */
    fill_rect(cx, y + 70, cw, card_h, COLOR_PANEL_BG);
    stroke_rect(cx, y + 70, cw, card_h, COLOR_BORDER);
    draw_text5x7(cx + 6, y + 76, "BRIGHTNESS", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 76, y + 76, brightness_text, COLOR_BORDER, text_scale);
    int32_t bar_w = cw - 80;
    if (bar_w < 20) {
        bar_w = 20;
    }
    fill_rect(cx + 6, y + 83, bar_w, 3, COLOR_TITLE_INACTIVE);
    int32_t fill_w = (bar_w * (int32_t)state.settings_brightness) / 100;
    fill_w = clamp_i32(fill_w, 1, bar_w);
    fill_rect(cx + 6, y + 83, fill_w, 3, COLOR_ACCENT);
    fill_rect(cx + 6 + fill_w - 1, y + 81, 3, 7, COLOR_ACCENT);
    set_bminus_btn_w = 16;
    set_bminus_btn_h = 11;
    set_bminus_btn_x = cx + cw - 36;
    set_bminus_btn_y = y + 74;
    draw_button(set_bminus_btn_x, set_bminus_btn_y, set_bminus_btn_w, set_bminus_btn_h, "-", false);
    set_bplus_btn_w = 16;
    set_bplus_btn_h = 11;
    set_bplus_btn_x = cx + cw - 18;
    set_bplus_btn_y = y + 74;
    draw_button(set_bplus_btn_x, set_bplus_btn_y, set_bplus_btn_w, set_bplus_btn_h, "+", true);

    /* Wallpaper card */
    fill_rect(cx, y + 97, cw, card_h, COLOR_PANEL_BG);
    stroke_rect(cx, y + 97, cw, card_h, COLOR_BORDER);
    draw_text5x7(cx + 6, y + 103, "WALLPAPER", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 56, y + 103,
                 (state.wallpaper_index % WALLPAPER_COUNT) == 0u ? "PHOTO" :
                 (state.wallpaper_index % WALLPAPER_COUNT) == 1u ? "SUNSET" : "MINIMAL",
                 COLOR_BORDER, text_scale);
    set_wp_prev_btn_w = 30;
    set_wp_prev_btn_h = 12;
    set_wp_prev_btn_x = cx + cw - 68;
    set_wp_prev_btn_y = y + 102;
    draw_button(set_wp_prev_btn_x, set_wp_prev_btn_y, set_wp_prev_btn_w, set_wp_prev_btn_h, "PREV", false);
    set_wp_next_btn_w = 30;
    set_wp_next_btn_h = 12;
    set_wp_next_btn_x = cx + cw - 34;
    set_wp_next_btn_y = y + 102;
    draw_button(set_wp_next_btn_x, set_wp_next_btn_y, set_wp_next_btn_w, set_wp_next_btn_h, "NEXT", true);

    /* Help strip */
    fill_rect(cx, y + 123, cw, 20, COLOR_PANEL_BG);
    stroke_rect(cx, y + 123, cw, 20, COLOR_BORDER);
    draw_text5x7(cx + 6, y + 126, "F WIFI C LINK B/N BRIGHT M WALL U/O/P SCALE", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 6, y + 134, "T LAYOUT G/Y BLUR X/V MOTION Z QUALITY", COLOR_BORDER, text_scale);

    /* Display scale + resolution card */
    fill_rect(cx, y + 146, cw, card_h + 8, COLOR_PANEL_BG);
    stroke_rect(cx, y + 146, cw, card_h + 8, COLOR_BORDER);
    draw_text5x7(cx + 6, y + 152, "DISPLAY SCALE", COLOR_BORDER, text_scale);
    char scale_text[8];
    u32_to_dec_text((uint32_t)desktop_scale(), scale_text, sizeof(scale_text));
    draw_text5x7(cx + 92, y + 152, scale_text, COLOR_BORDER, text_scale);
    draw_text5x7(cx + 102, y + 152, "X", COLOR_BORDER, text_scale);
    char rx[12];
    char ry[12];
    u32_to_dec_text(fb_width, rx, sizeof(rx));
    u32_to_dec_text(fb_height, ry, sizeof(ry));
    char res_line[TERM_LINE_MAX];
    text_copy(res_line, TERM_LINE_MAX, "RES ");
    text_append(res_line, TERM_LINE_MAX, rx);
    text_append(res_line, TERM_LINE_MAX, "X");
    text_append(res_line, TERM_LINE_MAX, ry);
    draw_text5x7(cx + 6, y + 162, res_line, COLOR_BORDER, text_scale);
    set_scale_minus_btn_w = 16;
    set_scale_minus_btn_h = 11;
    set_scale_minus_btn_x = cx + cw - 58;
    set_scale_minus_btn_y = y + 150;
    draw_button(set_scale_minus_btn_x, set_scale_minus_btn_y, set_scale_minus_btn_w, set_scale_minus_btn_h, "-", false);
    set_scale_plus_btn_w = 16;
    set_scale_plus_btn_h = 11;
    set_scale_plus_btn_x = cx + cw - 40;
    set_scale_plus_btn_y = y + 150;
    draw_button(set_scale_plus_btn_x, set_scale_plus_btn_y, set_scale_plus_btn_w, set_scale_plus_btn_h, "+", true);
    set_scale_auto_btn_w = 20;
    set_scale_auto_btn_h = 11;
    set_scale_auto_btn_x = cx + cw - 22;
    set_scale_auto_btn_y = y + 150;
    draw_button(set_scale_auto_btn_x, set_scale_auto_btn_y, set_scale_auto_btn_w, set_scale_auto_btn_h, "FIT", ui_scale_override == 0u);

    /* Security card */
    fill_rect(cx, y + 182, cw, card_h + 10, COLOR_PANEL_BG);
    stroke_rect(cx, y + 182, cw, card_h + 10, COLOR_BORDER);
    draw_text5x7(cx + 6, y + 188, "SECURITY LOCK", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 6, y + 198, state.settings_lock_enabled ? "LOCK ENABLED" : "LOCK DISABLED", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 104, y + 188, "PIN", COLOR_BORDER, text_scale);
    char pin_text[12];
    u32_to_dec_text((uint32_t)state.settings_lock_pin, pin_text, sizeof(pin_text));
    draw_text5x7(cx + 124, y + 188, pin_text, COLOR_BORDER, text_scale);

    set_lock_btn_w = 40;
    set_lock_btn_h = 12;
    set_lock_btn_x = cx + cw - 42;
    set_lock_btn_y = y + 186;
    draw_button(set_lock_btn_x, set_lock_btn_y, set_lock_btn_w, set_lock_btn_h, state.settings_lock_enabled ? "ON" : "OFF", state.settings_lock_enabled);

    set_pin_minus_btn_w = 16;
    set_pin_minus_btn_h = 12;
    set_pin_minus_btn_x = cx + cw - 76;
    set_pin_minus_btn_y = y + 186;
    draw_button(set_pin_minus_btn_x, set_pin_minus_btn_y, set_pin_minus_btn_w, set_pin_minus_btn_h, "-", false);

    set_pin_plus_btn_w = 16;
    set_pin_plus_btn_h = 12;
    set_pin_plus_btn_x = cx + cw - 58;
    set_pin_plus_btn_y = y + 186;
    draw_button(set_pin_plus_btn_x, set_pin_plus_btn_y, set_pin_plus_btn_w, set_pin_plus_btn_h, "+", true);

    set_lock_now_btn_w = 52;
    set_lock_now_btn_h = 12;
    set_lock_now_btn_x = cx + cw - 56;
    set_lock_now_btn_y = y + 200;
    draw_button(set_lock_now_btn_x, set_lock_now_btn_y, set_lock_now_btn_w, set_lock_now_btn_h, "LOCK NOW", false);

    /* Shell + effects card */
    fill_rect(cx, y + 216, cw, card_h + 12, COLOR_PANEL_BG);
    stroke_rect(cx, y + 216, cw, card_h + 12, COLOR_BORDER);
    draw_text5x7(cx + 6, y + 222, "SHELL + EFFECTS", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 6, y + 232, state.shell_layout == SHELL_LAYOUT_TASKBAR ? "LAYOUT TASKBAR" : "LAYOUT DOCK", COLOR_BORDER, text_scale);

    char blur_t[4];
    char motion_t[4];
    char quality_t[4];
    u8_to_dec_text(state.ui_blur_level, blur_t);
    u8_to_dec_text(state.ui_motion_level, motion_t);
    u8_to_dec_text(state.ui_effects_quality, quality_t);
    draw_text5x7(cx + 86, y + 222, "BL", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 98, y + 222, blur_t, COLOR_BORDER, text_scale);
    draw_text5x7(cx + 110, y + 222, "MO", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 124, y + 222, motion_t, COLOR_BORDER, text_scale);
    draw_text5x7(cx + 136, y + 222, "Q", COLOR_BORDER, text_scale);
    draw_text5x7(cx + 144, y + 222, quality_t, COLOR_BORDER, text_scale);

    set_shell_layout_btn_w = 46;
    set_shell_layout_btn_h = 11;
    set_shell_layout_btn_x = cx + 6;
    set_shell_layout_btn_y = y + 236;
    draw_button(set_shell_layout_btn_x, set_shell_layout_btn_y, set_shell_layout_btn_w, set_shell_layout_btn_h,
                state.shell_layout == SHELL_LAYOUT_TASKBAR ? "TASK" : "DOCK", true);

    set_blur_minus_btn_w = 14;
    set_blur_minus_btn_h = 11;
    set_blur_minus_btn_x = cx + 58;
    set_blur_minus_btn_y = y + 236;
    draw_button(set_blur_minus_btn_x, set_blur_minus_btn_y, set_blur_minus_btn_w, set_blur_minus_btn_h, "-", false);

    set_blur_plus_btn_w = 14;
    set_blur_plus_btn_h = 11;
    set_blur_plus_btn_x = cx + 74;
    set_blur_plus_btn_y = y + 236;
    draw_button(set_blur_plus_btn_x, set_blur_plus_btn_y, set_blur_plus_btn_w, set_blur_plus_btn_h, "+", true);

    set_motion_minus_btn_w = 14;
    set_motion_minus_btn_h = 11;
    set_motion_minus_btn_x = cx + 94;
    set_motion_minus_btn_y = y + 236;
    draw_button(set_motion_minus_btn_x, set_motion_minus_btn_y, set_motion_minus_btn_w, set_motion_minus_btn_h, "-", false);

    set_motion_plus_btn_w = 14;
    set_motion_plus_btn_h = 11;
    set_motion_plus_btn_x = cx + 110;
    set_motion_plus_btn_y = y + 236;
    draw_button(set_motion_plus_btn_x, set_motion_plus_btn_y, set_motion_plus_btn_w, set_motion_plus_btn_h, "+", true);

    set_quality_btn_w = 34;
    set_quality_btn_h = 11;
    set_quality_btn_x = cx + 130;
    set_quality_btn_y = y + 236;
    draw_button(set_quality_btn_x, set_quality_btn_y, set_quality_btn_w, set_quality_btn_h, "QUALITY", state.ui_effects_quality >= 2u);
}

static bool code_token_match(const char* token, uint8_t len, const char* kw) {
    if (token == 0 || kw == 0) {
        return false;
    }
    uint8_t i = 0u;
    while (i < len && kw[i] != '\0') {
        char a = token[i];
        if (a >= 'a' && a <= 'z') {
            a = (char)(a - ('a' - 'A'));
        }
        if (a != kw[i]) {
            return false;
        }
        i++;
    }
    return i == len && kw[i] == '\0';
}

static bool code_token_is_keyword(const char* token, uint8_t len) {
    static const char* keywords[] = {
        "INT", "VOID", "RETURN", "IF", "ELSE", "FOR", "WHILE",
        "STRUCT", "STATIC", "CHAR", "BOOL", "INCLUDE", "DEFINE",
        "TRUE", "FALSE", "BREAK", "CONTINUE", "SWITCH", "CASE",
    };

    for (uint32_t i = 0u; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        if (code_token_match(token, len, keywords[i])) {
            return true;
        }
    }
    return false;
}

static bool code_char_is_word(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return true;
    }
    if (ch >= 'a' && ch <= 'z') {
        return true;
    }
    if (ch >= '0' && ch <= '9') {
        return true;
    }
    return ch == '_' || ch == '.';
}

static void draw_code_line(int32_t x, int32_t y, const char* line, int32_t line_len, int32_t max_cols) {
    int32_t cx = 0;
    int32_t i = 0;
    while (i < line_len && cx < max_cols) {
        char ch = line[i];

        if (ch == ' ' || ch == '\t') {
            char s[2] = {ch == '\t' ? ' ' : ch, '\0'};
            draw_text5x7(x + cx * 6, y, s, COLOR_TITLE_INACTIVE, 1);
            cx++;
            i++;
            continue;
        }

        if (ch == '/' && i + 1 < line_len && line[i + 1] == '/') {
            while (i < line_len && cx < max_cols) {
                char c = line[i++];
                char s[2] = {c, '\0'};
                draw_text5x7(x + cx * 6, y, s, COLOR_TITLE_INACTIVE, 1);
                cx++;
            }
            break;
        }

        if (code_char_is_word(ch)) {
            int32_t j = i;
            while (j < line_len && code_char_is_word(line[j])) {
                j++;
            }
            uint8_t tok_len = (uint8_t)(j - i);
            uint8_t color = code_token_is_keyword(&line[i], tok_len) ? COLOR_PANEL_BG : COLOR_WINDOW_BG;
            for (int32_t k = i; k < j && cx < max_cols; ++k) {
                char s[2] = {line[k], '\0'};
                draw_text5x7(x + cx * 6, y, s, color, 1);
                cx++;
            }
            i = j;
            continue;
        }

        char s[2] = {ch, '\0'};
        uint8_t punctuation = (ch == '{' || ch == '}' || ch == '(' || ch == ')' || ch == '[' || ch == ']')
            ? COLOR_PANEL_BG
            : COLOR_WINDOW_BG;
        draw_text5x7(x + cx * 6, y, s, punctuation, 1);
        cx++;
        i++;
    }
}

static void draw_window_content_notepad(const struct app_window* w) {
    int32_t x = w->x + 6;
    int32_t y = w->y + TITLE_H + 4;
    int32_t ww = w->w - 12;
    int32_t hh = w->h - TITLE_H - 9;

    fill_rect(x, y, ww, hh, COLOR_WINDOW_BG);
    stroke_rect(x, y, ww, hh, COLOR_BORDER);

    int32_t activity_w = 18;
    int32_t explorer_w = clamp_i32(ww / 5, 56, 100);

    fill_rect(x + 1, y + 1, activity_w, hh - 2, COLOR_TASKBAR);
    stroke_rect(x + 1, y + 1, activity_w, hh - 2, COLOR_BORDER);
    fill_rect(x + 5, y + 10, 8, 2, COLOR_WINDOW_BG);
    fill_rect(x + 5, y + 16, 8, 2, COLOR_WINDOW_BG);
    fill_rect(x + 5, y + 22, 8, 2, COLOR_WINDOW_BG);

    int32_t ex = x + activity_w + 3;
    fill_rect(ex, y + 1, explorer_w, hh - 2, COLOR_PANEL_BG);
    stroke_rect(ex, y + 1, explorer_w, hh - 2, COLOR_BORDER);
    draw_text5x7(ex + 5, y + 7, "EXPLORER", COLOR_BORDER, 1);
    draw_text5x7(ex + 5, y + 18, "ROOT", COLOR_BORDER, 1);
    draw_text5x7(ex + 10, y + 28, "DESKTOP", COLOR_BORDER, 1);
    draw_text5x7(ex + 10, y + 38, notepad_state.file_name[0] ? notepad_state.file_name : "MAIN.WV", COLOR_BLACK, 1);
    draw_text5x7(ex + 10, y + 48, "SYSTEM.LOG", COLOR_BORDER, 1);

    int32_t cx0 = ex + explorer_w + 3;
    int32_t cw = ww - (cx0 - x) - 2;
    fill_rect(cx0, y + 1, cw, hh - 2, COLOR_WINDOW_BG);
    stroke_rect(cx0, y + 1, cw, hh - 2, COLOR_BORDER);

    fill_rect(cx0 + 1, y + 1, cw - 2, 13, COLOR_TASKBAR);
    draw_text5x7(cx0 + 5, y + 5, "WEVOA CODE", COLOR_WINDOW_BG, 1);
    draw_text5x7(cx0 + 70, y + 5, notepad_state.file_name[0] ? notepad_state.file_name : "MAIN.WV", COLOR_PANEL_BG, 1);
    draw_text5x7(cx0 + cw - 36, y + 5, notepad_state.dirty ? "MOD" : "SAVED", COLOR_WINDOW_BG, 1);

    note_btn_load_w = 30;
    note_btn_load_h = 10;
    note_btn_load_x = cx0 + cw - 100;
    note_btn_load_y = y + 16;
    draw_button(note_btn_load_x, note_btn_load_y, note_btn_load_w, note_btn_load_h, "LOAD", false);
    note_btn_save_w = 30;
    note_btn_save_h = 10;
    note_btn_save_x = cx0 + cw - 68;
    note_btn_save_y = y + 16;
    draw_button(note_btn_save_x, note_btn_save_y, note_btn_save_w, note_btn_save_h, "SAVE", true);
    note_btn_new_w = 30;
    note_btn_new_h = 10;
    note_btn_new_x = cx0 + cw - 36;
    note_btn_new_y = y + 16;
    draw_button(note_btn_new_x, note_btn_new_y, note_btn_new_w, note_btn_new_h, "NEW", false);

    int32_t editor_x = cx0 + 2;
    int32_t editor_y = y + 29;
    int32_t editor_w = cw - 4;
    int32_t editor_h = hh - 44;
    int32_t gutter_w = 24;
    fill_rect(editor_x, editor_y, editor_w, editor_h, COLOR_BLACK);
    stroke_rect(editor_x, editor_y, editor_w, editor_h, COLOR_BORDER);
    fill_rect(editor_x + 1, editor_y + 1, gutter_w, editor_h - 2, COLOR_TASKBAR);
    fill_rect(editor_x + gutter_w + 1, editor_y + 1, 1, editor_h - 2, COLOR_BORDER);

    int32_t text_x = editor_x + gutter_w + 4;
    int32_t text_y = editor_y + 4;
    int32_t cols = (editor_w - gutter_w - 8) / 6;
    int32_t rows = (editor_h - 8) / 8;
    if (cols < 1) {
        cols = 1;
    }
    if (rows < 1) {
        rows = 1;
    }

    char line_buf[TERM_LINE_MAX];
    int32_t line_len = 0;
    int32_t row = 0;
    int32_t caret_col = 0;
    int32_t caret_row = 0;
    uint32_t logical_line = 1u;

    for (uint32_t i = 0u; i <= notepad_state.len; ++i) {
        char ch = (i < notepad_state.len) ? notepad_state.buffer[i] : '\n';
        bool flush = (ch == '\n') || (line_len >= cols) || (line_len >= (int32_t)(TERM_LINE_MAX - 1u)) || (i == notepad_state.len);
        if (!flush) {
            line_buf[line_len++] = ch;
            continue;
        }

        if (row < rows) {
            char n[12];
            u32_to_dec_text(logical_line, n, sizeof(n));
            draw_text5x7(editor_x + 4, text_y + row * 8, n, COLOR_PANEL_BG, 1);
            draw_code_line(text_x, text_y + row * 8, line_buf, line_len, cols);
        }
        row++;
        logical_line++;
        line_len = 0;

        if (ch != '\n' && i < notepad_state.len) {
            line_buf[line_len++] = ch;
        }
    }

    int32_t cc = 0;
    int32_t rr = 0;
    for (uint32_t i = 0u; i < notepad_state.len; ++i) {
        char ch = notepad_state.buffer[i];
        if (ch == '\n') {
            rr++;
            cc = 0;
            continue;
        }
        if (cc >= cols) {
            rr++;
            cc = 0;
        }
        cc++;
    }
    caret_row = clamp_i32(rr, 0, rows - 1);
    caret_col = clamp_i32(cc, 0, cols - 1);

    int32_t caret_x = text_x + caret_col * 6;
    int32_t caret_y = text_y + caret_row * 8;
    draw_text5x7(caret_x, caret_y, "_", COLOR_PANEL_BG, 1);

    int32_t status_y = editor_y + editor_h + 2;
    fill_rect(cx0 + 2, status_y, cw - 4, 10, COLOR_TASKBAR);
    stroke_rect(cx0 + 2, status_y, cw - 4, 10, COLOR_BORDER);
    char status[TERM_LINE_MAX];
    char len_txt[12];
    u32_to_dec_text(notepad_state.len, len_txt, sizeof(len_txt));
    text_copy(status, TERM_LINE_MAX, "WEVOA-C  UTF-8  LEN ");
    text_append(status, TERM_LINE_MAX, len_txt);
    text_append(status, TERM_LINE_MAX, notepad_state.dirty ? "  MOD" : "  SAVED");
    draw_text5x7(cx0 + 6, status_y + 2, status, COLOR_WINDOW_BG, 1);
}

static void draw_window_content_calculator(const struct app_window* w) {
    int32_t text_scale = ui_text_scale();
    int32_t x = w->x + 6;
    int32_t y = w->y + TITLE_H + 4;
    int32_t ww = w->w - 12;
    int32_t hh = w->h - TITLE_H - 9;
    fill_rect(x, y, ww, hh, COLOR_WINDOW_BG);
    stroke_rect(x, y, ww, hh, COLOR_BORDER);

    fill_rect(x + 4, y + 4, ww - 8, 16, COLOR_BLACK);
    stroke_rect(x + 4, y + 4, ww - 8, 16, COLOR_BORDER);
    draw_text5x7(x + 8, y + 10, calc_state.expr_len == 0u ? "0" : calc_state.expr, COLOR_WINDOW_BG, text_scale);

    static const char* labels[16] = {
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "C", "0", "=", "+"
    };
    int32_t gx = x + 6;
    int32_t gy = y + 24;
    int32_t gw = (ww - 18) / 4;
    int32_t gh = (hh - 32) / 4;
    if (gw < 14) {
        gw = 14;
    }
    if (gh < 12) {
        gh = 12;
    }
    for (uint32_t i = 0u; i < 16u; ++i) {
        int32_t col = (int32_t)(i % 4u);
        int32_t row = (int32_t)(i / 4u);
        int32_t bx = gx + col * (gw + 2);
        int32_t by = gy + row * (gh + 2);
        calc_btn_x[i] = bx;
        calc_btn_y[i] = by;
        calc_btn_w[i] = gw;
        calc_btn_h[i] = gh;
        draw_button(bx, by, gw, gh, labels[i], (i == 12u || i == 14u));
    }
}

static void draw_window_content_image_viewer(const struct app_window* w) {
    int32_t text_scale = ui_text_scale();
    int32_t x = w->x + 6;
    int32_t y = w->y + TITLE_H + 4;
    int32_t ww = w->w - 12;
    int32_t hh = w->h - TITLE_H - 9;
    fill_rect(x, y, ww, hh, COLOR_WINDOW_BG);
    stroke_rect(x, y, ww, hh, COLOR_BORDER);

    fill_rect(x + 4, y + 4, ww - 8, 14, COLOR_PANEL_BG);
    stroke_rect(x + 4, y + 4, ww - 8, 14, COLOR_BORDER);
    draw_text5x7(x + 8, y + 8, "SAMPLE IMAGE", COLOR_BORDER, text_scale);

    int32_t px = x + 6;
    int32_t py = y + 22;
    int32_t pw = ww - 12;
    int32_t ph = hh - 28;
    fill_rect(px, py, pw, ph, COLOR_DESKTOP);
    stroke_rect(px, py, pw, ph, COLOR_BORDER);

    for (int32_t row = 0; row < ph; ++row) {
        uint8_t c = (row < ph / 3) ? COLOR_WINDOW_BG :
                    (row < (ph * 2) / 3) ? COLOR_DESKTOP :
                    COLOR_TASKBAR;
        fill_rect(px + 1, py + row, pw - 2, 1, c);
    }

    int32_t mx = px + pw / 3;
    int32_t my = py + ph * 2 / 3;
    fill_rect(mx - 40, my - 28, 80, 28, COLOR_TITLE_INACTIVE);
    fill_rect(mx + 30, my - 24, 62, 24, COLOR_BORDER);
    fill_rect(px + pw - 50, py + 16, 22, 22, COLOR_ICON_FOLDER);
    stroke_rect(px + pw - 50, py + 16, 22, 22, COLOR_BORDER);
    draw_text5x7(px + 8, py + ph - 12, "WEVOA_WALLPAPER.PNG", COLOR_BORDER, text_scale);
}

static void draw_window_content_video_player(const struct app_window* w) {
    int32_t text_scale = ui_text_scale();
    int32_t x = w->x + 6;
    int32_t y = w->y + TITLE_H + 4;
    int32_t ww = w->w - 12;
    int32_t hh = w->h - TITLE_H - 9;
    fill_rect(x, y, ww, hh, COLOR_WINDOW_BG);
    stroke_rect(x, y, ww, hh, COLOR_BORDER);

    int32_t vx = x + 4;
    int32_t vy = y + 4;
    int32_t vw = ww - 8;
    int32_t vh = hh - 34;
    fill_rect(vx, vy, vw, vh, COLOR_BLACK);
    stroke_rect(vx, vy, vw, vh, COLOR_BORDER);

    uint8_t frame = video_state.frame_index;
    int32_t band_w = clamp_i32(vw / 6, 12, 120);
    int32_t band_x = vx + 2 + ((int32_t)frame * 3) % clamp_i32(vw - band_w - 4, 1, vw);
    fill_rect(vx + 2, vy + 2, vw - 4, vh - 4, COLOR_BLACK);
    fill_rect(band_x, vy + 4, band_w, vh - 8, COLOR_ACCENT);
    fill_rect(vx + 8, vy + 8, vw / 3, 10, COLOR_PANEL_BG);
    draw_text5x7(vx + 10, vy + 11, "WEVOA DEMO VIDEO", COLOR_BORDER, text_scale);

    int32_t progress_w = vw - 12;
    int32_t progress = (progress_w * (int32_t)frame) / 120;
    if (progress < 1) {
        progress = 1;
    }
    fill_rect(vx + 6, vy + vh - 14, progress_w, 4, COLOR_TITLE_INACTIVE);
    fill_rect(vx + 6, vy + vh - 14, progress, 4, COLOR_ACCENT);

    int32_t cy = y + hh - 24;
    video_btn_play_w = 34;
    video_btn_play_h = 12;
    video_btn_play_x = x + 6;
    video_btn_play_y = cy;
    draw_button(video_btn_play_x, video_btn_play_y, video_btn_play_w, video_btn_play_h, "PLAY", video_state.playing);

    video_btn_pause_w = 34;
    video_btn_pause_h = 12;
    video_btn_pause_x = video_btn_play_x + 36;
    video_btn_pause_y = cy;
    draw_button(video_btn_pause_x, video_btn_pause_y, video_btn_pause_w, video_btn_pause_h, "PAUSE", !video_state.playing);

    video_btn_stop_w = 34;
    video_btn_stop_h = 12;
    video_btn_stop_x = video_btn_pause_x + 36;
    video_btn_stop_y = cy;
    draw_button(video_btn_stop_x, video_btn_stop_y, video_btn_stop_w, video_btn_stop_h, "STOP", false);

    draw_text5x7(video_btn_stop_x + 42, cy + 3, video_state.playing ? "PLAYING" : "PAUSED", COLOR_BORDER, text_scale);
}

static void draw_browser_preview_panel(int32_t x, int32_t y, int32_t w, int32_t h) {
    int32_t text_scale = ui_text_scale();
    if (text_scale < 2) {
        text_scale = 2;
    }
    if (w < 40 || h < 30) {
        return;
    }

    fill_rect(x, y, w, h, COLOR_WINDOW_BG);
    stroke_rect(x, y, w, h, COLOR_BORDER);

    if (text_contains(browser_state.page_title, "DEVDOPZ")) {
        int32_t hero_y = y + 22;
        int32_t hero_h = clamp_i32(h * 44 / 100, 36, h - 44);
        int32_t left_w = (w - 18) / 2;
        int32_t right_w = w - left_w - 18;
        int32_t cards_y = hero_y + hero_h + 6;
        int32_t cards_h = h - (cards_y - y) - 6;

        fill_rect(x + 4, y + 4, w - 8, 14, COLOR_TITLE_ACTIVE);
        draw_text5x7(x + 8, y + 7, "DEVDOPZ.COM", COLOR_WINDOW_BG, text_scale);
        draw_text5x7(x + 86, y + 7, "LANDING PREVIEW", COLOR_WINDOW_BG, text_scale);

        fill_rect(x + 6, hero_y, left_w, hero_h, COLOR_TITLE_INACTIVE);
        stroke_rect(x + 6, hero_y, left_w, hero_h, COLOR_BORDER);
        draw_text5x7(x + 10, hero_y + 6, "HERO IMAGE", COLOR_WINDOW_BG, text_scale);
        fill_rect(x + 12, hero_y + 14, left_w - 12, hero_h - 22, COLOR_TASKBAR_HI);
        stroke_rect(x + 12, hero_y + 14, left_w - 12, hero_h - 22, COLOR_BORDER);

        fill_rect(x + 10 + left_w, hero_y, right_w, hero_h, COLOR_PANEL_BG);
        stroke_rect(x + 10 + left_w, hero_y, right_w, hero_h, COLOR_BORDER);
        draw_text5x7(x + 14 + left_w, hero_y + 6, "CUSTOM WEB PRESENCE", COLOR_BORDER, text_scale);
        draw_text5x7(x + 14 + left_w, hero_y + 20, "BRAND SECTION", COLOR_BORDER, text_scale);
        draw_text5x7(x + 14 + left_w, hero_y + 32, "SERVICES", COLOR_BORDER, text_scale);
        draw_text5x7(x + 14 + left_w, hero_y + 44, "CONTACT CTA", COLOR_BORDER, text_scale);

        if (cards_h > 12) {
            int32_t card_w = (w - 18) / 2;
            fill_rect(x + 6, cards_y, card_w, cards_h, COLOR_PANEL_BG);
            stroke_rect(x + 6, cards_y, card_w, cards_h, COLOR_BORDER);
            draw_text5x7(x + 10, cards_y + 6, "PROJECTS", COLOR_BORDER, text_scale);
            draw_text5x7(x + 10, cards_y + 18, "SHOWCASE GRID", COLOR_BORDER, text_scale);

            fill_rect(x + 10 + card_w, cards_y, card_w, cards_h, COLOR_PANEL_BG);
            stroke_rect(x + 10 + card_w, cards_y, card_w, cards_h, COLOR_BORDER);
            draw_text5x7(x + 14 + card_w, cards_y + 6, "CONTACT", COLOR_BORDER, text_scale);
            draw_text5x7(x + 14 + card_w, cards_y + 18, "EMAIL + FORM", COLOR_BORDER, text_scale);
        }
        return;
    }

    if (text_contains(browser_state.page_title, "OPENAI")) {
        int32_t head_h = 14;
        fill_rect(x + 4, y + 4, w - 8, head_h, COLOR_TITLE_ACTIVE);
        draw_text5x7(x + 8, y + 7, "OPENAI.COM", COLOR_WINDOW_BG, text_scale);
        draw_text5x7(x + 64, y + 7, "PRODUCT OVERVIEW", COLOR_WINDOW_BG, text_scale);

        int32_t col_y = y + 22;
        int32_t col_h = h - 28;
        int32_t col_w = (w - 16) / 3;
        for (int32_t c = 0; c < 3; ++c) {
            int32_t cx = x + 4 + c * (col_w + 2);
            fill_rect(cx, col_y, col_w, col_h, COLOR_PANEL_BG);
            stroke_rect(cx, col_y, col_w, col_h, COLOR_BORDER);
            draw_text5x7(cx + 4, col_y + 6, c == 0 ? "CHAT" : (c == 1 ? "API" : "MODELS"), COLOR_BORDER, text_scale);
            draw_text5x7(cx + 4, col_y + 18, "FEATURE CARD", COLOR_BORDER, text_scale);
            draw_text5x7(cx + 4, col_y + 30, "READ MORE", COLOR_BORDER, text_scale);
        }
        return;
    }

    /* Generic page preview */
    fill_rect(x + 4, y + 4, w - 8, 11, COLOR_TITLE_INACTIVE);
    draw_text5x7(x + 8, y + 6, "WEBSITE PREVIEW", COLOR_WINDOW_BG, text_scale);
    fill_rect(x + 6, y + 20, w - 12, h - 26, COLOR_PANEL_BG);
    stroke_rect(x + 6, y + 20, w - 12, h - 26, COLOR_BORDER);
    draw_text5x7(x + 10, y + 26, "PAGE SNAPSHOT NOT AVAILABLE", COLOR_BORDER, text_scale);
    draw_text5x7(x + 10, y + 36, "TRY: DEVDOPZ.COM / OPENAI.COM", COLOR_BORDER, text_scale);
}

static void draw_window_content_browser(const struct app_window* w) {
    int32_t text_scale = ui_text_scale();
    if (text_scale < 2) {
        text_scale = 2;
    }
    int32_t x = w->x + 6;
    int32_t y = w->y + TITLE_H + 4;
    int32_t ww = w->w - 12;
    int32_t hh = w->h - TITLE_H - 9;
    fill_rect(x, y, ww, hh, COLOR_WINDOW_BG);
    stroke_rect(x, y, ww, hh, COLOR_BORDER);

    /* Top bar + tab strip */
    fill_rect(x + 2, y + 2, ww - 4, 40, COLOR_PANEL_BG);
    stroke_rect(x + 2, y + 2, ww - 4, 40, COLOR_BORDER);
    fill_rect(x + 8, y + 5, 160, 14, COLOR_WINDOW_BG);
    stroke_rect(x + 8, y + 5, 160, 14, COLOR_BORDER);
    draw_text5x7(x + 12, y + 8, "WEVOA BROWSER", COLOR_BORDER, text_scale);

    browser_home_btn_w = 34;
    browser_home_btn_h = 14;
    browser_home_btn_x = x + 6;
    browser_home_btn_y = y + 24;
    draw_button(browser_home_btn_x, browser_home_btn_y, browser_home_btn_w, browser_home_btn_h, "HOME", false);

    browser_refresh_btn_w = 30;
    browser_refresh_btn_h = 14;
    browser_refresh_btn_x = browser_home_btn_x + browser_home_btn_w + 4;
    browser_refresh_btn_y = y + 24;
    draw_button(browser_refresh_btn_x, browser_refresh_btn_y, browser_refresh_btn_w, browser_refresh_btn_h, "REF", false);

    browser_go_btn_w = 30;
    browser_go_btn_h = 14;
    browser_go_btn_x = x + ww - browser_go_btn_w - 6;
    browser_go_btn_y = y + 24;
    draw_button(browser_go_btn_x, browser_go_btn_y, browser_go_btn_w, browser_go_btn_h, "GO", true);

    browser_url_x = browser_refresh_btn_x + browser_refresh_btn_w + 5;
    browser_url_y = y + 24;
    browser_url_w = browser_go_btn_x - browser_url_x - 4;
    browser_url_h = 14;
    fill_rect(browser_url_x, browser_url_y, browser_url_w, browser_url_h, COLOR_PANEL_BG);
    stroke_rect(browser_url_x, browser_url_y, browser_url_w, browser_url_h, COLOR_BORDER);

    char url_line[TERM_LINE_MAX];
    text_copy(url_line, sizeof(url_line), browser_state.url[0] ? browser_state.url : "WEVOA/HOME");
    text_append(url_line, sizeof(url_line), "_");
    draw_text5x7(browser_url_x + 4, browser_url_y + 3, url_line, COLOR_BORDER, text_scale);

    int32_t px = x + 4;
    int32_t py = y + 46;
    int32_t pw = ww - 8;
    int32_t ph = hh - 50;
    fill_rect(px, py, pw, ph, COLOR_WINDOW_BG);
    stroke_rect(px, py, pw, ph, COLOR_BORDER);
    fill_rect(px + 2, py + 2, pw - 4, 14, COLOR_PANEL_BG);
    stroke_rect(px + 2, py + 2, pw - 4, 14, COLOR_BORDER);
    draw_text5x7(px + 6, py + 5, browser_state.page_title, COLOR_BORDER, text_scale);

    fill_rect(px + 2, py + 19, pw - 4, 24, COLOR_PANEL_BG);
    stroke_rect(px + 2, py + 19, pw - 4, 24, COLOR_BORDER);
    draw_text5x7(px + 6, py + 23, browser_state.page_line1, COLOR_BORDER, text_scale);
    draw_text5x7(px + 6, py + 34, browser_state.page_line2, COLOR_BORDER, text_scale);

    {
        int32_t preview_x = px + 6;
        int32_t preview_y = py + 46;
        int32_t preview_w = pw - 12;
        int32_t preview_h = ph - 64;
        if (preview_h > 18) {
            draw_browser_preview_panel(preview_x, preview_y, preview_w, preview_h);
        }
    }

    fill_rect(px + 6, py + ph - 16, pw - 12, 10, COLOR_PANEL_BG);
    stroke_rect(px + 6, py + ph - 16, pw - 12, 10, COLOR_BORDER);
    draw_text5x7(px + 8, py + ph - 13, browser_state.status_line, COLOR_BORDER, text_scale);
}

/**
 * @brief Draws the title bar controls for the given window.
 *
 * @param app The index of the window in the state.windows array.
 * @param w The window to draw the controls for.
 * @param active Whether the window is active or not.
 *
 * This function draws the minimize, maximize, and close buttons in the title bar of the given window.
 * It also draws the background of the title bar as well as the border of the window.
 */
static void draw_window_title_controls(uint32_t app, const struct app_window* w, bool active) {
    int32_t btn_w = 11;
    int32_t btn_h = 9;
    int32_t gap = 2;
    int32_t y = w->y + 3;
    int32_t close_x = w->x + w->w - 4 - btn_w;
    int32_t max_x = close_x - gap - btn_w;
    int32_t min_x = max_x - gap - btn_w;

    win_btn_min_x[app] = min_x;
    win_btn_min_y[app] = y;
    win_btn_min_w[app] = btn_w;
    win_btn_min_h[app] = btn_h;
    win_btn_max_x[app] = max_x;
    win_btn_max_y[app] = y;
    win_btn_max_w[app] = btn_w;
    win_btn_max_h[app] = btn_h;
    win_btn_close_x[app] = close_x;
    win_btn_close_y[app] = y;
    win_btn_close_w[app] = btn_w;
    win_btn_close_h[app] = btn_h;

    draw_glass_rect(min_x, y, btn_w, btn_h, active ? COLOR_PANEL_BG : COLOR_TITLE_INACTIVE, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_glass_rect(max_x, y, btn_w, btn_h, active ? COLOR_PANEL_BG : COLOR_TITLE_INACTIVE, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_glass_rect(close_x, y, btn_w, btn_h, COLOR_ACCENT, COLOR_TASKBAR_HI, COLOR_BORDER);

    fill_rect(min_x + 2, y + btn_h - 3, btn_w - 4, 1, COLOR_BORDER);

    if (state.windows[app].maximized) {
        stroke_rect(max_x + 2, y + 3, btn_w - 4, btn_h - 4, COLOR_BORDER);
        stroke_rect(max_x + 1, y + 2, btn_w - 4, btn_h - 4, COLOR_BORDER);
    } else {
        stroke_rect(max_x + 2, y + 2, btn_w - 4, btn_h - 4, COLOR_BORDER);
    }

    for (int32_t i = 0; i < 5; ++i) {
        put_px(close_x + 2 + i, y + 2 + i, COLOR_WINDOW_BG);
        put_px(close_x + 2 + i, y + 6 - i, COLOR_WINDOW_BG);
    }
}

static void draw_window(uint32_t app, bool active) {
    int32_t text_scale = ui_text_scale();
    const char* title = app_window_title(app);
    struct app_window* w = &state.windows[app];

    fill_rect(w->x + 3, w->y + 3, w->w, w->h, COLOR_SHADOW);
    fill_rect(w->x + 1, w->y + 1, w->w, w->h, COLOR_SHADOW);
    draw_glass_rect(w->x, w->y, w->w, w->h, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);

    draw_glass_rect(w->x + 1, w->y + 1, w->w - 2, TITLE_H - 1, active ? COLOR_TITLE_ACTIVE : COLOR_TITLE_INACTIVE, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_text5x7(w->x + 5, w->y + 3, title, COLOR_WINDOW_BG, text_scale);
    draw_window_title_controls(app, w, active);

    if (app == APP_FILE_MANAGER) {
        draw_window_content_file(w);
    } else if (app == APP_TERMINAL) {
        draw_window_content_terminal(w);
    } else if (app == APP_SETTINGS) {
        draw_window_content_settings(w);
    } else if (app == APP_NOTEPAD) {
        draw_window_content_notepad(w);
    } else if (app == APP_CALCULATOR) {
        draw_window_content_calculator(w);
    } else if (app == APP_IMAGE_VIEWER) {
        draw_window_content_image_viewer(w);
    } else if (app == APP_VIDEO_PLAYER) {
        draw_window_content_video_player(w);
    } else {
        draw_window_content_browser(w);
    }
}

static void draw_windows(void) {
    int32_t active = state.active_app;
    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        win_btn_close_w[i] = 0;
        win_btn_max_w[i] = 0;
        win_btn_min_w[i] = 0;
    }

    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        if (!state.windows[i].open || state.windows[i].minimized) {
            continue;
        }
        if ((int32_t)i == active) {
            continue;
        }
        draw_window(i, false);
    }

    if (active >= 0 && active < (int32_t)APP_COUNT &&
        state.windows[(uint32_t)active].open &&
        !state.windows[(uint32_t)active].minimized) {
        draw_window((uint32_t)active, true);
    }
}

static const char* desktop_menu_label(uint8_t idx) {
    if (idx == 0u) {
        return "REFRESH";
    }
    if (idx == 1u) {
        return "NEW FOLDER";
    }
    return "NEW FILE";
}

static void desktop_menu_open_at(int32_t x, int32_t y) {
    desktop_menu_h = desktop_menu_item_h * (int32_t)DESKTOP_MENU_ITEM_COUNT;
    desktop_menu_x = clamp_i32(x, 2, (int32_t)fb_width - desktop_menu_w - 2);
    desktop_menu_y = clamp_i32(y, shell_top_h() + 2, (int32_t)fb_height - shell_bottom_h() - desktop_menu_h - 2);
    desktop_menu_open = true;
}

static int32_t desktop_menu_hit_index(int32_t x, int32_t y) {
    if (!desktop_menu_open) {
        return -1;
    }
    if (!point_in_rect(x, y, desktop_menu_x, desktop_menu_y, desktop_menu_w, desktop_menu_h)) {
        return -1;
    }
    int32_t idx = (y - desktop_menu_y) / desktop_menu_item_h;
    if (idx < 0 || idx >= (int32_t)DESKTOP_MENU_ITEM_COUNT) {
        return -1;
    }
    return idx;
}

static void desktop_menu_create_entry(bool is_dir) {
    char name[VFS_NAME_MAX];
    uint8_t node = VFS_INVALID_NODE;

    for (uint32_t try_i = 0u; try_i < 64u; ++try_i) {
        build_desktop_generated_name(is_dir, name);
        if (vfs_find_child(desktop_root_node, name) == VFS_INVALID_NODE) {
            node = vfs_add_node(desktop_root_node, is_dir ? VFS_TYPE_DIR : VFS_TYPE_FILE, name, is_dir ? 0 : "NEW FILE");
            break;
        }
    }

    if (node == VFS_INVALID_NODE) {
        terminal_push_err(wstatus_str(W_ERR_NO_SPACE));
        return;
    }

    terminal_push_ok(is_dir ? "DESKTOP FOLDER CREATED" : "DESKTOP FILE CREATED");
    sync_desktop_items_from_vfs();
    build_desktop_template();
    mark_pstore_dirty();
}

static void desktop_menu_execute(uint8_t idx) {
    if (idx == 0u) {
        build_desktop_template();
        terminal_push_ok("DESKTOP REFRESHED");
        return;
    }
    if (idx == 1u) {
        desktop_menu_create_entry(true);
        return;
    }
    desktop_menu_create_entry(false);
}

static void draw_desktop_menu(void) {
    if (!desktop_menu_open) {
        return;
    }

    fill_rect(desktop_menu_x, desktop_menu_y, desktop_menu_w, desktop_menu_h, COLOR_WINDOW_BG);
    stroke_rect(desktop_menu_x, desktop_menu_y, desktop_menu_w, desktop_menu_h, COLOR_BORDER);

    int32_t text_scale = ui_text_scale();
    for (uint8_t i = 0u; i < DESKTOP_MENU_ITEM_COUNT; ++i) {
        int32_t iy = desktop_menu_y + (int32_t)i * desktop_menu_item_h;
        bool hover = point_in_rect(state.cursor_x, state.cursor_y, desktop_menu_x, iy, desktop_menu_w, desktop_menu_item_h);
        fill_rect(desktop_menu_x + 1, iy + 1, desktop_menu_w - 2, desktop_menu_item_h - 2, hover ? COLOR_PANEL_BG : COLOR_WINDOW_BG);
        draw_text5x7(desktop_menu_x + 7, iy + 4, desktop_menu_label(i), COLOR_BORDER, text_scale);
    }
}

static void draw_shell_launcher(void) {
    if (!shell_launcher_open) {
        return;
    }

    int32_t scale = desktop_scale();
    int32_t text_scale = ui_text_scale();
    int32_t row_h = clamp_i32(16 + scale * 2, 16, 24);
    int32_t header_h = 16;
    int32_t panel_w = clamp_i32((int32_t)fb_width / 3, 220, 380);
    int32_t panel_h = header_h + 8 + row_h * (int32_t)APP_COUNT + 6;
    int32_t x = shell_search_x;
    int32_t y = shell_search_y + shell_search_h + 4;
    int32_t min_y = shell_top_h() + 2;
    int32_t max_y = (int32_t)fb_height - shell_bottom_h() - panel_h - 2;
    if (max_y < min_y) {
        max_y = min_y;
    }

    x = clamp_i32(x, 4, (int32_t)fb_width - panel_w - 4);
    y = clamp_i32(y, min_y, max_y);

    shell_launcher_x = x;
    shell_launcher_y = y;
    shell_launcher_w = panel_w;
    shell_launcher_h = panel_h;

    if (ui_blur_runtime > 0u) {
        blur_region(x, y, panel_w, panel_h, ui_blur_runtime);
    }

    draw_glass_rect(x, y, panel_w, panel_h, COLOR_WINDOW_BG, COLOR_TASKBAR_HI, COLOR_BORDER);
    draw_text5x7(x + 8, y + 4, "WEVOA LAUNCHER", COLOR_BORDER, text_scale);

    int32_t row_x = x + 4;
    int32_t row_w = panel_w - 8;
    int32_t row_y = y + header_h + 2;
    for (uint32_t i = 0u; i < APP_COUNT; ++i) {
        bool hover = point_in_rect(state.cursor_x, state.cursor_y, row_x, row_y, row_w, row_h);
        bool active = state.windows[i].open && !state.windows[i].minimized && state.active_app == (int32_t)i;
        uint8_t bg = active ? COLOR_ACCENT : (hover ? COLOR_TASKBAR_HI : COLOR_PANEL_BG);
        uint8_t fg = active ? COLOR_WINDOW_BG : COLOR_BORDER;

        shell_launcher_item_x[i] = row_x;
        shell_launcher_item_y[i] = row_y;
        shell_launcher_item_w[i] = row_w;
        shell_launcher_item_h[i] = row_h;

        draw_glass_rect(row_x, row_y, row_w, row_h, bg, COLOR_TASKBAR_HI, COLOR_BORDER);
        draw_icon_glyph((uint8_t)i, row_x + 4, row_y + 2, row_h - 4);
        draw_text5x7(row_x + row_h + 2, row_y + 4, app_launcher_label(i), fg, text_scale);

        row_y += row_h;
    }
}

static void draw_lock_screen(void) {
    int32_t text_scale = ui_text_scale();
    int32_t panel_w = clamp_i32((int32_t)(fb_width * 46u / 100u), 280, 540);
    int32_t panel_h = clamp_i32((int32_t)(fb_height * 42u / 100u), 170, 320);
    int32_t x = ((int32_t)fb_width - panel_w) / 2;
    int32_t y = ((int32_t)fb_height - panel_h) / 2;

    fill_rect(0, 0, (int32_t)fb_width, (int32_t)fb_height, COLOR_SHADOW);
    fill_rect(x, y, panel_w, panel_h, COLOR_WINDOW_BG);
    stroke_rect(x, y, panel_w, panel_h, COLOR_BORDER);
    fill_rect(x + 1, y + 1, panel_w - 2, TITLE_H - 1, COLOR_TITLE_ACTIVE);
    draw_text5x7(x + 6, y + 3, "WEVOA LOCK SCREEN", COLOR_WINDOW_BG, text_scale);

    draw_text5x7(x + 10, y + 26, "SYSTEM LOCKED", COLOR_BORDER, text_scale);
    draw_text5x7(x + 10, y + 38, "ENTER PIN AND PRESS ENTER", COLOR_BORDER, text_scale);

    int32_t input_x = x + 10;
    int32_t input_y = y + 54;
    int32_t input_w = panel_w - 20;
    fill_rect(input_x, input_y, input_w, 20, COLOR_BLACK);
    stroke_rect(input_x, input_y, input_w, 20, COLOR_BORDER);

    char masked[16];
    uint8_t idx = 0u;
    for (; idx < lock_screen_pin_len && idx + 2u < sizeof(masked); ++idx) {
        masked[idx] = '*';
    }
    masked[idx++] = '_';
    masked[idx] = '\0';
    draw_text5x7(input_x + 8, input_y + 7, masked, COLOR_WINDOW_BG, text_scale);

    if (lock_screen_bad_pin) {
        draw_text5x7(x + 10, y + 80, "WRONG PIN", COLOR_ACCENT, text_scale);
    }

    draw_text5x7(x + 10, y + panel_h - 28, "TIP: DEFAULT PIN IS 1234", COLOR_BORDER, text_scale);
    draw_text5x7(x + 10, y + panel_h - 16, "CHANGE IT IN SETTINGS", COLOR_BORDER, text_scale);
}

static void draw_cursor(void) {
    int32_t x = state.cursor_x;
    int32_t y = state.cursor_y;
    int32_t u = 2;
    if (fb_width < 900u && fb_height < 700u) {
        u = 1;
    } else if (fb_width >= 2200u || fb_height >= 1400u) {
        u = 3;
    }

    int32_t tri_h = 9 * u;
    int32_t tri_w_max = 6 * u;
    int32_t stem_y0 = 7 * u;
    int32_t stem_y1 = 14 * u;
    int32_t stem_x0 = 2 * u;
    int32_t stem_x1 = 4 * u;
    int32_t tail_y0 = 12 * u;
    int32_t tail_y1 = 18 * u;
    int32_t tail_x0 = 4 * u;
    int32_t tail_x1 = 6 * u;

    /* soft shadow */
    for (int32_t row = 0; row < tri_h; ++row) {
        int32_t span = 1 + row / 2;
        if (span > tri_w_max) {
            span = tri_w_max;
        }
        for (int32_t col = 0; col < span; ++col) {
            put_px(x + col + u, y + row + u, COLOR_SHADOW);
        }
    }
    fill_rect(x + stem_x0 + u, y + stem_y0 + u, stem_x1 - stem_x0 + 1, stem_y1 - stem_y0, COLOR_SHADOW);
    fill_rect(x + tail_x0 + u, y + tail_y0 + u, tail_x1 - tail_x0 + 1, tail_y1 - tail_y0, COLOR_SHADOW);

    for (int32_t row = 0; row < tri_h; ++row) {
        int32_t span = 1 + row / 2;
        if (span > tri_w_max) {
            span = tri_w_max;
        }
        for (int32_t col = 0; col < span; ++col) {
            bool edge = (col == 0) || (col == span - 1) || (row == tri_h - 1);
            put_px(x + col, y + row, edge ? COLOR_CURSOR_OUT : COLOR_CURSOR_IN);
        }
    }

    for (int32_t row = stem_y0; row < stem_y1; ++row) {
        for (int32_t col = stem_x0; col <= stem_x1; ++col) {
            bool edge = (col == stem_x0) || (col == stem_x1) || (row == stem_y1 - 1);
            put_px(x + col, y + row, edge ? COLOR_CURSOR_OUT : COLOR_CURSOR_IN);
        }
    }

    for (int32_t row = tail_y0; row < tail_y1; ++row) {
        for (int32_t col = tail_x0; col <= tail_x1; ++col) {
            bool edge = (col == tail_x0) || (col == tail_x1) || (row == tail_y1 - 1);
            put_px(x + col, y + row, edge ? COLOR_CURSOR_OUT : COLOR_CURSOR_IN);
        }
    }
}

static void clear_pressed(void) {
    for (uint32_t i = 0; i < KEY_MAX; ++i) {
        state.key_pressed[i] = false;
    }
    state.mouse_left_pressed = false;
    state.mouse_right_pressed = false;
}

static void reset_layout(void) {
    int32_t top_h = shell_top_h();
    int32_t bottom_h = shell_bottom_h();
    int32_t file_w = (int32_t)(fb_width * 74u / 100u);
    int32_t file_h = (int32_t)(fb_height * 80u / 100u);
    file_w = clamp_i32(file_w, 260, 920);
    file_h = clamp_i32(file_h, 180, 680);
    int32_t workspace_y = top_h + 2;
    int32_t workspace_h = (int32_t)fb_height - workspace_y - bottom_h - 2;
    int32_t max_file_y = workspace_y + workspace_h - file_h;
    if (max_file_y < workspace_y) {
        max_file_y = workspace_y;
    }
    int32_t file_x = clamp_i32((int32_t)((fb_width - (uint32_t)file_w) / 2u), 50, (int32_t)fb_width - file_w - 4);
    int32_t file_y = clamp_i32(workspace_y + (workspace_h - file_h) / 2, workspace_y, max_file_y);

    state.windows[APP_FILE_MANAGER].x = file_x;
    state.windows[APP_FILE_MANAGER].y = file_y;
    state.windows[APP_FILE_MANAGER].w = file_w;
    state.windows[APP_FILE_MANAGER].h = file_h;
    state.windows[APP_FILE_MANAGER].open = false;
    state.windows[APP_FILE_MANAGER].minimized = false;
    state.windows[APP_FILE_MANAGER].maximized = false;
    state.windows[APP_FILE_MANAGER].restore_x = file_x;
    state.windows[APP_FILE_MANAGER].restore_y = file_y;
    state.windows[APP_FILE_MANAGER].restore_w = file_w;
    state.windows[APP_FILE_MANAGER].restore_h = file_h;

    int32_t term_w = clamp_i32(file_w * 76 / 100, 240, 760);
    int32_t term_h = clamp_i32(file_h * 58 / 100, 140, 500);
    state.windows[APP_TERMINAL].x = file_x + 34;
    state.windows[APP_TERMINAL].y = file_y + 28;
    state.windows[APP_TERMINAL].w = term_w;
    state.windows[APP_TERMINAL].h = term_h;
    state.windows[APP_TERMINAL].open = false;
    state.windows[APP_TERMINAL].minimized = false;
    state.windows[APP_TERMINAL].maximized = false;
    state.windows[APP_TERMINAL].restore_x = state.windows[APP_TERMINAL].x;
    state.windows[APP_TERMINAL].restore_y = state.windows[APP_TERMINAL].y;
    state.windows[APP_TERMINAL].restore_w = term_w;
    state.windows[APP_TERMINAL].restore_h = term_h;

    int32_t set_w = clamp_i32(file_w * 62 / 100, 220, 620);
    int32_t set_h = clamp_i32(file_h * 66 / 100, 240, 500);
    state.windows[APP_SETTINGS].x = file_x + 18;
    state.windows[APP_SETTINGS].y = file_y + 16;
    state.windows[APP_SETTINGS].w = set_w;
    state.windows[APP_SETTINGS].h = set_h;
    state.windows[APP_SETTINGS].open = false;
    state.windows[APP_SETTINGS].minimized = false;
    state.windows[APP_SETTINGS].maximized = false;
    state.windows[APP_SETTINGS].restore_x = state.windows[APP_SETTINGS].x;
    state.windows[APP_SETTINGS].restore_y = state.windows[APP_SETTINGS].y;
    state.windows[APP_SETTINGS].restore_w = set_w;
    state.windows[APP_SETTINGS].restore_h = set_h;

    int32_t note_w = clamp_i32(file_w * 60 / 100, 240, 680);
    int32_t note_h = clamp_i32(file_h * 52 / 100, 150, 420);
    state.windows[APP_NOTEPAD].x = file_x + 24;
    state.windows[APP_NOTEPAD].y = file_y + 20;
    state.windows[APP_NOTEPAD].w = note_w;
    state.windows[APP_NOTEPAD].h = note_h;
    state.windows[APP_NOTEPAD].open = false;
    state.windows[APP_NOTEPAD].minimized = false;
    state.windows[APP_NOTEPAD].maximized = false;
    state.windows[APP_NOTEPAD].restore_x = state.windows[APP_NOTEPAD].x;
    state.windows[APP_NOTEPAD].restore_y = state.windows[APP_NOTEPAD].y;
    state.windows[APP_NOTEPAD].restore_w = note_w;
    state.windows[APP_NOTEPAD].restore_h = note_h;

    int32_t calc_w = clamp_i32(file_w * 34 / 100, 150, 300);
    int32_t calc_h = clamp_i32(file_h * 48 / 100, 130, 330);
    state.windows[APP_CALCULATOR].x = file_x + file_w - calc_w - 16;
    state.windows[APP_CALCULATOR].y = file_y + 26;
    state.windows[APP_CALCULATOR].w = calc_w;
    state.windows[APP_CALCULATOR].h = calc_h;
    state.windows[APP_CALCULATOR].open = false;
    state.windows[APP_CALCULATOR].minimized = false;
    state.windows[APP_CALCULATOR].maximized = false;
    state.windows[APP_CALCULATOR].restore_x = state.windows[APP_CALCULATOR].x;
    state.windows[APP_CALCULATOR].restore_y = state.windows[APP_CALCULATOR].y;
    state.windows[APP_CALCULATOR].restore_w = calc_w;
    state.windows[APP_CALCULATOR].restore_h = calc_h;

    int32_t image_w = clamp_i32(file_w * 52 / 100, 220, 620);
    int32_t image_h = clamp_i32(file_h * 48 / 100, 160, 420);
    state.windows[APP_IMAGE_VIEWER].x = file_x + 14;
    state.windows[APP_IMAGE_VIEWER].y = file_y + 34;
    state.windows[APP_IMAGE_VIEWER].w = image_w;
    state.windows[APP_IMAGE_VIEWER].h = image_h;
    state.windows[APP_IMAGE_VIEWER].open = false;
    state.windows[APP_IMAGE_VIEWER].minimized = false;
    state.windows[APP_IMAGE_VIEWER].maximized = false;
    state.windows[APP_IMAGE_VIEWER].restore_x = state.windows[APP_IMAGE_VIEWER].x;
    state.windows[APP_IMAGE_VIEWER].restore_y = state.windows[APP_IMAGE_VIEWER].y;
    state.windows[APP_IMAGE_VIEWER].restore_w = image_w;
    state.windows[APP_IMAGE_VIEWER].restore_h = image_h;

    int32_t video_w = clamp_i32(file_w * 58 / 100, 250, 700);
    int32_t video_h = clamp_i32(file_h * 52 / 100, 170, 460);
    state.windows[APP_VIDEO_PLAYER].x = file_x + file_w - video_w - 12;
    state.windows[APP_VIDEO_PLAYER].y = file_y + 38;
    state.windows[APP_VIDEO_PLAYER].w = video_w;
    state.windows[APP_VIDEO_PLAYER].h = video_h;
    state.windows[APP_VIDEO_PLAYER].open = false;
    state.windows[APP_VIDEO_PLAYER].minimized = false;
    state.windows[APP_VIDEO_PLAYER].maximized = false;
    state.windows[APP_VIDEO_PLAYER].restore_x = state.windows[APP_VIDEO_PLAYER].x;
    state.windows[APP_VIDEO_PLAYER].restore_y = state.windows[APP_VIDEO_PLAYER].y;
    state.windows[APP_VIDEO_PLAYER].restore_w = video_w;
    state.windows[APP_VIDEO_PLAYER].restore_h = video_h;

    int32_t browser_w = clamp_i32(file_w * 64 / 100, 280, 820);
    int32_t browser_h = clamp_i32(file_h * 60 / 100, 190, 520);
    state.windows[APP_BROWSER].x = file_x + 26;
    state.windows[APP_BROWSER].y = file_y + 22;
    state.windows[APP_BROWSER].w = browser_w;
    state.windows[APP_BROWSER].h = browser_h;
    state.windows[APP_BROWSER].open = false;
    state.windows[APP_BROWSER].minimized = false;
    state.windows[APP_BROWSER].maximized = false;
    state.windows[APP_BROWSER].restore_x = state.windows[APP_BROWSER].x;
    state.windows[APP_BROWSER].restore_y = state.windows[APP_BROWSER].y;
    state.windows[APP_BROWSER].restore_w = browser_w;
    state.windows[APP_BROWSER].restore_h = browser_h;

    state.active_app = -1;
    state.dragging = false;
    state.drag_dx = 0;
    state.drag_dy = 0;
    state.mouse_left_down = false;
    state.mouse_left_pressed = false;
    state.mouse_right_down = false;
    state.mouse_right_pressed = false;
    state.mouse_index = 0;
    state.key_move_accum = 0;
    state.last_tsc = 0;

    state.cursor_x = (int32_t)(fb_width / 2u);
    state.cursor_y = (int32_t)(fb_height / 2u);
    state.cursor_fx = state.cursor_x << CURSOR_FP_SHIFT;
    state.cursor_fy = state.cursor_y << CURSOR_FP_SHIFT;
    shell_launcher_open = false;
    shell_launcher_x = shell_launcher_y = shell_launcher_w = shell_launcher_h = 0;
    shell_search_x = shell_search_y = shell_search_w = shell_search_h = 0;
    set_wifi_btn_x = set_wifi_btn_y = set_wifi_btn_w = set_wifi_btn_h = 0;
    set_link_btn_x = set_link_btn_y = set_link_btn_w = set_link_btn_h = 0;
    set_bminus_btn_x = set_bminus_btn_y = set_bminus_btn_w = set_bminus_btn_h = 0;
    set_bplus_btn_x = set_bplus_btn_y = set_bplus_btn_w = set_bplus_btn_h = 0;
    set_wp_prev_btn_x = set_wp_prev_btn_y = set_wp_prev_btn_w = set_wp_prev_btn_h = 0;
    set_wp_next_btn_x = set_wp_next_btn_y = set_wp_next_btn_w = set_wp_next_btn_h = 0;
    set_scale_minus_btn_x = set_scale_minus_btn_y = set_scale_minus_btn_w = set_scale_minus_btn_h = 0;
    set_scale_plus_btn_x = set_scale_plus_btn_y = set_scale_plus_btn_w = set_scale_plus_btn_h = 0;
    set_scale_auto_btn_x = set_scale_auto_btn_y = set_scale_auto_btn_w = set_scale_auto_btn_h = 0;
    note_btn_load_x = note_btn_load_y = note_btn_load_w = note_btn_load_h = 0;
    note_btn_save_x = note_btn_save_y = note_btn_save_w = note_btn_save_h = 0;
    note_btn_new_x = note_btn_new_y = note_btn_new_w = note_btn_new_h = 0;
    video_btn_play_x = video_btn_play_y = video_btn_play_w = video_btn_play_h = 0;
    video_btn_pause_x = video_btn_pause_y = video_btn_pause_w = video_btn_pause_h = 0;
    video_btn_stop_x = video_btn_stop_y = video_btn_stop_w = video_btn_stop_h = 0;
    browser_url_x = browser_url_y = browser_url_w = browser_url_h = 0;
    browser_go_btn_x = browser_go_btn_y = browser_go_btn_w = browser_go_btn_h = 0;
    browser_home_btn_x = browser_home_btn_y = browser_home_btn_w = browser_home_btn_h = 0;
    browser_refresh_btn_x = browser_refresh_btn_y = browser_refresh_btn_w = browser_refresh_btn_h = 0;
    set_lock_btn_x = set_lock_btn_y = set_lock_btn_w = set_lock_btn_h = 0;
    set_pin_minus_btn_x = set_pin_minus_btn_y = set_pin_minus_btn_w = set_pin_minus_btn_h = 0;
    set_pin_plus_btn_x = set_pin_plus_btn_y = set_pin_plus_btn_w = set_pin_plus_btn_h = 0;
    set_lock_now_btn_x = set_lock_now_btn_y = set_lock_now_btn_w = set_lock_now_btn_h = 0;
    set_net_up_btn_x = set_net_up_btn_y = set_net_up_btn_w = set_net_up_btn_h = 0;
    set_net_down_btn_x = set_net_down_btn_y = set_net_down_btn_w = set_net_down_btn_h = 0;
    set_dns_dev_btn_x = set_dns_dev_btn_y = set_dns_dev_btn_w = set_dns_dev_btn_h = 0;
    set_dns_openai_btn_x = set_dns_openai_btn_y = set_dns_openai_btn_w = set_dns_openai_btn_h = 0;
    set_fw80_btn_x = set_fw80_btn_y = set_fw80_btn_w = set_fw80_btn_h = 0;
    set_fw443_btn_x = set_fw443_btn_y = set_fw443_btn_w = set_fw443_btn_h = 0;
    set_shell_layout_btn_x = set_shell_layout_btn_y = set_shell_layout_btn_w = set_shell_layout_btn_h = 0;
    set_blur_minus_btn_x = set_blur_minus_btn_y = set_blur_minus_btn_w = set_blur_minus_btn_h = 0;
    set_blur_plus_btn_x = set_blur_plus_btn_y = set_blur_plus_btn_w = set_blur_plus_btn_h = 0;
    set_motion_minus_btn_x = set_motion_minus_btn_y = set_motion_minus_btn_w = set_motion_minus_btn_h = 0;
    set_motion_plus_btn_x = set_motion_plus_btn_y = set_motion_plus_btn_w = set_motion_plus_btn_h = 0;
    set_quality_btn_x = set_quality_btn_y = set_quality_btn_w = set_quality_btn_h = 0;
    for (uint32_t i = 0u; i < 16u; ++i) {
        calc_btn_x[i] = calc_btn_y[i] = calc_btn_w[i] = calc_btn_h[i] = 0;
    }
    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        shell_launcher_item_x[i] = shell_launcher_item_y[i] = 0;
        shell_launcher_item_w[i] = shell_launcher_item_h[i] = 0;
        win_btn_close_x[i] = win_btn_close_y[i] = win_btn_close_w[i] = win_btn_close_h[i] = 0;
        win_btn_max_x[i] = win_btn_max_y[i] = win_btn_max_w[i] = win_btn_max_h[i] = 0;
        win_btn_min_x[i] = win_btn_min_y[i] = win_btn_min_w[i] = win_btn_min_h[i] = 0;
    }
    config_reset_defaults();
    state_sync_from_config();
    desktop_menu_open = false;
    terminal_init();
    notepad_bind_default_file();
    image_state.sample_index = 0u;
    video_state.playing = false;
    video_state.frame_index = 0u;
    video_state.frame_accum_cycles = 0ull;
    browser_state.url_len = 0u;
    browser_state.url[0] = '\0';
    browser_state.input_focus = false;
    browser_nav_token = 0u;
    browser_navigate("WEVOA/HOME");
    lock_screen_active = false;
    lock_screen_bad_pin = false;
    lock_screen_pin_len = 0u;
    lock_screen_pin_input[0] = '\0';
    text_copy(settings_net_status_line, TERM_LINE_MAX, "NET READY");
    pstore_dirty = false;
    pstore_dirty_since_tsc = 0ull;
}

static void focus_app(uint32_t app) {
    if (app >= APP_COUNT) {
        return;
    }
    state.windows[app].open = true;
    state.windows[app].minimized = false;
    state.active_app = (int32_t)app;
    shell_launcher_open = false;
}

static void focus_first_visible(void) {
    for (int32_t i = (int32_t)APP_COUNT - 1; i >= 0; --i) {
        if (state.windows[(uint32_t)i].open && !state.windows[(uint32_t)i].minimized) {
            state.active_app = i;
            return;
        }
    }
    state.active_app = -1;
}

static void close_app(uint32_t app) {
    if (app >= APP_COUNT) {
        return;
    }

    struct app_window* w = &state.windows[app];
    if (!w->open) {
        return;
    }

    w->open = false;
    w->minimized = false;
    w->maximized = false;
    state.dragging = false;

    if (state.active_app == (int32_t)app) {
        focus_first_visible();
    }
}

static void minimize_app(uint32_t app) {
    if (app >= APP_COUNT) {
        return;
    }

    struct app_window* w = &state.windows[app];
    if (!w->open) {
        return;
    }

    w->minimized = true;
    if (state.active_app == (int32_t)app) {
        state.dragging = false;
        focus_first_visible();
    }
}

static void toggle_maximize_app(uint32_t app) {
    if (app >= APP_COUNT) {
        return;
    }

    struct app_window* w = &state.windows[app];
    if (!w->open) {
        return;
    }

    if (!w->maximized) {
        int32_t top_h = shell_top_h();
        int32_t bottom_h = shell_bottom_h();
        w->restore_x = w->x;
        w->restore_y = w->y;
        w->restore_w = w->w;
        w->restore_h = w->h;
        w->x = 2;
        w->y = top_h + 2;
        w->w = clamp_i32((int32_t)fb_width - 4, 120, (int32_t)fb_width);
        w->h = clamp_i32((int32_t)fb_height - top_h - bottom_h - 4, 80, (int32_t)fb_height);
        w->maximized = true;
        state.dragging = false;
        return;
    }

    w->x = w->restore_x;
    w->y = w->restore_y;
    w->w = w->restore_w;
    w->h = w->restore_h;
    w->maximized = false;
    state.dragging = false;
}

static void close_active_app(void) {
    if (state.active_app < 0 || state.active_app >= (int32_t)APP_COUNT) {
        return;
    }
    close_app((uint32_t)state.active_app);
}

static void cycle_focus(void) {
    int32_t start = state.active_app;

    for (uint32_t step = 1; step <= APP_COUNT; ++step) {
        int32_t raw = start + (int32_t)step;
        if (raw < 0) {
            raw += (int32_t)APP_COUNT;
        }
        int32_t idx = raw % (int32_t)APP_COUNT;

        if (state.windows[(uint32_t)idx].open && !state.windows[(uint32_t)idx].minimized) {
            state.active_app = idx;
            return;
        }
    }

    state.active_app = -1;
}

static void limit_window(struct app_window* w) {
    int32_t top_h = shell_top_h();
    int32_t bottom_h = shell_bottom_h();
    if (w->maximized) {
        w->x = 2;
        w->y = top_h + 2;
        w->w = clamp_i32((int32_t)fb_width - 4, 120, (int32_t)fb_width);
        w->h = clamp_i32((int32_t)fb_height - top_h - bottom_h - 4, 80, (int32_t)fb_height);
        return;
    }

    int32_t min_x = 36;
    int32_t max_x = (int32_t)fb_width - w->w - 4;
    int32_t min_y = top_h + 2;
    int32_t max_y = (int32_t)fb_height - bottom_h - w->h - 2;

    if (max_x < min_x) {
        max_x = min_x;
    }
    if (max_y < min_y) {
        max_y = min_y;
    }

    w->x = clamp_i32(w->x, min_x, max_x);
    w->y = clamp_i32(w->y, min_y, max_y);
}

static int32_t window_at_point(int32_t x, int32_t y) {
    if (state.active_app >= 0 && state.active_app < (int32_t)APP_COUNT) {
        struct app_window* aw = &state.windows[(uint32_t)state.active_app];
        if (aw->open && !aw->minimized && point_in_rect(x, y, aw->x, aw->y, aw->w, aw->h)) {
            return state.active_app;
        }
    }

    for (int32_t i = (int32_t)APP_COUNT - 1; i >= 0; --i) {
        struct app_window* w = &state.windows[(uint32_t)i];
        if (!w->open || w->minimized) {
            continue;
        }
        if (point_in_rect(x, y, w->x, w->y, w->w, w->h)) {
            return i;
        }
    }

    return -1;
}

static bool title_hit(int32_t app, int32_t x, int32_t y) {
    if (app < 0 || app >= (int32_t)APP_COUNT) {
        return false;
    }

    struct app_window* w = &state.windows[(uint32_t)app];
    if (!w->open || w->minimized) {
        return false;
    }

    if (!point_in_rect(x, y, w->x, w->y, w->w, TITLE_H)) {
        return false;
    }

    if (point_in_rect(x, y,
                      win_btn_min_x[(uint32_t)app], win_btn_min_y[(uint32_t)app],
                      win_btn_min_w[(uint32_t)app], win_btn_min_h[(uint32_t)app])) {
        return false;
    }
    if (point_in_rect(x, y,
                      win_btn_max_x[(uint32_t)app], win_btn_max_y[(uint32_t)app],
                      win_btn_max_w[(uint32_t)app], win_btn_max_h[(uint32_t)app])) {
        return false;
    }
    if (point_in_rect(x, y,
                      win_btn_close_x[(uint32_t)app], win_btn_close_y[(uint32_t)app],
                      win_btn_close_w[(uint32_t)app], win_btn_close_h[(uint32_t)app])) {
        return false;
    }

    return true;
}

static bool handle_settings_control_click(void) {
    if (state.active_app != (int32_t)APP_SETTINGS ||
        !state.windows[APP_SETTINGS].open ||
        state.windows[APP_SETTINGS].minimized) {
        return false;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_wifi_btn_x, set_wifi_btn_y, set_wifi_btn_w, set_wifi_btn_h)) {
        int rc = config_set(WEVOA_CFG_WIFI, state.settings_wifi_on ? 0u : 1u);
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
            text_copy(settings_net_status_line, TERM_LINE_MAX, state.settings_wifi_on ? "WIFI ENABLED" : "WIFI DISABLED");
        } else {
            text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_net_up_btn_x, set_net_up_btn_y, set_net_up_btn_w, set_net_up_btn_h)) {
        int rc = config_set(WEVOA_CFG_WIFI, 1u);
        if (rc == W_OK) {
            rc = config_set(WEVOA_CFG_NET_LINK, 1u);
        }
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
            text_copy(settings_net_status_line, TERM_LINE_MAX, "NETWORK LINK UP");
        } else {
            text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_net_down_btn_x, set_net_down_btn_y, set_net_down_btn_w, set_net_down_btn_h)) {
        int rc = config_set(WEVOA_CFG_NET_LINK, 0u);
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
            text_copy(settings_net_status_line, TERM_LINE_MAX, "NETWORK LINK DOWN");
        } else {
            text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_dns_dev_btn_x, set_dns_dev_btn_y, set_dns_dev_btn_w, set_dns_dev_btn_h)) {
        uint8_t ip[4];
        char ip_text[20];
        int rc = net_dns_resolve("DEVDOPZ.COM", ip);
        if (rc == W_OK) {
            text_copy(settings_net_status_line, TERM_LINE_MAX, "DNS DEVDOPZ ");
            ipv4_to_text(ip, ip_text);
            text_append(settings_net_status_line, TERM_LINE_MAX, ip_text);
        } else {
            text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_dns_openai_btn_x, set_dns_openai_btn_y, set_dns_openai_btn_w, set_dns_openai_btn_h)) {
        uint8_t ip[4];
        char ip_text[20];
        int rc = net_dns_resolve("OPENAI.COM", ip);
        if (rc == W_OK) {
            text_copy(settings_net_status_line, TERM_LINE_MAX, "DNS OPENAI ");
            ipv4_to_text(ip, ip_text);
            text_append(settings_net_status_line, TERM_LINE_MAX, ip_text);
        } else {
            text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_fw80_btn_x, set_fw80_btn_y, set_fw80_btn_w, set_fw80_btn_h)) {
        uint8_t allow = 1u;
        int rc = net_firewall_get_port(80u, &allow);
        if (rc == W_OK) {
            rc = net_firewall_set_port(80u, allow ? 0u : 1u);
        }
        if (rc == W_OK) {
            text_copy(settings_net_status_line, TERM_LINE_MAX, allow ? "PORT 80 BLOCKED" : "PORT 80 ALLOWED");
        } else {
            text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_fw443_btn_x, set_fw443_btn_y, set_fw443_btn_w, set_fw443_btn_h)) {
        uint8_t allow = 1u;
        int rc = net_firewall_get_port(443u, &allow);
        if (rc == W_OK) {
            rc = net_firewall_set_port(443u, allow ? 0u : 1u);
        }
        if (rc == W_OK) {
            text_copy(settings_net_status_line, TERM_LINE_MAX, allow ? "PORT 443 BLOCKED" : "PORT 443 ALLOWED");
        } else {
            text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_link_btn_x, set_link_btn_y, set_link_btn_w, set_link_btn_h)) {
        int rc = W_OK;
        if (state.settings_wifi_on) {
            rc = config_set(WEVOA_CFG_NET_LINK, state.settings_wifi_connected ? 0u : 1u);
        } else {
            rc = config_set(WEVOA_CFG_NET_LINK, 0u);
        }
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
            text_copy(settings_net_status_line, TERM_LINE_MAX, state.settings_wifi_connected ? "NETWORK LINK UP" : "NETWORK LINK DOWN");
        } else {
            text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_bminus_btn_x, set_bminus_btn_y, set_bminus_btn_w, set_bminus_btn_h)) {
        int32_t v = (int32_t)state.settings_brightness - 5;
        int rc = config_set(WEVOA_CFG_BRIGHT, (uint32_t)clamp_i32(v, 10, 100));
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_bplus_btn_x, set_bplus_btn_y, set_bplus_btn_w, set_bplus_btn_h)) {
        int32_t v = (int32_t)state.settings_brightness + 5;
        int rc = config_set(WEVOA_CFG_BRIGHT, (uint32_t)clamp_i32(v, 10, 100));
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_wp_prev_btn_x, set_wp_prev_btn_y, set_wp_prev_btn_w, set_wp_prev_btn_h)) {
        uint32_t next = (state.wallpaper_index == 0u) ? (WALLPAPER_COUNT - 1u) : (state.wallpaper_index - 1u);
        int rc = config_set(WEVOA_CFG_WALL, next);
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_wp_next_btn_x, set_wp_next_btn_y, set_wp_next_btn_w, set_wp_next_btn_h)) {
        int rc = config_set(WEVOA_CFG_WALL, state.wallpaper_index + 1u);
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_scale_minus_btn_x, set_scale_minus_btn_y, set_scale_minus_btn_w, set_scale_minus_btn_h)) {
        if (ui_scale_override == 0u) {
            ui_scale_override = (uint8_t)desktop_scale();
        }
        if (ui_scale_override > 1u) {
            ui_scale_override--;
        }
        mark_pstore_dirty();
        build_desktop_template();
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_scale_plus_btn_x, set_scale_plus_btn_y, set_scale_plus_btn_w, set_scale_plus_btn_h)) {
        if (ui_scale_override == 0u) {
            ui_scale_override = (uint8_t)desktop_scale();
        }
        if (ui_scale_override < 3u) {
            ui_scale_override++;
        }
        mark_pstore_dirty();
        build_desktop_template();
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_scale_auto_btn_x, set_scale_auto_btn_y, set_scale_auto_btn_w, set_scale_auto_btn_h)) {
        ui_scale_override = 0u;
        mark_pstore_dirty();
        build_desktop_template();
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_lock_btn_x, set_lock_btn_y, set_lock_btn_w, set_lock_btn_h)) {
        int rc = config_set(WEVOA_CFG_LOCK_ENABLED, state.settings_lock_enabled ? 0u : 1u);
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_pin_minus_btn_x, set_pin_minus_btn_y, set_pin_minus_btn_w, set_pin_minus_btn_h)) {
        uint32_t pin = (state.settings_lock_pin == 0u) ? 9999u : (uint32_t)(state.settings_lock_pin - 1u);
        int rc = config_set(WEVOA_CFG_LOCK_PIN, pin);
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_pin_plus_btn_x, set_pin_plus_btn_y, set_pin_plus_btn_w, set_pin_plus_btn_h)) {
        uint32_t pin = (state.settings_lock_pin >= 9999u) ? 0u : (uint32_t)(state.settings_lock_pin + 1u);
        int rc = config_set(WEVOA_CFG_LOCK_PIN, pin);
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_lock_now_btn_x, set_lock_now_btn_y, set_lock_now_btn_w, set_lock_now_btn_h)) {
        lock_screen_activate();
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_shell_layout_btn_x, set_shell_layout_btn_y, set_shell_layout_btn_w, set_shell_layout_btn_h)) {
        uint32_t next = (state.shell_layout == SHELL_LAYOUT_DOCK) ? SHELL_LAYOUT_TASKBAR : SHELL_LAYOUT_DOCK;
        int rc = config_set(WEVOA_CFG_SHELL_LAYOUT, next);
        state_sync_from_config();
        if (rc == W_OK) {
            build_desktop_template();
            for (uint32_t i = 0u; i < APP_COUNT; ++i) {
                limit_window(&state.windows[i]);
            }
            mark_pstore_dirty();
        }
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, set_blur_minus_btn_x, set_blur_minus_btn_y, set_blur_minus_btn_w, set_blur_minus_btn_h)) {
        int32_t v = (int32_t)state.ui_blur_level - 1;
        int rc = config_set(WEVOA_CFG_UI_BLUR_LEVEL, (uint32_t)clamp_i32(v, 0, UI_LEVEL_MAX));
        state_sync_from_config();
        if (rc == W_OK) {
            build_desktop_template();
            mark_pstore_dirty();
        }
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, set_blur_plus_btn_x, set_blur_plus_btn_y, set_blur_plus_btn_w, set_blur_plus_btn_h)) {
        int32_t v = (int32_t)state.ui_blur_level + 1;
        int rc = config_set(WEVOA_CFG_UI_BLUR_LEVEL, (uint32_t)clamp_i32(v, 0, UI_LEVEL_MAX));
        state_sync_from_config();
        if (rc == W_OK) {
            build_desktop_template();
            mark_pstore_dirty();
        }
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, set_motion_minus_btn_x, set_motion_minus_btn_y, set_motion_minus_btn_w, set_motion_minus_btn_h)) {
        int32_t v = (int32_t)state.ui_motion_level - 1;
        int rc = config_set(WEVOA_CFG_UI_MOTION_LEVEL, (uint32_t)clamp_i32(v, 0, UI_LEVEL_MAX));
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, set_motion_plus_btn_x, set_motion_plus_btn_y, set_motion_plus_btn_w, set_motion_plus_btn_h)) {
        int32_t v = (int32_t)state.ui_motion_level + 1;
        int rc = config_set(WEVOA_CFG_UI_MOTION_LEVEL, (uint32_t)clamp_i32(v, 0, UI_LEVEL_MAX));
        state_sync_from_config();
        if (rc == W_OK) {
            mark_pstore_dirty();
        }
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, set_quality_btn_x, set_quality_btn_y, set_quality_btn_w, set_quality_btn_h)) {
        uint32_t next = (uint32_t)((state.ui_effects_quality + 1u) % (UI_LEVEL_MAX + 1u));
        int rc = config_set(WEVOA_CFG_UI_EFFECTS_QUALITY, next);
        state_sync_from_config();
        if (rc == W_OK) {
            build_desktop_template();
            mark_pstore_dirty();
        }
        return true;
    }

    return false;
}

static bool handle_notepad_click(void) {
    if (state.active_app != (int32_t)APP_NOTEPAD ||
        !state.windows[APP_NOTEPAD].open ||
        state.windows[APP_NOTEPAD].minimized) {
        return false;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, note_btn_load_x, note_btn_load_y, note_btn_load_w, note_btn_load_h)) {
        notepad_load_from_node(notepad_state.file_node);
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, note_btn_save_x, note_btn_save_y, note_btn_save_w, note_btn_save_h)) {
        notepad_save_to_file();
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, note_btn_new_x, note_btn_new_y, note_btn_new_w, note_btn_new_h)) {
        char name[VFS_NAME_MAX];
        uint8_t created = VFS_INVALID_NODE;
        for (uint32_t t = 0u; t < 64u; ++t) {
            char id[4];
            u8_to_dec_text(desktop_new_file_id, id);
            text_copy(name, VFS_NAME_MAX, "NOTE");
            text_append(name, VFS_NAME_MAX, id);
            text_append(name, VFS_NAME_MAX, ".TXT");
            desktop_new_file_id++;
            if (vfs_find_child(desktop_root_node, name) == VFS_INVALID_NODE) {
                created = vfs_add_node(desktop_root_node, VFS_TYPE_FILE, name, "");
                break;
            }
        }
        if (created != VFS_INVALID_NODE) {
            notepad_load_from_node(created);
            sync_desktop_items_from_vfs();
            build_desktop_template();
            mark_pstore_dirty();
        }
        return true;
    }
    return false;
}

static bool handle_calculator_click(void) {
    if (state.active_app != (int32_t)APP_CALCULATOR ||
        !state.windows[APP_CALCULATOR].open ||
        state.windows[APP_CALCULATOR].minimized) {
        return false;
    }

    static const char tokens[16] = {
        '7', '8', '9', '/',
        '4', '5', '6', '*',
        '1', '2', '3', '-',
        'C', '0', '=', '+'
    };

    for (uint32_t i = 0u; i < 16u; ++i) {
        if (!point_in_rect(state.cursor_x, state.cursor_y, calc_btn_x[i], calc_btn_y[i], calc_btn_w[i], calc_btn_h[i])) {
            continue;
        }
        char tk = tokens[i];
        if (tk == 'C') {
            calc_state.expr_len = 0u;
            calc_state.expr[0] = '\0';
            return true;
        }
        if (tk == '=') {
            bool ok = false;
            int32_t v = calc_eval_expr(calc_state.expr, &ok);
            if (ok) {
                calc_set_display_value(v);
            } else {
                text_copy(calc_state.expr, sizeof(calc_state.expr), "ERR");
                calc_state.expr_len = 3u;
            }
            return true;
        }
        if (calc_state.expr_len + 1u < sizeof(calc_state.expr)) {
            calc_state.expr[calc_state.expr_len++] = tk;
            calc_state.expr[calc_state.expr_len] = '\0';
            return true;
        }
        return true;
    }
    return false;
}

static bool handle_video_click(void) {
    if (state.active_app != (int32_t)APP_VIDEO_PLAYER ||
        !state.windows[APP_VIDEO_PLAYER].open ||
        state.windows[APP_VIDEO_PLAYER].minimized) {
        return false;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, video_btn_play_x, video_btn_play_y, video_btn_play_w, video_btn_play_h)) {
        video_state.playing = true;
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, video_btn_pause_x, video_btn_pause_y, video_btn_pause_w, video_btn_pause_h)) {
        video_state.playing = false;
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, video_btn_stop_x, video_btn_stop_y, video_btn_stop_w, video_btn_stop_h)) {
        video_state.playing = false;
        video_state.frame_index = 0u;
        video_state.frame_accum_cycles = 0ull;
        return true;
    }
    return false;
}

static bool handle_browser_click(void) {
    if (state.active_app != (int32_t)APP_BROWSER ||
        !state.windows[APP_BROWSER].open ||
        state.windows[APP_BROWSER].minimized) {
        return false;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, browser_home_btn_x, browser_home_btn_y, browser_home_btn_w, browser_home_btn_h)) {
        browser_navigate("WEVOA/HOME");
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, browser_refresh_btn_x, browser_refresh_btn_y, browser_refresh_btn_w, browser_refresh_btn_h)) {
        browser_navigate(browser_state.url);
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, browser_go_btn_x, browser_go_btn_y, browser_go_btn_w, browser_go_btn_h)) {
        browser_navigate(browser_state.url);
        return true;
    }
    if (point_in_rect(state.cursor_x, state.cursor_y, browser_url_x, browser_url_y, browser_url_w, browser_url_h)) {
        browser_state.input_focus = true;
        return true;
    }
    browser_state.input_focus = false;
    return false;
}

static void lock_screen_clear_input(void) {
    lock_screen_pin_len = 0u;
    lock_screen_pin_input[0] = '\0';
    lock_screen_bad_pin = false;
}

static void lock_screen_activate(void) {
    lock_screen_active = true;
    lock_screen_clear_input();
    state.active_app = -1;
    state.dragging = false;
    desktop_menu_open = false;
}

static bool lock_screen_handle_input(void) {
    bool changed = false;
    bool shift = state.key_down[SC_LSHIFT] || state.key_down[SC_RSHIFT];

    if (state.key_pressed[SC_BACKSPACE]) {
        if (lock_screen_pin_len > 0u) {
            lock_screen_pin_len--;
            lock_screen_pin_input[lock_screen_pin_len] = '\0';
            changed = true;
        }
    }

    for (uint32_t sc = 0u; sc < KEY_MAX; ++sc) {
        if (!state.key_pressed[sc]) {
            continue;
        }
        if (sc == SC_ENTER || sc == SC_BACKSPACE || sc == SC_TAB || sc == SC_ESC) {
            continue;
        }
        char ch = scancode_to_char((uint8_t)sc, shift);
        if (ch < '0' || ch > '9') {
            continue;
        }
        if (lock_screen_pin_len + 1u < sizeof(lock_screen_pin_input)) {
            lock_screen_pin_input[lock_screen_pin_len++] = ch;
            lock_screen_pin_input[lock_screen_pin_len] = '\0';
            changed = true;
        }
    }

    if (state.key_pressed[SC_ENTER]) {
        uint32_t pin = 0u;
        uint32_t expected = 0u;
        if (!parse_u32(lock_screen_pin_input, &pin)) {
            pin = 0u;
        }
        if (config_get(WEVOA_CFG_LOCK_PIN, &expected) != W_OK) {
            expected = 1234u;
        }
        if (pin == expected) {
            lock_screen_active = false;
            lock_screen_clear_input();
            terminal_push_ok("UNLOCKED");
            changed = true;
        } else {
            lock_screen_bad_pin = true;
            lock_screen_pin_len = 0u;
            lock_screen_pin_input[0] = '\0';
            changed = true;
        }
    }

    if (state.key_pressed[SC_ESC]) {
        lock_screen_clear_input();
        changed = true;
    }

    return changed;
}

static bool handle_window_control_click_for_app(uint32_t app) {
    if (app >= APP_COUNT) {
        return false;
    }

    struct app_window* w = &state.windows[app];
    if (!w->open || w->minimized) {
        return false;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y,
                      win_btn_close_x[app], win_btn_close_y[app],
                      win_btn_close_w[app], win_btn_close_h[app])) {
        close_app(app);
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y,
                      win_btn_max_x[app], win_btn_max_y[app],
                      win_btn_max_w[app], win_btn_max_h[app])) {
        state.active_app = (int32_t)app;
        toggle_maximize_app(app);
        return true;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y,
                      win_btn_min_x[app], win_btn_min_y[app],
                      win_btn_min_w[app], win_btn_min_h[app])) {
        minimize_app(app);
        return true;
    }

    return false;
}

static bool handle_window_control_click(void) {
    if (state.active_app >= 0 && state.active_app < (int32_t)APP_COUNT) {
        if (handle_window_control_click_for_app((uint32_t)state.active_app)) {
            return true;
        }
    }

    for (int32_t i = (int32_t)APP_COUNT - 1; i >= 0; --i) {
        if (i == state.active_app) {
            continue;
        }
        if (handle_window_control_click_for_app((uint32_t)i)) {
            return true;
        }
    }

    return false;
}

static int32_t desktop_item_at_point(int32_t x, int32_t y) {
    int32_t scale = desktop_scale();
    int32_t icon_box_w = desktop_icon_box_w();
    int32_t icon_box_h = desktop_icon_box_h();
    for (uint32_t i = 0u; i < desktop_item_count && i < DESKTOP_ITEM_MAX; ++i) {
        if (!desktop_items[i].used) {
            continue;
        }
        if (point_in_rect(x, y,
                          desktop_item_x[i] - 2 * scale,
                          desktop_item_y[i] - 2 * scale,
                          icon_box_w,
                          icon_box_h)) {
            return (int32_t)i;
        }
    }
    return -1;
}

static bool is_desktop_free_space(int32_t x, int32_t y) {
    int32_t scale = desktop_scale();
    int32_t icon_box_w = desktop_icon_box_w();
    int32_t icon_box_h = desktop_icon_box_h();

    if (y < shell_top_h()) {
        return false;
    }
    if (y >= (int32_t)fb_height - shell_bottom_h()) {
        return false;
    }
    if (window_at_point(x, y) >= 0) {
        return false;
    }

    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        if (point_in_rect(x, y,
                          desktop_icon_x[i] - 2 * scale,
                          desktop_icon_y[i] - 2 * scale,
                          icon_box_w,
                          icon_box_h)) {
            return false;
        }
    }
    if (desktop_item_at_point(x, y) >= 0) {
        return false;
    }
    return true;
}

static bool handle_desktop_menu_left_click(void) {
    if (!desktop_menu_open) {
        return false;
    }

    int32_t idx = desktop_menu_hit_index(state.cursor_x, state.cursor_y);
    if (idx >= 0) {
        desktop_menu_execute((uint8_t)idx);
        desktop_menu_open = false;
        return true;
    }

    desktop_menu_open = false;
    return false;
}

static bool handle_shell_overlay_click(void) {
    if (!shell_launcher_open) {
        return false;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, shell_search_x, shell_search_y, shell_search_w, shell_search_h)) {
        return false;
    }

    if (!point_in_rect(state.cursor_x, state.cursor_y, shell_launcher_x, shell_launcher_y, shell_launcher_w, shell_launcher_h)) {
        shell_launcher_open = false;
        return true;
    }

    for (uint32_t i = 0u; i < APP_COUNT; ++i) {
        if (point_in_rect(state.cursor_x, state.cursor_y,
                          shell_launcher_item_x[i], shell_launcher_item_y[i],
                          shell_launcher_item_w[i], shell_launcher_item_h[i])) {
            focus_app(i);
            shell_launcher_open = false;
            return true;
        }
    }

    return true;
}

static void handle_right_click(void) {
    if (shell_launcher_open) {
        shell_launcher_open = false;
        return;
    }

    if (desktop_menu_open) {
        int32_t hit = desktop_menu_hit_index(state.cursor_x, state.cursor_y);
        if (hit >= 0) {
            desktop_menu_execute((uint8_t)hit);
        }
        desktop_menu_open = false;
        return;
    }

    if (is_desktop_free_space(state.cursor_x, state.cursor_y)) {
        desktop_menu_open_at(state.cursor_x, state.cursor_y);
    } else {
        desktop_menu_open = false;
    }
}

static void handle_click(void) {
    int32_t scale = desktop_scale();
    int32_t icon_box_w = desktop_icon_box_w();
    int32_t icon_box_h = desktop_icon_box_h();
    int32_t hit = -1;

    if (handle_desktop_menu_left_click()) {
        return;
    }

    if (handle_shell_overlay_click()) {
        return;
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, shell_search_x, shell_search_y, shell_search_w, shell_search_h)) {
        shell_launcher_open = !shell_launcher_open;
        desktop_menu_open = false;
        return;
    }

    if (handle_window_control_click()) {
        return;
    }

    /*
     * Important: consume clicks on window surfaces before desktop objects.
     * This prevents click-through when a window overlaps desktop icons/items.
     */
    hit = window_at_point(state.cursor_x, state.cursor_y);
    if (hit >= 0) {
        state.active_app = hit;
        if (handle_settings_control_click()) {
            return;
        }
        if (handle_notepad_click()) {
            return;
        }
        if (handle_calculator_click()) {
            return;
        }
        if (handle_video_click()) {
            return;
        }
        if (handle_browser_click()) {
            return;
        }
        return;
    }

    if (handle_settings_control_click()) {
        return;
    }
    if (handle_notepad_click()) {
        return;
    }
    if (handle_calculator_click()) {
        return;
    }
    if (handle_video_click()) {
        return;
    }
    if (handle_browser_click()) {
        return;
    }

    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        if (point_in_rect(state.cursor_x,
                          state.cursor_y,
                          desktop_icon_x[i] - 2 * scale,
                          desktop_icon_y[i] - 2 * scale,
                          icon_box_w,
                          icon_box_h)) {
            focus_app(i);
            return;
        }
    }

    int32_t item_hit = desktop_item_at_point(state.cursor_x, state.cursor_y);
    if (item_hit >= 0) {
        struct desktop_item* item = &desktop_items[(uint32_t)item_hit];
        if (item->is_dir) {
            focus_app(APP_FILE_MANAGER);
            terminal_push_line("OPEN DIR");
            terminal_push_line(item->name);
        } else {
            focus_app(APP_NOTEPAD);
            terminal_push_line("OPEN FILE");
            terminal_push_line(item->name);
            if (item->node < VFS_MAX_NODES && vfs_nodes[item->node].used && vfs_nodes[item->node].type == VFS_TYPE_FILE) {
                notepad_load_from_node(item->node);
                if (vfs_nodes[item->node].content[0] != '\0') {
                    terminal_push_line(vfs_nodes[item->node].content);
                }
            }
        }
        return;
    }

    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        if (point_in_rect(state.cursor_x,
                          state.cursor_y,
                          taskbar_icon_x[i],
                          taskbar_icon_y[i],
                          taskbar_icon_size,
                          taskbar_icon_size)) {
            focus_app(i);
            return;
        }
    }

    if (point_in_rect(state.cursor_x, state.cursor_y, settings_btn_x, settings_btn_y, settings_btn_w, settings_btn_h)) {
        focus_app(APP_SETTINGS);
        return;
    }

    state.active_app = -1;
}

static void keyboard_event(uint8_t scancode) {
    if (scancode == 0xE0u) {
        state.e0_prefix = true;
        return;
    }

    bool released = (scancode & 0x80u) != 0u;
    uint8_t code = (uint8_t)(scancode & 0x7Fu);

    if (code < KEY_MAX) {
        if (!released) {
            if (!state.key_down[code]) {
                state.key_pressed[code] = true;
            }
            state.key_down[code] = true;
        } else {
            state.key_down[code] = false;
        }
    }

    if (state.e0_prefix) {
        state.e0_prefix = false;
    }
}

static void mouse_event(uint8_t byte) {
    if (state.mouse_index == 0u && (byte & 0x08u) == 0u) {
        return;
    }

    state.mouse_packet[state.mouse_index++] = byte;
    if (state.mouse_index < 3u) {
        return;
    }
    state.mouse_index = 0u;

    uint8_t flags = state.mouse_packet[0];
    if ((flags & 0xC0u) != 0u) {
        return;
    }

    int32_t dx = (int8_t)state.mouse_packet[1];
    int32_t dy = (int8_t)state.mouse_packet[2];
    dx = clamp_i32(dx, -MOUSE_PACKET_CLAMP, MOUSE_PACKET_CLAMP);
    dy = clamp_i32(dy, -MOUSE_PACKET_CLAMP, MOUSE_PACKET_CLAMP);

    uint32_t cursor_speed = 160u; /* 1600 DPI-like default */
    if (config_get(WEVOA_CFG_CURSOR_SPEED, &cursor_speed) != W_OK) {
        cursor_speed = 160u;
    }
    cursor_speed = (uint32_t)clamp_i32((int32_t)cursor_speed, 40, 240);

    int32_t mag = abs_i32(dx) + abs_i32(dy);
    int32_t sens_fp = (int32_t)(cursor_speed / 12u);
    if (sens_fp < MOUSE_SENS_FP_MIN) {
        sens_fp = MOUSE_SENS_FP_MIN;
    }

    if (mag > 2) {
        sens_fp += (mag - 2) / 2;
    }
    if (mag > 16) {
        sens_fp += (mag - 16) / 2;
    }
    if (sens_fp > MOUSE_SENS_FP_MAX) {
        sens_fp = MOUSE_SENS_FP_MAX;
    }

    int32_t move_x = dx * sens_fp;
    int32_t move_y = dy * sens_fp;
    if (abs_i32(dx) <= 1 && abs_i32(dy) <= 1) {
        move_x = dx * (sens_fp + 2);
        move_y = dy * (sens_fp + 2);
    }

    state.cursor_fx += move_x;
    state.cursor_fy -= move_y;

    bool left = (flags & 0x01u) != 0u;
    bool right = (flags & 0x02u) != 0u;
    state.mouse_left_pressed = left && !state.mouse_left_down;
    state.mouse_right_pressed = right && !state.mouse_right_down;
    state.mouse_left_down = left;
    state.mouse_right_down = right;
}

static void poll_input(void) {
    for (;;) {
        uint8_t status = inb(0x64u);
        if ((status & 0x01u) == 0u) {
            break;
        }

        uint8_t data = inb(0x60u);
        if ((status & 0x20u) != 0u) {
            mouse_event(data);
        } else {
            keyboard_event(data);
        }
    }
}

static bool update_state(void) {
    int32_t old_cursor_x = state.cursor_x;
    int32_t old_cursor_y = state.cursor_y;
    int32_t old_active_app = state.active_app;
    bool old_dragging = state.dragging;
    bool old_desktop_menu_open = desktop_menu_open;
    bool old_settings_wifi_on = state.settings_wifi_on;
    bool old_settings_wifi_connected = state.settings_wifi_connected;
    uint8_t old_settings_brightness = state.settings_brightness;
    uint8_t old_wallpaper_index = state.wallpaper_index;
    uint8_t old_shell_layout = state.shell_layout;
    bool old_shell_launcher_open = shell_launcher_open;
    uint8_t old_ui_blur_level = state.ui_blur_level;
    uint8_t old_ui_motion_level = state.ui_motion_level;
    uint8_t old_ui_effects_quality = state.ui_effects_quality;
    uint8_t old_term_line_count = terminal_state.line_count;
    uint8_t old_term_input_len = terminal_state.input_len;
    uint8_t old_term_cwd = terminal_state.cwd_node;
    uint8_t old_note_len = notepad_state.len;
    bool old_note_dirty = notepad_state.dirty;
    uint8_t old_calc_len = calc_state.expr_len;
    bool old_video_playing = video_state.playing;
    uint8_t old_video_frame_index = video_state.frame_index;
    uint8_t old_browser_url_len = browser_state.url_len;
    bool old_browser_input_focus = browser_state.input_focus;
    uint32_t old_browser_nav_token = browser_nav_token;
    uint8_t old_ui_scale_override = ui_scale_override;
    uint8_t old_desktop_item_count = desktop_item_count;
    bool old_settings_lock_enabled = state.settings_lock_enabled;
    uint16_t old_settings_lock_pin = state.settings_lock_pin;
    bool old_lock_screen_active = lock_screen_active;
    bool old_lock_screen_bad_pin = lock_screen_bad_pin;
    uint8_t old_lock_screen_pin_len = lock_screen_pin_len;
    int32_t old_win_x[APP_COUNT];
    int32_t old_win_y[APP_COUNT];
    int32_t old_win_w[APP_COUNT];
    int32_t old_win_h[APP_COUNT];
    bool old_win_open[APP_COUNT];
    bool old_win_minimized[APP_COUNT];
    bool old_win_maximized[APP_COUNT];

    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        old_win_x[i] = state.windows[i].x;
        old_win_y[i] = state.windows[i].y;
        old_win_w[i] = state.windows[i].w;
        old_win_h[i] = state.windows[i].h;
        old_win_open[i] = state.windows[i].open;
        old_win_minimized[i] = state.windows[i].minimized;
        old_win_maximized[i] = state.windows[i].maximized;
    }

    uint64_t now = rdtsc();
    if (state.last_tsc == 0ull || now < state.last_tsc) {
        state.last_tsc = now;
    }
    uint64_t delta_cycles = now - state.last_tsc;
    state.last_tsc = now;
    if (delta_cycles > 120000000ull) {
        delta_cycles = 120000000ull;
    }

    bool terminal_text_focus = (state.active_app == (int32_t)APP_TERMINAL &&
                                state.windows[APP_TERMINAL].open &&
                                !state.windows[APP_TERMINAL].minimized);
    bool notepad_text_focus = (state.active_app == (int32_t)APP_NOTEPAD &&
                               state.windows[APP_NOTEPAD].open &&
                               !state.windows[APP_NOTEPAD].minimized);
    bool calc_text_focus = (state.active_app == (int32_t)APP_CALCULATOR &&
                            state.windows[APP_CALCULATOR].open &&
                            !state.windows[APP_CALCULATOR].minimized);
    bool browser_text_focus = (state.active_app == (int32_t)APP_BROWSER &&
                               state.windows[APP_BROWSER].open &&
                               !state.windows[APP_BROWSER].minimized);
    bool terminal_open = state.windows[APP_TERMINAL].open && !state.windows[APP_TERMINAL].minimized;
    if (!terminal_text_focus &&
        !notepad_text_focus &&
        !calc_text_focus &&
        !browser_text_focus &&
        terminal_open &&
        terminal_input_key_pressed() &&
        (state.active_app == -1 || state.active_app == (int32_t)APP_TERMINAL)) {
        focus_app(APP_TERMINAL);
        terminal_text_focus = true;
    }

    bool text_focus_any = terminal_text_focus || notepad_text_focus || calc_text_focus || browser_text_focus;

    bool fast_move = state.key_down[SC_LSHIFT] || state.key_down[SC_RSHIFT];
    uint64_t cycles_per_pixel = fast_move ? KEY_MOVE_CYCLES_FAST : KEY_MOVE_CYCLES_NORMAL;
    bool wants_kbd_motion = (!text_focus_any) &&
                            (state.key_down[SC_W] || state.key_down[SC_A] ||
                             state.key_down[SC_S] || state.key_down[SC_D] ||
                             state.key_down[SC_I] || state.key_down[SC_J] ||
                             state.key_down[SC_K] || state.key_down[SC_L]);

    int32_t key_step = 0;
    if (wants_kbd_motion) {
        state.key_move_accum += delta_cycles;
        while (state.key_move_accum >= cycles_per_pixel && key_step < 6) {
            state.key_move_accum -= cycles_per_pixel;
            key_step++;
        }
    } else {
        state.key_move_accum = 0;
    }

    if (key_step > 0) {
        if (state.key_down[SC_W]) {
            state.cursor_fy -= key_step * CURSOR_FP_ONE;
        }
        if (state.key_down[SC_S]) {
            state.cursor_fy += key_step * CURSOR_FP_ONE;
        }
        if (state.key_down[SC_A]) {
            state.cursor_fx -= key_step * CURSOR_FP_ONE;
        }
        if (state.key_down[SC_D]) {
            state.cursor_fx += key_step * CURSOR_FP_ONE;
        }
    }

    int32_t max_fx = ((int32_t)fb_width - 1) << CURSOR_FP_SHIFT;
    int32_t max_fy = ((int32_t)fb_height - 1) << CURSOR_FP_SHIFT;
    state.cursor_fx = clamp_i32(state.cursor_fx, 0, max_fx);
    state.cursor_fy = clamp_i32(state.cursor_fy, 0, max_fy);
    state.cursor_x = state.cursor_fx >> CURSOR_FP_SHIFT;
    state.cursor_y = state.cursor_fy >> CURSOR_FP_SHIFT;

    if (lock_screen_active) {
        bool lock_changed = lock_screen_handle_input();
        bool changed = (state.cursor_x != old_cursor_x) ||
                       (state.cursor_y != old_cursor_y) ||
                       lock_changed ||
                       (lock_screen_active != old_lock_screen_active) ||
                       (lock_screen_bad_pin != old_lock_screen_bad_pin) ||
                       (lock_screen_pin_len != old_lock_screen_pin_len);
        clear_pressed();
        return changed;
    }

    if (!text_focus_any) {
        if (state.key_pressed[SC_1]) {
            focus_app(APP_FILE_MANAGER);
        }
        if (state.key_pressed[SC_2]) {
            focus_app(APP_TERMINAL);
        }
        if (state.key_pressed[SC_3]) {
            focus_app(APP_SETTINGS);
        }
        if (state.key_pressed[SC_4]) {
            focus_app(APP_NOTEPAD);
        }
        if (state.key_pressed[SC_5]) {
            focus_app(APP_CALCULATOR);
        }
        if (state.key_pressed[SC_6]) {
            focus_app(APP_IMAGE_VIEWER);
        }
        if (state.key_pressed[SC_7]) {
            focus_app(APP_VIDEO_PLAYER);
        }
        if (state.key_pressed[SC_8]) {
            focus_app(APP_BROWSER);
        }
        if (state.key_pressed[SC_TAB]) {
            cycle_focus();
        }
        if (state.key_pressed[SC_Q]) {
            close_active_app();
        }
    }

    if (terminal_text_focus) {
        terminal_handle_key_input();
    }
    if (notepad_text_focus) {
        if (notepad_handle_key_input()) {
            mark_pstore_dirty();
        }
    }
    if (calc_text_focus) {
        (void)calc_handle_key_input();
    }
    if (browser_text_focus) {
        (void)browser_handle_key_input();
    }

    if ((state.key_pressed[SC_ENTER] && !text_focus_any) || state.mouse_left_pressed) {
        handle_click();
    }
    if (state.mouse_right_pressed) {
        handle_right_click();
    }
    if (state.key_pressed[SC_ESC]) {
        if (shell_launcher_open) {
            shell_launcher_open = false;
        } else {
            reset_layout();
            build_desktop_template();
        }
    }

    if (state.active_app == (int32_t)APP_SETTINGS &&
        state.windows[APP_SETTINGS].open &&
        !state.windows[APP_SETTINGS].minimized) {
        if (state.key_pressed[SC_F]) {
            int rc = config_set(WEVOA_CFG_WIFI, state.settings_wifi_on ? 0u : 1u);
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
                text_copy(settings_net_status_line, TERM_LINE_MAX, state.settings_wifi_on ? "WIFI ENABLED" : "WIFI DISABLED");
            } else {
                text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
            }
        }
        if (state.key_pressed[SC_C] && state.settings_wifi_on) {
            int rc = config_set(WEVOA_CFG_NET_LINK, state.settings_wifi_connected ? 0u : 1u);
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
                text_copy(settings_net_status_line, TERM_LINE_MAX, state.settings_wifi_connected ? "NETWORK LINK UP" : "NETWORK LINK DOWN");
            } else {
                text_copy(settings_net_status_line, TERM_LINE_MAX, wstatus_str(rc));
            }
        }
        if (state.key_pressed[SC_B]) {
            int32_t v = (int32_t)state.settings_brightness - 5;
            int rc = config_set(WEVOA_CFG_BRIGHT, (uint32_t)clamp_i32(v, 10, 100));
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
            }
        }
        if (state.key_pressed[SC_N]) {
            int32_t v = (int32_t)state.settings_brightness + 5;
            int rc = config_set(WEVOA_CFG_BRIGHT, (uint32_t)clamp_i32(v, 10, 100));
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
            }
        }
        if (state.key_pressed[SC_M]) {
            int rc = config_set(WEVOA_CFG_WALL, state.wallpaper_index + 1u);
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
            }
        }
        if (state.key_pressed[SC_U]) {
            if (ui_scale_override == 0u) {
                ui_scale_override = (uint8_t)desktop_scale();
            }
            if (ui_scale_override > 1u) {
                ui_scale_override--;
            }
            mark_pstore_dirty();
            build_desktop_template();
        }
        if (state.key_pressed[SC_O]) {
            if (ui_scale_override == 0u) {
                ui_scale_override = (uint8_t)desktop_scale();
            }
            if (ui_scale_override < 3u) {
                ui_scale_override++;
            }
            mark_pstore_dirty();
            build_desktop_template();
        }
        if (state.key_pressed[SC_P]) {
            ui_scale_override = 0u;
            mark_pstore_dirty();
            build_desktop_template();
        }
        if (state.key_pressed[SC_T]) {
            uint32_t next = (state.shell_layout == SHELL_LAYOUT_DOCK) ? SHELL_LAYOUT_TASKBAR : SHELL_LAYOUT_DOCK;
            int rc = config_set(WEVOA_CFG_SHELL_LAYOUT, next);
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
                build_desktop_template();
                for (uint32_t i = 0u; i < APP_COUNT; ++i) {
                    limit_window(&state.windows[i]);
                }
            }
        }
        if (state.key_pressed[SC_G]) {
            int32_t v = (int32_t)state.ui_blur_level - 1;
            int rc = config_set(WEVOA_CFG_UI_BLUR_LEVEL, (uint32_t)clamp_i32(v, 0, UI_LEVEL_MAX));
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
                build_desktop_template();
            }
        }
        if (state.key_pressed[SC_Y]) {
            int32_t v = (int32_t)state.ui_blur_level + 1;
            int rc = config_set(WEVOA_CFG_UI_BLUR_LEVEL, (uint32_t)clamp_i32(v, 0, UI_LEVEL_MAX));
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
                build_desktop_template();
            }
        }
        if (state.key_pressed[SC_X]) {
            int32_t v = (int32_t)state.ui_motion_level - 1;
            int rc = config_set(WEVOA_CFG_UI_MOTION_LEVEL, (uint32_t)clamp_i32(v, 0, UI_LEVEL_MAX));
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
            }
        }
        if (state.key_pressed[SC_V]) {
            int32_t v = (int32_t)state.ui_motion_level + 1;
            int rc = config_set(WEVOA_CFG_UI_MOTION_LEVEL, (uint32_t)clamp_i32(v, 0, UI_LEVEL_MAX));
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
            }
        }
        if (state.key_pressed[SC_Z]) {
            uint32_t next = (uint32_t)((state.ui_effects_quality + 1u) % (UI_LEVEL_MAX + 1u));
            int rc = config_set(WEVOA_CFG_UI_EFFECTS_QUALITY, next);
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
                build_desktop_template();
            }
        }
        if (state.key_pressed[SC_L]) {
            int rc = config_set(WEVOA_CFG_LOCK_ENABLED, state.settings_lock_enabled ? 0u : 1u);
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
            }
        }
        if (state.key_pressed[SC_H]) {
            uint32_t pin = (state.settings_lock_pin == 0u) ? 9999u : (uint32_t)(state.settings_lock_pin - 1u);
            int rc = config_set(WEVOA_CFG_LOCK_PIN, pin);
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
            }
        }
        if (state.key_pressed[SC_J]) {
            uint32_t pin = (state.settings_lock_pin >= 9999u) ? 0u : (uint32_t)(state.settings_lock_pin + 1u);
            int rc = config_set(WEVOA_CFG_LOCK_PIN, pin);
            state_sync_from_config();
            if (rc == W_OK) {
                mark_pstore_dirty();
            }
        }
        if (state.key_pressed[SC_R]) {
            lock_screen_activate();
        }
    }

    if (video_state.playing &&
        state.windows[APP_VIDEO_PLAYER].open &&
        !state.windows[APP_VIDEO_PLAYER].minimized) {
        video_state.frame_accum_cycles += delta_cycles;
        while (video_state.frame_accum_cycles >= 85000000ull) {
            video_state.frame_accum_cycles -= 85000000ull;
            video_state.frame_index = (uint8_t)((video_state.frame_index + 1u) % 120u);
        }
    }

    if (state.active_app >= 0 && state.active_app < (int32_t)APP_COUNT) {
        struct app_window* w = &state.windows[(uint32_t)state.active_app];

        if (w->open && !w->minimized) {
            if (key_step > 0 && !w->maximized && !text_focus_any) {
                if (state.key_down[SC_I]) {
                    w->y -= key_step;
                }
                if (state.key_down[SC_K]) {
                    w->y += key_step;
                }
                if (state.key_down[SC_J]) {
                    w->x -= key_step;
                }
                if (state.key_down[SC_L]) {
                    w->x += key_step;
                }
            }

            bool select_down = state.key_down[SC_SPACE] || state.mouse_left_down;
            if (select_down) {
                if (!state.dragging &&
                    !w->maximized &&
                    title_hit(state.active_app, state.cursor_x, state.cursor_y)) {
                    state.dragging = true;
                    state.drag_dx = state.cursor_x - w->x;
                    state.drag_dy = state.cursor_y - w->y;
                }

                if (state.dragging && !w->maximized) {
                    w->x = state.cursor_x - state.drag_dx;
                    w->y = state.cursor_y - state.drag_dy;
                }
            } else {
                state.dragging = false;
            }

            limit_window(w);
        }
    } else {
        state.dragging = false;
    }

    if (state.active_app >= 0 && state.active_app < (int32_t)APP_COUNT) {
        struct app_window* aw = &state.windows[(uint32_t)state.active_app];
        if (!aw->open || aw->minimized) {
            focus_first_visible();
        }
    }

    if (state.settings_brightness != old_settings_brightness) {
        apply_theme_brightness();
    }
    if (state.wallpaper_index != old_wallpaper_index) {
        build_desktop_template();
    }
    if (ui_scale_override != old_ui_scale_override) {
        build_desktop_template();
    }
    if (desktop_item_count != old_desktop_item_count) {
        build_desktop_template();
    }
    if (state.shell_layout != old_shell_layout) {
        build_desktop_template();
    }

    bool changed = (state.cursor_x != old_cursor_x) ||
                   (state.cursor_y != old_cursor_y) ||
                   (state.active_app != old_active_app) ||
                   (state.dragging != old_dragging) ||
                   (desktop_menu_open != old_desktop_menu_open) ||
                   (state.settings_wifi_on != old_settings_wifi_on) ||
                   (state.settings_wifi_connected != old_settings_wifi_connected) ||
                   (state.settings_brightness != old_settings_brightness) ||
                   (state.settings_lock_enabled != old_settings_lock_enabled) ||
                   (state.settings_lock_pin != old_settings_lock_pin) ||
                   (state.wallpaper_index != old_wallpaper_index) ||
                   (state.shell_layout != old_shell_layout) ||
                   (shell_launcher_open != old_shell_launcher_open) ||
                   (state.ui_blur_level != old_ui_blur_level) ||
                   (state.ui_motion_level != old_ui_motion_level) ||
                   (state.ui_effects_quality != old_ui_effects_quality) ||
                   (terminal_state.line_count != old_term_line_count) ||
                   (terminal_state.input_len != old_term_input_len) ||
                   (terminal_state.cwd_node != old_term_cwd) ||
                   (notepad_state.len != old_note_len) ||
                   (notepad_state.dirty != old_note_dirty) ||
                   (calc_state.expr_len != old_calc_len) ||
                   (video_state.playing != old_video_playing) ||
                   (video_state.frame_index != old_video_frame_index) ||
                   (browser_state.url_len != old_browser_url_len) ||
                   (browser_state.input_focus != old_browser_input_focus) ||
                   (browser_nav_token != old_browser_nav_token) ||
                   (lock_screen_active != old_lock_screen_active) ||
                   (lock_screen_bad_pin != old_lock_screen_bad_pin) ||
                   (lock_screen_pin_len != old_lock_screen_pin_len) ||
                   (ui_scale_override != old_ui_scale_override) ||
                   (desktop_item_count != old_desktop_item_count);

    for (uint32_t i = 0; i < APP_COUNT; ++i) {
        if (state.windows[i].x != old_win_x[i] ||
            state.windows[i].y != old_win_y[i] ||
            state.windows[i].w != old_win_w[i] ||
            state.windows[i].h != old_win_h[i] ||
            state.windows[i].open != old_win_open[i] ||
            state.windows[i].minimized != old_win_minimized[i] ||
            state.windows[i].maximized != old_win_maximized[i]) {
            changed = true;
            break;
        }
    }

    if (pstore_available && pstore_dirty) {
        uint64_t save_now = rdtsc();
        if (save_now < pstore_dirty_since_tsc) {
            pstore_dirty_since_tsc = save_now;
        }
        if ((save_now - pstore_dirty_since_tsc) >= PSTORE_AUTOSAVE_CYCLES) {
            int save_rc = pstore_save_runtime();
            if (save_rc == W_OK) {
                pstore_dirty = false;
                terminal_push_ok("AUTOSAVED");
                changed = true;
            } else {
                terminal_push_err(wstatus_str(save_rc));
                pstore_dirty_since_tsc = save_now;
                changed = true;
            }
        }
    }

    clear_pressed();
    return changed;
}

static void idle_pause(void) {
    for (uint32_t i = 0; i < 4000u; ++i) {
        __asm__ volatile ("pause");
    }
}

void gui_run(const struct wevoa_boot_info* info) {
    if (info == 0) {
        return;
    }

    bool mode_vga13 = (info->fb_type == WEVOA_FB_TYPE_VGA13 && info->fb_bpp == 8u);
    bool mode_linear = (info->fb_type == WEVOA_FB_TYPE_LINEAR &&
                        (info->fb_bpp == 24u || info->fb_bpp == 32u));
    if (!mode_vga13 && !mode_linear) {
        return;
    }

    fb_base = (uint8_t*)(uintptr_t)info->fb_phys_addr;
    fb_width = info->fb_width;
    fb_height = info->fb_height;
    fb_pitch = info->fb_pitch;
    fb_bpp_mode = info->fb_bpp;

    if (fb_base == 0 || fb_width == 0u || fb_height == 0u || fb_pitch == 0u) {
        return;
    }

    if (fb_width > BACKBUFFER_MAX_W || fb_height > BACKBUFFER_MAX_H) {
        return;
    }

    for (uint32_t i = 0; i < KEY_MAX; ++i) {
        state.key_down[i] = false;
        state.key_pressed[i] = false;
    }

    state.e0_prefix = false;
    reset_layout();
    int pstore_init_rc = pstore_init((uint8_t)(info->boot_drive & 0xFFu));
    pstore_available = (pstore_init_rc == W_OK) && (pstore_ready() != 0);
    if (pstore_available) {
        int load_rc = pstore_load_runtime();
        if (load_rc == W_OK) {
            terminal_push_ok("STORE RESTORED");
            pstore_dirty = false;
        } else if (load_rc == W_ERR_NOT_FOUND) {
            terminal_push_ok("STORE EMPTY");
        } else if (load_rc == W_ERR_CORRUPT) {
            reset_runtime_data_defaults();
            terminal_push_err("CORRUPT STORE");
            pstore_dirty = true;
            pstore_dirty_since_tsc = rdtsc();
        } else {
            terminal_push_err(wstatus_str(load_rc));
        }
    }
    if (state.settings_lock_enabled) {
        lock_screen_activate();
    }
    mouse_init();

    palette_init();
    build_desktop_template();

    bool force_redraw = true;
    for (;;) {
        uint64_t frame_start = rdtsc();
        poll_input();
        bool changed = update_state();

        if (changed || force_redraw) {
            copy_desktop_to_frame();
            if (!lock_screen_active) {
                draw_taskbar_icons();
                draw_windows();
                draw_desktop_menu();
                perf_draw_overlay();
            } else {
                draw_lock_screen();
            }
            draw_cursor();
            present_frame();
            perf_record_frame(rdtsc() - frame_start);
            while ((rdtsc() - frame_start) < FRAME_TARGET_CYCLES_60FPS) {
                idle_pause();
            }
            force_redraw = false;
        } else {
            idle_pause();
        }
    }
}
