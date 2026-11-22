/* Clean, C++-safe interface to the Rockbox plugin API.
 * The C++ game includes ONLY this header; rbshim.c is the single place that
 * includes plugin.h. This is the seam the lcdui backend will grow from. */
#ifndef RBSHIM_H
#define RBSHIM_H

#ifdef __cplusplus
extern "C" {
#endif

/* button bitmask returned by shim_poll_event() */
enum {
    SB_NONE        = 0,
    SB_SELECT      = 1,
    SB_MENU        = 2,
    SB_LEFT        = 4,
    SB_RIGHT       = 8,
    SB_SCROLL_FWD  = 16,
    SB_SCROLL_BACK = 32,
    SB_PLAY        = 64,
    SB_SCROLL_TOUCH = 128   /* finger resting on the clickwheel */
};

void shim_screen_size(int* w, int* h);
void shim_clear(void);
void shim_set_color(int r, int g, int b);
void shim_pixel(int x, int y);
void shim_fillrect(int x, int y, int w, int h);
void shim_line(int x1, int y1, int x2, int y2);
void shim_text(int x, int y, const char* s);
void shim_text_right(int xright, int y, const char* s);  /* right edge at xright */
void shim_text_num(int x, int y, const char* label, int value);
void shim_update(void);
int  shim_poll_event(void);   /* waits ~one frame, returns SB_* bitmask */
int  shim_poll_event_nb(void);/* non-blocking poll (game loop uses shim_frame_*) */
void shim_frame_reset(void);  /* start the frame pacer */
void shim_frame_wait(void);   /* sleep to keep a steady ~50fps */
void shim_cpu_boost(int on);  /* hold the CPU at full clock during play */
int  shim_held(void);         /* currently-held buttons, SB_* bitmask */
void shim_wait_release(void); /* block until all buttons released (debounce) */

/* fonts (menus). id from shim_load_font; shim_set_font(-1) = system fixed font,
 * shim_set_font(-2 = SHIM_FONT_UI) = the player's UI font (readable fallback) */
#define SHIM_FONT_SYS (-1)
#define SHIM_FONT_UI  (-2)
int  shim_load_font(const char* path);
void shim_unload_font(int id);
void shim_set_font(int id);
int  shim_font_height(int id);

/* sprites (32-bit BMP with alpha, loaded once) */
int  shim_load_sprite(const char* path);   /* returns id >=0, or -1 */
void shim_sprite_reset(void);              /* free all sprites, reset the pool */
void shim_sprite_size(int id, int* w, int* h);
void shim_blit(int id, int sx, int sy, int x, int y, int w, int h);

/* music playback control (the player's own audio keeps running under the game) */
int  shim_music_status(void);   /* 0=stopped/none, 1=playing, 2=paused */
void shim_music_playpause(void);
void shim_music_next(void);
void shim_music_prev(void);
int  shim_music_volume(void);   /* current volume (raw sound units) */
void shim_music_vol_step(int dir); /* dir=+1/-1: nudge volume */

/* append a diagnostic line "msg a b" to /.rockbox/gravity_log.txt */
void shim_log(const char* msg, int a, int b);

/* mods: directory listing */
int  shim_dir_ensure(const char* path);
int  shim_list_mrg(const char* dir, char* out, int outcap);

/* persistence + lap-time drawing */
int  shim_save(const char* path, const void* buf, int len);
void shim_text_time(int x, int y, const char* label, int frames);

/* file I/O (read-only) for level loading */
int  shim_open(const char* path);          /* returns handle >=0, or -1 */
int  shim_read(int fd, void* buf, int n);  /* bytes read, or <0 */
void shim_seek(int fd, int pos);           /* absolute seek from start */
void shim_close(int fd);

#ifdef __cplusplus
}
#endif

#endif
