/* The ONLY file that bridges to plugin.h. Compiled as C. */
#include "plugin.h"
#include "rbshim.h"

void shim_screen_size(int* w, int* h) { *w = LCD_WIDTH; *h = LCD_HEIGHT; }
void shim_clear(void)                 { rb->lcd_clear_display(); }
void shim_set_color(int r, int g, int b) { rb->lcd_set_foreground(LCD_RGBPACK(r, g, b)); }
void shim_pixel(int x, int y)         { rb->lcd_drawpixel(x, y); }
void shim_fillrect(int x, int y, int w, int h) { rb->lcd_fillrect(x, y, w, h); }
void shim_line(int x1, int y1, int x2, int y2) { rb->lcd_drawline(x1, y1, x2, y2); }
void shim_text(int x, int y, const char* s) { rb->lcd_putsxy(x, y, (const unsigned char*)s); }
/* draw right-aligned: the string's right edge sits at xright */
void shim_text_right(int xright, int y, const char* s)
{
    int tw = 0, th = 0;
    rb->lcd_getstringsize((const unsigned char*)s, &tw, &th);
    rb->lcd_putsxy(xright - tw, y, (const unsigned char*)s);
}
void shim_text_num(int x, int y, const char* label, int value)
{
    rb->lcd_putsxyf(x, y, "%s %d", label, value);
}
void shim_update(void) { rb->lcd_update(); }

static int map_button(int ev)
{
    int r = 0;
    if (ev & BUTTON_REL) return 0;      /* ignore key-release events (not presses) */
    if (ev & BUTTON_MENU)        r |= SB_MENU;
    if (ev & BUTTON_SELECT)      r |= SB_SELECT;
    if (ev & BUTTON_LEFT)        r |= SB_LEFT;
    if (ev & BUTTON_RIGHT)       r |= SB_RIGHT;
    if (ev & BUTTON_SCROLL_FWD)  r |= SB_SCROLL_FWD;
    if (ev & BUTTON_SCROLL_BACK) r |= SB_SCROLL_BACK;
    if (ev & BUTTON_PLAY)        r |= SB_PLAY;
    return r;
}

/* non-blocking poll (for the game loop, which paces itself with shim_frame_*) */
int shim_poll_event_nb(void) { return map_button(rb->button_get_w_tmo(0)); }

/* keep the CPU at full clock during play (idle otherwise downclocks after ~2s,
 * which slows the physics). Reference-counted: balance each on with an off. */
void shim_cpu_boost(int on)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(on != 0);
#else
    (void)on;
#endif
}

/* steady frame pacing: sleep so each frame is ~20ms (50fps) regardless of input,
 * instead of the input-dependent block below. Yields CPU when idle (saves power);
 * heavy frames (landings) simply don't sleep. */
static long s_frame_next = 0;
void shim_frame_reset(void) { s_frame_next = *rb->current_tick; }
void shim_frame_wait(void)
{
    s_frame_next += HZ / 50;
    long now = *rb->current_tick;
    long d = s_frame_next - now;
    if (d > 0) rb->sleep(d);
    else      s_frame_next = now;       /* behind schedule: don't accumulate */
}

int shim_poll_event(void)
{
    int ev = rb->button_get_w_tmo(HZ / 50);   /* ~50fps (full demo speed); sleeps when idle */
    int r = 0;
    if (ev & BUTTON_REL) return 0;      /* ignore key-release events (not presses) */
    if (ev & BUTTON_MENU)        r |= SB_MENU;
    if (ev & BUTTON_SELECT)      r |= SB_SELECT;
    if (ev & BUTTON_LEFT)        r |= SB_LEFT;
    if (ev & BUTTON_RIGHT)       r |= SB_RIGHT;
    if (ev & BUTTON_SCROLL_FWD)  r |= SB_SCROLL_FWD;
    if (ev & BUTTON_SCROLL_BACK) r |= SB_SCROLL_BACK;
    if (ev & BUTTON_PLAY)        r |= SB_PLAY;
    return r;
}

/* ---- sprites ---- */
#define MAX_SPRITES 10
static unsigned char spr_pool[384 * 1024];
static size_t        spr_off = 0;
static struct bitmap spr_bm[MAX_SPRITES];
static int           spr_n = 0;

int shim_load_sprite(const char* path)
{
    if (spr_n >= MAX_SPRITES) return -1;
    struct bitmap* bm = &spr_bm[spr_n];
    bm->data = spr_pool + spr_off;
    int avail = (int)(sizeof(spr_pool) - spr_off);
    /* load as plain native RGB565; transparency is colour-keyed at blit time */
    int r = rb->read_bmp_file(path, bm, avail, FORMAT_NATIVE, NULL);
    if (r < 0) return -1;
    spr_off += (size_t)((r + 3) & ~3);   /* advance, 4-byte aligned */
    return spr_n++;
}

/* drop all loaded sprites and reclaim the pool (used after the splash logo so
 * the game sprites reuse its space). */
void shim_sprite_reset(void) { spr_off = 0; spr_n = 0; }

void shim_sprite_size(int id, int* w, int* h)
{
    if (id < 0 || id >= spr_n) { *w = 0; *h = 0; return; }
    *w = spr_bm[id].width;
    *h = spr_bm[id].height;
}

void shim_blit(int id, int sx, int sy, int x, int y, int w, int h)
{
    if (id < 0 || id >= spr_n) return;
    /* colour-key transparency: TRANSPARENT_COLOR (magenta) pixels are skipped */
    rb->lcd_bitmap_transparent_part((const fb_data*)spr_bm[id].data,
                                    sx, sy, spr_bm[id].width, x, y, w, h);
}

int shim_held(void)
{
    int st = rb->button_status();
    int r = 0;
    if (st & BUTTON_MENU)        r |= SB_MENU;
    if (st & BUTTON_SELECT)      r |= SB_SELECT;
    if (st & BUTTON_LEFT)        r |= SB_LEFT;
    if (st & BUTTON_RIGHT)       r |= SB_RIGHT;
    if (st & BUTTON_SCROLL_FWD)  r |= SB_SCROLL_FWD;
    if (st & BUTTON_SCROLL_BACK) r |= SB_SCROLL_BACK;
    if (st & BUTTON_PLAY)        r |= SB_PLAY;
#ifdef BUTTON_SCROLL_TOUCH
    if (st & BUTTON_SCROLL_TOUCH) r |= SB_SCROLL_TOUCH;
#endif
    return r;
}

int shim_load_font(const char* path) { return rb->font_load(path); }
void shim_unload_font(int id) { if (id >= 0) rb->font_unload(id); }

/* id >=0: a loaded font. -1: system fixed (small HUD font). -2: the player's
 * UI font (readable fallback when a bundled .fnt is missing on the device). */
static int shim_font_id(int id)
{
    if (id == SHIM_FONT_UI)  return FONT_UI;
    if (id < 0)              return FONT_SYSFIXED;
    return id;
}
void shim_set_font(int id) { rb->lcd_setfont(shim_font_id(id)); }
int shim_font_height(int id)
{
    struct font* f = rb->font_get(shim_font_id(id));
    return f ? f->height : 8;
}

void shim_wait_release(void)
{
    /* drain until no button is held, so a press doesn't carry into the next screen */
    while (rb->button_status() != 0)
        rb->button_get_w_tmo(HZ / 100);
}

/* ---- music playback control (player audio keeps running under the game) ---- */
int shim_music_status(void)
{
    int st = rb->audio_status();
    if (st & AUDIO_STATUS_PAUSE) return 2;
    if (st & AUDIO_STATUS_PLAY)  return 1;
    return 0;
}
void shim_music_playpause(void)
{
    int st = rb->audio_status();
    if (!(st & AUDIO_STATUS_PLAY)) return;      /* nothing loaded */
    if (st & AUDIO_STATUS_PAUSE) rb->audio_resume();
    else                         rb->audio_pause();
}
void shim_music_next(void) { if (rb->audio_status() & AUDIO_STATUS_PLAY) rb->audio_next(); }
void shim_music_prev(void) { if (rb->audio_status() & AUDIO_STATUS_PLAY) rb->audio_prev(); }

int shim_music_volume(void) { return rb->global_status->volume; }

void shim_music_vol_step(int dir)
{
    int vmin = rb->sound_min(SOUND_VOLUME);
    int vmax = rb->sound_max(SOUND_VOLUME);
    int step = (vmax - vmin) / 25;              /* ~25 notches across the range */
    if (step < 1) step = 1;
    int v = rb->global_status->volume + dir * step;
    if (v < vmin) v = vmin;
    if (v > vmax) v = vmax;
    rb->global_status->volume = v;
    rb->sound_set(SOUND_VOLUME, v);
}

/* append one diagnostic line to /.rockbox/gravity_log.txt (closed each call so
 * it survives a later hang/reboot). Used only for rare events (solver bail-out). */
void shim_log(const char* msg, int a, int b)
{
    char buf[96];
    int n = rb->snprintf(buf, sizeof(buf), "%s %d %d\n", msg, a, b);
    int fd = rb->open("/.rockbox/gravity_log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) return;
    rb->write(fd, buf, n);
    rb->close(fd);
}

/* make sure a directory exists (create it if missing). 1 = ok. */
int shim_dir_ensure(const char* path)
{
    if (rb->dir_exists(path)) return 1;
    return rb->mkdir(path) == 0 ? 1 : 0;
}

/* list *.mrg files in a directory into out as NUL-separated names; returns count */
int shim_list_mrg(const char* dir, char* out, int outcap)
{
    DIR* d = rb->opendir(dir);
    if (!d) return 0;
    int count = 0, off = 0;
    struct dirent* e;
    while ((e = rb->readdir(d)) != NULL) {
        const char* nm = e->d_name;
        if (nm[0] == '.') continue;                    /* hidden / ._ AppleDouble files */
        int len = 0; while (nm[len]) len++;
        if (len < 5) continue;                         /* need "x.mrg" */
        const char* x = nm + len - 4;
        if (x[0] != '.' ||
            (x[1] != 'm' && x[1] != 'M') ||
            (x[2] != 'r' && x[2] != 'R') ||
            (x[3] != 'g' && x[3] != 'G')) continue;
        if (off + len + 1 > outcap) break;
        for (int i = 0; i <= len; i++) out[off + i] = nm[i];  /* incl. NUL */
        off += len + 1;
        count++;
    }
    rb->closedir(d);
    return count;
}

/* write a whole file (progress/records). returns bytes written, or -1. */
int shim_save(const char* path, const void* buf, int len)
{
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return -1;
    int w = rb->write(fd, buf, len);
    rb->close(fd);
    return w;
}

/* draw a lap time: "<label>m:ss.cc" from a frame count (50 fps -> 1 frame = 2cs) */
void shim_text_time(int x, int y, const char* label, int frames)
{
    int cs = frames * 2;
    int m = cs / 6000;
    int s = (cs / 100) % 60;
    int c = cs % 100;
    rb->lcd_putsxyf(x, y, "%s%d:%02d.%02d", label, m, s, c);
}

int  shim_open(const char* path)          { return rb->open(path, O_RDONLY); }
int  shim_read(int fd, void* buf, int n)  { return rb->read(fd, buf, n); }
void shim_seek(int fd, int pos)           { rb->lseek(fd, pos, SEEK_SET); }
void shim_close(int fd)                   { if (fd >= 0) rb->close(fd); }

/* plugin entry (C side) -> hands off to the C++ game */
extern int cpp_game_run(void);

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    rb->lcd_set_background(LCD_WHITE);
    rb->lcd_set_drawmode(DRMODE_FG);     /* text draws glyphs only, no solid box behind */
    return cpp_game_run() == 0 ? PLUGIN_OK : PLUGIN_ERROR;
}
