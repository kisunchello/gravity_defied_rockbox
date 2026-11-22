/* Gravity Defied port — menus + game loop.
 * Data + fonts in /.rockbox/rocks/games/gravity/.
 *
 * In game: SELECT=gas, PLAY=brake, clickwheel=lean (auto-gas while leaning,
 * lean held while finger on wheel), LEFT/RIGHT=lean, SELECT+PLAY=toggle look,
 * MENU=pause.
 */
#include <stddef.h>
#include "rbshim.h"
#include "LevelLoader.h"
#include "GamePhysics.h"
#include "GameLevel.h"
#include "GameCanvas.h"

/* bump-pool mark/release from cppsupport.cpp (reclaim a mod run's allocations) */
extern "C" size_t gd_pool_mark(void);
extern "C" void   gd_pool_release(size_t m);

#define DATA_DIR "/.rockbox/rocks/games/gravity/"
#define MRG_PATH DATA_DIR "levels.mrg"

static int animCounter = 0;
static int g_fontTitle = -1;  /* big menu title */
static int g_fontBig = -1;    /* main menu items */
static int g_fontList = -1;   /* level list */

/* background attract demo (AI rider on a gentle level, shown behind the main
 * menu). Its own loader/physics so the game's loader stays free. Light: the
 * physics is stepped slowly (slow-motion) and only while the main menu shows. */
static LevelLoader* g_demoLoader = 0;
static GamePhysics* g_demo = 0;
static int g_demoThrottle = 0;

static bool g_sprite = true;   /* sprite vs line rendering (Options) */
static bool g_persp  = true;   /* 3D perspective (Options) */
static int  g_ctrlMode = 0;    /* clickwheel: 0 = lean+gas, 1 = lean only, 2 = off */
static bool g_demoBg = true;   /* animated bike demo behind the main menu */
static bool g_reachedFinish = false;   /* did the last playLevel cross the finish? */

/* build "prefix + text" into d (small buffers only) */
static void buildLabel(char* d, const char* a, const char* b)
{
    int i = 0;
    while (*a) d[i++] = *a++;
    while (*b && i < 26) d[i++] = *b++;
    d[i] = 0;
}

struct Entry { int league, level; };

/* ---- progression + leaderboard (persisted to progress.dat) ----
 * Three difficulty tiers (0..2 = Easy / Medium / Hard), each with its own bike
 * (setMotoLeague 0..2, faster each tier). Finishing EVERY track in a tier
 * unlocks the next tier + its bike. Each track keeps a top-3 leaderboard
 * (time in frames + 3-char name). cheat = the RKE code unlocks everything. */
#define MAXLV 24
#define LBN   3                        /* leaderboard entries per track */
#define PROG_PATH DATA_DIR "progress.dat"
#define PROG_MAGIC 0x47443204          /* 'GD' v4 */

struct Progress {
    int  magic;
    int  cheat;
    int  done[3][MAXLV];
    int  lbt[3][MAXLV][LBN];           /* times, sorted ascending, -1 = empty */
    char lbn[3][MAXLV][LBN][4];        /* names */
};
static Progress g_prog;
static int g_curTier  = 0;             /* chosen difficulty tier */
static int g_curTrack = 0;             /* chosen track in that tier */
static int g_curBike  = 0;             /* chosen bike (setMotoLeague) */

static const char* TIER_NAME[3] = { "Easy", "Medium", "Hard" };            /* difficulty (Level) */
static const char* BIKE_NAME[4] = { "100cc", "175cc", "220cc", "325cc" };  /* bikes (League); 325cc = bonus */

static void progressReset(void)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < MAXLV; j++) {
            g_prog.done[i][j] = 0;
            for (int k = 0; k < LBN; k++) { g_prog.lbt[i][j][k] = -1; g_prog.lbn[i][j][k][0] = 0; }
        }
    g_prog.cheat = 0;
    g_prog.magic = PROG_MAGIC;
}
static void recordsReset(void)         /* clear only the leaderboards */
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < MAXLV; j++)
            for (int k = 0; k < LBN; k++) { g_prog.lbt[i][j][k] = -1; g_prog.lbn[i][j][k][0] = 0; }
}
static void progressSave(void) { g_prog.magic = PROG_MAGIC; shim_save(PROG_PATH, &g_prog, (int)sizeof(g_prog)); }

static void progressLoad(void)
{
    progressReset();
    int fd = shim_open(PROG_PATH);
    if (fd < 0) return;
    Progress tmp;
    int r = shim_read(fd, &tmp, (int)sizeof(tmp));
    shim_close(fd);
    if (r == (int)sizeof(tmp) && tmp.magic == PROG_MAGIC) g_prog = tmp;
}

/* session settings (last league/track/bike + options) in a SEPARATE file so
 * they persist without touching the leaderboard file. */
#define SET_PATH  DATA_DIR "settings.dat"
#define SET_MAGIC 0x47445302
struct Settings { int magic, tier, track, bike, ctrl, sprite, persp, demobg; };
static void settingsSave(void)
{
    Settings s;
    s.magic = SET_MAGIC; s.tier = g_curTier; s.track = g_curTrack; s.bike = g_curBike;
    s.ctrl = g_ctrlMode; s.sprite = g_sprite ? 1 : 0; s.persp = g_persp ? 1 : 0;
    s.demobg = g_demoBg ? 1 : 0;
    shim_save(SET_PATH, &s, (int)sizeof(s));
}
static void settingsLoad(void)
{
    int fd = shim_open(SET_PATH);
    if (fd < 0) return;
    Settings s;
    int r = shim_read(fd, &s, (int)sizeof(s));
    shim_close(fd);
    if (r != (int)sizeof(s) || s.magic != SET_MAGIC) return;
    g_curTier  = (s.tier  >= 0 && s.tier  < 3) ? s.tier  : 0;
    g_curTrack = (s.track >= 0) ? s.track : 0;
    g_curBike  = (s.bike  >= 0 && s.bike  < 4) ? s.bike  : 0;
    g_ctrlMode = (s.ctrl  >= 0 && s.ctrl  < 3) ? s.ctrl  : 0;
    g_sprite = s.sprite != 0;
    g_persp  = s.persp  != 0;
    g_demoBg = s.demobg != 0;
}

/* best time for a track (-1 = none) */
static int bestTime(int tier, int track) { return g_prog.lbt[tier][track][0]; }

/* insert a time into a track's top-3 (sorted ascending); returns rank or -1 */
static int lbInsert(int tier, int track, int time, const char* nm)
{
    int r = -1;
    for (int i = 0; i < LBN; i++)
        if (g_prog.lbt[tier][track][i] < 0 || time < g_prog.lbt[tier][track][i]) { r = i; break; }
    if (r < 0) return -1;
    for (int i = LBN - 1; i > r; i--) {
        g_prog.lbt[tier][track][i] = g_prog.lbt[tier][track][i - 1];
        for (int k = 0; k < 4; k++) g_prog.lbn[tier][track][i][k] = g_prog.lbn[tier][track][i - 1][k];
    }
    g_prog.lbt[tier][track][r] = time;
    g_prog.lbn[tier][track][r][0] = nm[0];
    g_prog.lbn[tier][track][r][1] = nm[1];
    g_prog.lbn[tier][track][r][2] = nm[2];
    g_prog.lbn[tier][track][r][3] = 0;
    return r;
}

static bool tierUnlocked(LevelLoader& loader, int tier)
{
    if (g_prog.cheat) return true;
    if (tier <= 0) return true;
    int prev = tier - 1, cnt = loader.levelsIn(prev);
    for (int i = 0; i < cnt && i < MAXLV; i++)
        if (!g_prog.done[prev][i]) return false;
    return true;
}
/* every track of every league finished -> unlocks the bonus 325cc bike */
static bool allTracksDone(LevelLoader& loader)
{
    if (g_prog.cheat) return true;
    for (int t = 0; t < loader.leagues(); t++)
        for (int i = 0; i < loader.levelsIn(t) && i < MAXLV; i++)
            if (!g_prog.done[t][i]) return false;
    return true;
}
/* bikes 0..leagues-1 unlock with their tier; the last (bonus) needs everything */
static int bikeCount(LevelLoader& loader) { return loader.leagues() + 1; }
static bool bikeUnlocked(LevelLoader& loader, int bike)
{
    if (bike < loader.leagues()) return tierUnlocked(loader, bike);
    return allTracksDone(loader);
}
/* tracks of one tier */
static int buildLeagueList(LevelLoader& loader, int tier, Entry* out, int maxn)
{
    int n = 0;
    for (int lv = 0; lv < loader.levelsIn(tier) && n < maxn; lv++) {
        out[n].league = tier; out[n].level = lv; n++;
    }
    return n;
}

/* small padlock glyph at (x,y) for locked rows */
static void drawLock(int x, int y)
{
    shim_fillrect(x, y + 4, 10, 7);          /* body */
    shim_line(x + 2, y + 4, x + 2, y + 1);   /* shackle */
    shim_line(x + 2, y + 1, x + 7, y + 1);
    shim_line(x + 7, y + 1, x + 7, y + 4);
}

/* footer hint shown only when the highlighted row (sel) is locked:
 * "Finish all of <prev tier> to unlock <name[sel]>" */
static void drawUnlockHint(LevelLoader& loader, int w, int h, int sel, const char** name)
{
    (void)w;
    if (sel <= 0 || tierUnlocked(loader, sel)) return;
    char msg[48];
    buildLabel(msg, "Finish all of ", TIER_NAME[sel - 1]);
    int l = 0; while (msg[l]) l++;
    const char* t = " to unlock ";
    for (int k = 0; t[k] && l < 44; k++) msg[l++] = t[k];
    const char* nx = name[sel];
    for (int k = 0; nx[k] && l < 47; k++) msg[l++] = nx[k];
    msg[l] = 0;
    int footer = shim_font_height(g_fontList) + 4;
    shim_set_font(g_fontList);
    shim_set_color(120, 120, 120);
    shim_text(10, h - footer, msg);
}

static void drawBg(int w, int h) { shim_set_color(255, 255, 255); shim_fillrect(0, 0, w, h); }

/* startup splash: the logo centred on white for ~2 s (skippable). Loads the
 * image, shows it, then frees it so the game sprites reuse the pool. */
static void splashScreen(int w, int h)
{
    int id = shim_load_sprite(DATA_DIR "logo.bmp");
    int lw = 0, lh = 0;
    if (id >= 0) shim_sprite_size(id, &lw, &lh);
    shim_wait_release();
    for (int f = 0; f < 60; f++) {             /* ~2 s */
        if (shim_poll_event() & (SB_MENU | SB_SELECT | SB_PLAY)) break;
        drawBg(w, h);
        if (id >= 0) shim_blit(id, 0, 0, (w - lw) / 2, (h - lh) / 2, lw, lh);
        shim_set_font(SHIM_FONT_UI);
        shim_set_color(90, 90, 90);
        shim_text_right(w - 8, h - 16, "Gravity Defied  v1.1");
        shim_update();
    }
    shim_sprite_reset();                       /* reclaim the pool */
}

/* set up the background attract demo (an AI rider looping a gentle level) */
static void demoInit(int w, int h)
{
    g_demoLoader = new LevelLoader(MRG_PATH);
    if (!g_demoLoader->isOk() || !g_demoLoader->loadLevel(0, 0)) { g_demo = 0; return; }
    g_demo = new GamePhysics(g_demoLoader);
    g_demo->setMode(1);
    g_demo->setMotoLeague(0);
    g_demo->setMinimalScreenWH(w < h ? w : h);
    g_demo->setEnableLookAhead(false);
    g_demo->method_22(0);              /* line rendering (light, pale look) */
    g_demo->enableGenerateInputAI();   /* self-driving */
    g_demo->aiForceGas = true;         /* always accelerate so it never stalls */
    if (!g_demoBg) return;             /* demo off: skip the warm-up (saves launch CPU) */
    for (int f = 0; f < 90; f++) {     /* warm-up so the bike is already moving */
        int ret = g_demo->updatePhysics();
        g_demo->method_53(); g_demo->setMotoComponents();
        if (ret == 3 || ret == 5 || g_demo->method_38()) g_demo->enableGenerateInputAI();
    }
}

/* restore the demo bike after a race: playing changes the shared static moto
 * params (setMotoLeague), which would otherwise stall the AI on Intro. */
static void demoRestart(void)
{
    if (!g_demo) return;
    g_demo->setMotoLeague(0);          /* restore league-0 params (also turns AI off) */
    g_demo->enableGenerateInputAI();   /* ...so turn the AI back on */
    for (int f = 0; f < 90; f++) {     /* warm it back up so it's already moving */
        int ret = g_demo->updatePhysics();
        g_demo->method_53(); g_demo->setMotoComponents();
        if (ret == 3 || ret == 5 || g_demo->method_38()) g_demo->enableGenerateInputAI();
    }
}

/* draw the demo scene as the menu background (clears to white + draws it).
 * Physics is advanced only every few frames -> slow-motion and low CPU. */
static void demoRender(GameCanvas& canvas, int w, int h)
{
    if (!g_demo) { drawBg(w, h); return; }
    int cx = w / 2, cy = h / 2;
    if (++g_demoThrottle >= 1) {        /* step physics every frame (full demo speed) */
        g_demoThrottle = 0;
        int ret = g_demo->updatePhysics();
        g_demo->method_53();
        g_demo->setMotoComponents();
        /* crash/stuck/finish -> restart. Use enableGenerateInputAI (NOT resetSmth),
         * because resetSmth turns the AI off, which would leave the bike standing. */
        if (ret == 3 || ret == 5 || g_demo->method_38())
            g_demo->enableGenerateInputAI();
    }
    int camX = g_demo->getCamPosX(), camY = g_demo->getCamPosY();
    canvas.setViewPosition(-camX + cx, camY + cy);
    g_demo->setRenderMinMaxX(camX - cx, camX - cx + w);
    canvas.setFaint(true);
    g_demo->renderGame(&canvas);       /* clears white + draws track + rider (pale) */
    canvas.setFaint(false);
}

/* vertical menu: big title on top, indented items below. bgDemo draws the
 * attract demo behind it (main menu only); otherwise plain white. -1 on MENU. */
static int runMenu(GameCanvas& canvas, int w, int h, const char* title,
                   const char* const* items, int count, int startSel, bool bgDemo)
{
    int sel = startSel;
    int fh = shim_font_height(g_fontBig);
    int th = shim_font_height(g_fontTitle);
    int rowH = fh + 8;
    int top = th + 16;
    int rows = (h - top) / rowH;           /* how many fit -> scroll the rest */
    if (rows < 1) rows = 1;
    if (rows > count) rows = count;
    shim_wait_release();
    while (1) {
        int ev = shim_poll_event();
        /* MENU or rewind(LEFT) = back */
        if (ev & (SB_MENU | SB_LEFT)) { shim_set_font(-1); return -1; }
        if (ev & (SB_SCROLL_FWD | SB_RIGHT)) { sel++; if (sel >= count) sel = 0; }
        if (ev & SB_SCROLL_BACK) { sel--; if (sel < 0) sel = count - 1; }
        if (ev & (SB_SELECT | SB_PLAY)) { shim_set_font(-1); return sel; }
        animCounter++;

        int firstRow = sel - rows / 2;
        if (firstRow < 0) firstRow = 0;
        if (firstRow > count - rows) firstRow = count > rows ? count - rows : 0;

        if (bgDemo && g_demoBg) demoRender(canvas, w, h);
        else                    drawBg(w, h);

        shim_set_font(g_fontTitle);
        shim_set_color(20, 20, 20);
        shim_text(12, 6, title);

        shim_set_font(g_fontBig);
        for (int i = 0; i < rows && firstRow + i < count; i++) {
            int idx = firstRow + i;
            int y = top + i * rowH;
            if (idx == sel) {
                canvas.drawMenuHelmet(14, y + (fh - 8) / 2, animCounter / 2);
                shim_set_color(20, 20, 20);
            } else {
                shim_set_color(70, 70, 70);
            }
            shim_text(38, y, items[idx]);
        }
        shim_update();
    }
}

/* 3-letter name entry for the leaderboard. Wheel picks the letter; LEFT/RIGHT
 * only move the cursor; centre(SELECT) advances to the next slot and, on the
 * last slot, saves and moves on. MENU does nothing (no exit from here). */
static void nameEntry(GameCanvas& canvas, int w, int h, char* out)
{
    char ch[3] = { 'A', 'A', 'A' };
    int pos = 0;
    int fh = shim_font_height(g_fontTitle);
    shim_wait_release();
    while (1) {
        int ev = shim_poll_event();
        if (ev & SB_SCROLL_FWD)  { ch[pos]++; if (ch[pos] > 'Z') ch[pos] = 'A'; }
        if (ev & SB_SCROLL_BACK) { ch[pos]--; if (ch[pos] < 'A') ch[pos] = 'Z'; }
        if (ev & SB_LEFT)  { if (pos > 0) pos--; }         /* cursor only */
        if (ev & SB_RIGHT) { if (pos < 2) pos++; }         /* cursor only */
        if (ev & (SB_SELECT | SB_PLAY)) {                  /* centre = advance/confirm */
            if (pos < 2) pos++;
            else break;
        }
        animCounter++;

        drawBg(w, h);
        shim_set_font(g_fontBig);
        shim_set_color(20, 20, 20);
        shim_text(12, 14, "Enter name:");
        shim_set_font(g_fontTitle);
        for (int i = 0; i < 3; i++) {
            int x = w / 2 - 44 + i * 34;
            int c = (i == pos) ? 20 : 150;
            shim_set_color(c, c, c);
            char s[2] = { ch[i], 0 };
            shim_text(x, h / 2 - fh / 2, s);
            if (i == pos) { shim_set_color(20, 20, 20); shim_fillrect(x, h / 2 + fh / 2, 20, 3); }
        }
        shim_set_font(g_fontList);
        shim_set_color(120, 120, 120);
        shim_text(10, h - 20, "wheel=letter  </>=move  OK=next");
        shim_update();
    }
    out[0] = ch[0]; out[1] = ch[1]; out[2] = ch[2]; out[3] = 0;
}

static bool confirmYesNo(GameCanvas& canvas, int w, int h, const char* q)
{
    const char* items[2] = { "No", "Yes" };
    return runMenu(canvas, w, h, q, items, 2, 0, false) == 1;
}

static void messageScreen(int w, int h, const char* l1, const char* l2)
{
    shim_wait_release();
    while (1) {
        if (shim_poll_event() & (SB_MENU | SB_SELECT | SB_PLAY)) break;
        drawBg(w, h);
        shim_set_font(g_fontTitle); shim_set_color(0, 120, 0); shim_text(14, h / 2 - 40, l1);
        if (l2) { shim_set_font(g_fontBig); shim_set_color(20, 20, 20); shim_text(14, h / 2 + 4, l2); }
        shim_set_font(g_fontList); shim_set_color(120, 120, 120); shim_text(14, h - 20, "press any key");
        shim_update();
    }
}

static void optionsMenu(GameCanvas& canvas, int w, int h)
{
    static const char* WHEEL[3] = { "Clickwheel: lean + gas", "Clickwheel: lean only", "Clickwheel: off" };
    int sel = 0;
    while (1) {
        const char* gfx = g_sprite ? "Graphics: Sprites" : "Graphics: Lines";
        const char* d3d = g_persp  ? "3D track: On"      : "3D track: Off";
        const char* dmo = g_demoBg ? "Menu demo: On"     : "Menu demo: Off";
        const char* items[7] = { gfx, d3d, WHEEL[g_ctrlMode % 3], dmo,
                                 "Reset progress", "Reset leaderboard", "Back" };
        int r = runMenu(canvas, w, h, "Options", items, 7, sel, false);
        if (r < 0 || r == 6) return;
        sel = r;                             /* keep the cursor where it was */
        if (r == 0) {
            g_sprite = !g_sprite;
        } else if (r == 1) {
            g_persp = !g_persp;
            LevelLoader::isEnabledPerspective = g_persp;
        } else if (r == 2) {
            g_ctrlMode = (g_ctrlMode + 1) % 3;   /* cycle clickwheel control scheme */
        } else if (r == 3) {
            g_demoBg = !g_demoBg;                /* toggle the menu background demo */
        } else if (r == 4) {
            if (confirmYesNo(canvas, w, h, "Reset progress?")) {
                progressReset(); progressSave();
                g_curTier = g_curTrack = g_curBike = 0;
                messageScreen(w, h, "Progress reset", 0);
            }
        } else if (r == 5) {
            if (confirmYesNo(canvas, w, h, "Reset records?")) {
                recordsReset(); progressSave();
                messageScreen(w, h, "Records cleared", 0);
            }
        }
    }
}

/* pick the difficulty tier (Easy/Medium/Hard). Locked tiers show faint with a
 * padlock; selecting one keeps the current. Returns a chosen tier or -1. */
static int tierSelect(GameCanvas& canvas, LevelLoader& loader, int w, int h)
{
    int sel = g_curTier;
    int fh = shim_font_height(g_fontBig);
    int th = shim_font_height(g_fontTitle);
    int rowH = fh + 10, top = th + 24;
    int cnt = loader.leagues();
    shim_wait_release();
    while (1) {
        int ev = shim_poll_event();
        if (ev & (SB_MENU | SB_LEFT)) { shim_set_font(-1); return -1; }
        if (ev & (SB_SCROLL_FWD | SB_RIGHT)) { sel++; if (sel >= cnt) sel = 0; }
        if (ev & SB_SCROLL_BACK) { sel--; if (sel < 0) sel = cnt - 1; }
        if (ev & (SB_SELECT | SB_PLAY)) {
            if (tierUnlocked(loader, sel)) { shim_set_font(-1); return sel; }
        }
        animCounter++;

        drawBg(w, h);
        shim_set_font(g_fontTitle); shim_set_color(20, 20, 20); shim_text(12, 6, "Level");
        shim_set_font(g_fontBig);
        for (int i = 0; i < cnt; i++) {
            int y = top + i * rowH;
            bool unlk = tierUnlocked(loader, i);
            if (i == sel) { canvas.drawMenuHelmet(14, y + (fh - 8) / 2, animCounter / 2); shim_set_color(unlk ? 20 : 150, unlk ? 20 : 150, unlk ? 20 : 150); }
            else          { shim_set_color(unlk ? 80 : 180, unlk ? 80 : 180, unlk ? 80 : 180); }
            shim_text(38, y, TIER_NAME[i]);
            if (!unlk) { shim_set_color(150, 150, 150); drawLock(w - 26, y); }
        }
        drawUnlockHint(loader, w, h, sel, TIER_NAME);   /* only when a locked row is highlighted */
        shim_update();
    }
}

/* pick the bike (one per tier, unlocked with its tier). Names from the original
 * game (100cc / 175cc / 220cc). Returns bike or -1. */
static int bikeSelect(GameCanvas& canvas, LevelLoader& loader, int w, int h)
{
    int sel = g_curBike;
    int fh = shim_font_height(g_fontBig);
    int th = shim_font_height(g_fontTitle);
    int rowH = fh + 10, top = th + 24;
    int cnt = bikeCount(loader);        /* leagues + the bonus 325cc */
    shim_wait_release();
    while (1) {
        int ev = shim_poll_event();
        if (ev & (SB_MENU | SB_LEFT)) { shim_set_font(-1); return -1; }
        if (ev & (SB_SCROLL_FWD | SB_RIGHT)) { sel++; if (sel >= cnt) sel = 0; }
        if (ev & SB_SCROLL_BACK) { sel--; if (sel < 0) sel = cnt - 1; }
        if (ev & (SB_SELECT | SB_PLAY)) {
            if (bikeUnlocked(loader, sel)) { shim_set_font(-1); return sel; }
        }
        animCounter++;

        drawBg(w, h);
        shim_set_font(g_fontTitle); shim_set_color(20, 20, 20); shim_text(12, 6, "League");
        shim_set_font(g_fontBig);
        for (int i = 0; i < cnt; i++) {
            int y = top + i * rowH;
            bool unlk = bikeUnlocked(loader, i);
            if (i == sel) { canvas.drawMenuHelmet(14, y + (fh - 8) / 2, animCounter / 2); shim_set_color(unlk ? 20 : 150, unlk ? 20 : 150, unlk ? 20 : 150); }
            else          { shim_set_color(unlk ? 80 : 180, unlk ? 80 : 180, unlk ? 80 : 180); }
            shim_text(38, y, BIKE_NAME[i]);
            if (!unlk) { shim_set_color(150, 150, 150); drawLock(w - 26, y); }
        }
        /* unlock hint for the highlighted locked bike */
        if (!bikeUnlocked(loader, sel)) {
            int footer = shim_font_height(g_fontList) + 4;
            shim_set_font(g_fontList); shim_set_color(120, 120, 120);
            if (sel >= loader.leagues())
                shim_text(10, h - footer, "Finish ALL tracks to unlock 325cc");
            else
                drawUnlockHint(loader, w, h, sel, BIKE_NAME);
        }
        shim_update();
    }
}

/* scrolling level list. returns index, or -1 on back. showRecords=false hides
 * the leaderboard columns (used for mod packs, which aren't scored). */
static int levelSelect(GameCanvas& canvas, LevelLoader& loader, int w, int h,
                       Entry* list, int n, int sel, const char* title, bool showRecords,
                       const bool* modDone)
{
    int fh = shim_font_height(g_fontList);
    int rowH = fh + 3;
    int top = fh + 18, footer = fh + 4;   /* more gap below the title */
    int rows = (h - top - footer) / rowH;
    if (rows < 1) rows = 1;
    shim_wait_release();

    while (1) {
        int ev = shim_poll_event();
        if (ev & (SB_MENU | SB_LEFT)) { shim_set_font(-1); return -1; }
        if (ev & (SB_SCROLL_FWD | SB_RIGHT)) { sel++; if (sel >= n) sel = 0; }
        if (ev & SB_SCROLL_BACK) { sel--; if (sel < 0) sel = n - 1; }
        if (ev & (SB_SELECT | SB_PLAY)) { shim_set_font(-1); return sel; }
        animCounter++;

        int firstRow = sel - rows / 2;
        if (firstRow < 0) firstRow = 0;
        if (firstRow > n - rows) firstRow = n > rows ? n - rows : 0;

        drawBg(w, h);
        shim_set_font(g_fontList);
        shim_set_color(20, 20, 20);
        shim_text(12, 8, title);
        /* current bike, top-right (same margin as the track names on the left) */
        shim_set_color(90, 90, 90);
        shim_text_right(w - 20, 8, BIKE_NAME[(g_curBike >= 0 && g_curBike < 4) ? g_curBike : 0]);

        for (int i = 0; i < rows && firstRow + i < n; i++) {
            int idx = firstRow + i;
            int y = top + i * rowH;
            int lg = list[idx].league, lv = list[idx].level;
            if (idx == sel) {
                canvas.drawMenuHelmet(6, y + (fh - 8) / 2, animCounter / 2);
                shim_set_color(20, 20, 20);
            } else {
                shim_set_color(90, 90, 90);
            }
            shim_text(20, y, loader.getName(lg, lv));
            if (showRecords) {
                if (bestTime(lg, lv) >= 0) {
                    shim_text_time(w - 108, y, "", bestTime(lg, lv));
                    if (g_prog.lbn[lg][lv][0][0]) shim_text(w - 46, y, g_prog.lbn[lg][lv][0]);
                } else if (g_prog.done[lg][lv]) {
                    shim_text_right(w - 20, y, "ok");
                }
            } else if (modDone && modDone[idx]) {   /* mod completion, same right margin */
                shim_text_right(w - 20, y, "ok");
            }
        }
        shim_update();
    }
}

static void aboutScreen(int w, int h)
{
    static const char* lines[] = {
        "Gravity Defied Classic",
        "Version 1.1",
        "Rockbox port for iPod Classic 6G",
        "",
        "The legendary 2D motorcycle",
        "trials game. Original game by",
        "Codebrew Software.",
        "",
        "This is an unofficial fan port,",
        "not affiliated with or endorsed",
        "by Codebrew Software. All rights",
        "to the original game, its name",
        "and brand belong to them.",
        "",
        "Based on the C++ reimplementation",
        "by rgimad (gravity_defied_cpp)",
        "and the Android remake AGDTR by",
        "evgenyzinoviev. Level data from",
        "the original .mrg packs.",
        "",
        "Native Rockbox plugin written",
        "in C/C++: integer physics, 3D",
        "tracks, leagues & bikes (100-",
        "325cc), leaderboards, records.",
        "",
        "Built with the help of Claude.",
        "",
        "<< or MENU : back",
    };
    int nlines = (int)(sizeof(lines) / sizeof(lines[0]));
    int flh = shim_font_height(g_fontList);
    int rowH = flh + 2;
    int th = shim_font_height(g_fontTitle);
    int top = th + 10;
    int rows = (h - top) / rowH; if (rows < 1) rows = 1;
    int first = 0;
    shim_wait_release();
    while (1) {
        int ev = shim_poll_event();
        if (ev & (SB_MENU | SB_LEFT | SB_SELECT | SB_PLAY)) { shim_set_font(-1); return; }
        if (ev & (SB_SCROLL_FWD | SB_RIGHT)) { if (first < nlines - rows) first++; }
        if (ev & SB_SCROLL_BACK) { if (first > 0) first--; }

        drawBg(w, h);
        shim_set_font(g_fontTitle); shim_set_color(20, 20, 20); shim_text(12, 6, "About");
        shim_set_font(g_fontList);
        for (int i = 0; i < rows && first + i < nlines; i++) {
            shim_set_color(40, 40, 40);
            shim_text(12, top + i * rowH, lines[first + i]);
        }
        shim_update();
    }
}

/* in-game music controls; stays open so you can skip tracks / change volume
 * without leaving the game (the player's audio keeps running underneath). */
static void musicMenu(GameCanvas& canvas, int w, int h)
{
    const char* items[6] = { "Play / Pause", "Next track", "Prev track",
                             "Volume +", "Volume -", "Back" };
    const int count = 6;
    int sel = 0;
    int fh = shim_font_height(g_fontBig);
    int rowH = fh + 6;
    int top = fh + 28;
    shim_wait_release();
    while (1) {
        int ev = shim_poll_event();
        if (ev & (SB_MENU | SB_LEFT)) { shim_set_font(-1); return; }
        if (ev & (SB_SCROLL_FWD | SB_RIGHT)) { sel++; if (sel >= count) sel = 0; }
        if (ev & SB_SCROLL_BACK) { sel--; if (sel < 0) sel = count - 1; }
        if (ev & (SB_SELECT | SB_PLAY)) {
            switch (sel) {
                case 0: shim_music_playpause(); break;
                case 1: shim_music_next();      break;
                case 2: shim_music_prev();      break;
                case 3: shim_music_vol_step(1); break;
                case 4: shim_music_vol_step(-1);break;
                case 5: shim_set_font(-1);      return;
            }
            shim_wait_release();
        }
        animCounter++;

        drawBg(w, h);
        shim_set_font(g_fontBig);
        shim_set_color(20, 20, 20);
        shim_text(10, 6, "Music");
        int st = shim_music_status();
        const char* ss = st == 1 ? "Playing" : (st == 2 ? "Paused" : "No track");
        shim_set_color(110, 110, 110);
        shim_text(10, top - fh - 2, ss);
        for (int i = 0; i < count; i++) {
            int y = top + i * rowH;
            if (i == sel) {
                canvas.drawMenuHelmet(10, y + (fh - 8) / 2, animCounter / 2);
                shim_set_color(20, 20, 20);
            } else {
                shim_set_color(90, 90, 90);
            }
            shim_text(26, y, items[i]);
        }
        shim_set_color(110, 110, 110);
        shim_text_num(10, top + count * rowH + 4, "Vol", shim_music_volume());
        shim_update();
    }
}

/* finish results screen: faint frozen scene behind + FINISH + time + top-3 +
 * menu. Returns 0 = continue (track list), 1 = restart, 2 = exit to main menu. */
static int resultsScreen(GameCanvas& canvas, GamePhysics* phys, int w, int h,
                         int tier, int track, int finishFrames, int place, bool showLeaders)
{
    static const char* PLACE[3] = { "1st place!", "2nd place!", "3rd place!" };
    const char* items[3] = { "Continue", "Restart", "Exit" };
    int sel = 0, cx = w / 2, cy = h / 2;
    int fh = shim_font_height(g_fontBig);
    int flh = shim_font_height(g_fontList);
    int th = shim_font_height(g_fontTitle);
    shim_wait_release();
    while (1) {
        int ev = shim_poll_event();
        if (ev & (SB_MENU | SB_LEFT)) return -1;    /* back to track list */
        if (ev & (SB_SCROLL_FWD | SB_RIGHT)) { sel++; if (sel >= 3) sel = 0; }
        if (ev & SB_SCROLL_BACK) { sel--; if (sel < 0) sel = 2; }
        if (ev & (SB_SELECT | SB_PLAY)) return sel;  /* 0=Continue(next) 1=Restart 2=Exit */
        animCounter++;

        /* faint frozen game scene as the backdrop (transparent, like the menu) */
        int camX = phys->getCamPosX(), camY = phys->getCamPosY();
        canvas.setViewPosition(-camX + cx, camY + cy);
        phys->setRenderMinMaxX(camX - cx, camX - cx + w);
        canvas.setFaint(true); phys->renderGame(&canvas); canvas.setFaint(false);

        shim_set_font(g_fontTitle); shim_set_color(20, 20, 20); shim_text(12, 4, "FINISH");
        shim_set_font(g_fontBig);
        shim_set_color(20, 20, 20);
        shim_text_time(14, th + 6, "Time ", finishFrames);
        if (place >= 0 && place < 3) {   /* beat a stored best -> show the place */
            shim_set_color(200, 120, 0);
            shim_text(w / 2 + 8, th + 6, PLACE[place]);
        }

        if (showLeaders) {
            int ly = th + 6 + fh + 6;
            shim_set_font(g_fontList); shim_set_color(20, 20, 20); shim_text(14, ly, "Leaders:");
            for (int i = 0; i < LBN; i++) {
                int y = ly + flh + 2 + i * (flh + 2);
                char num[2] = { (char)('1' + i), 0 };
                shim_set_color(90, 90, 90);
                shim_text(18, y, num);
                if (g_prog.lbt[tier][track][i] >= 0) {
                    shim_text(34, y, g_prog.lbn[tier][track][i][0] ? g_prog.lbn[tier][track][i] : "---");
                    shim_text_time(w - 62, y, "", g_prog.lbt[tier][track][i]);
                } else {
                    shim_text(34, y, "---");
                }
            }
        }
        shim_set_font(g_fontBig);
        int my = h - 3 * (fh + 4) - 2;
        for (int i = 0; i < 3; i++) {
            int y = my + i * (fh + 4);
            if (i == sel) { canvas.drawMenuHelmet(14, y + (fh - 8) / 2, animCounter / 2); shim_set_color(20, 20, 20); }
            else            shim_set_color(90, 90, 90);
            shim_text(38, y, items[i]);
        }
        shim_update();
    }
}

/* returns 0 = back to track list, 1 = exit to main menu. scored=false (mod
 * packs): no leaderboard / progression writes, results shows just the time. */
static int playLevel(LevelLoader& loader, GameCanvas& canvas, int w, int h,
                     int league, int level, int bike, bool scored)
{
    int cx = w / 2, cy = h / 2;
    g_reachedFinish = false;
    shim_cpu_boost(1);              /* full CPU clock for the whole race */

restart:
    if (!loader.loadLevel(league, level)) { shim_cpu_boost(0); return 0; }

    GamePhysics* phys = new GamePhysics(&loader);
    phys->setMode(1);
    phys->setEnableLookAhead(false);   /* keep the bike centred (classic camera) */
    phys->setMinimalScreenWH(w < h ? w : h);
    phys->method_22(g_sprite ? 3 : 0);

    const int LEAN_FRAMES = 11;     /* how long a wheel flick keeps leaning (~0.75 s) */
    const int FINISH_DELAY = 75;    /* 1.5 s "FINISH" (transparent) before the results screen */
    int leanDir = 0, leanTimer = 0;
    int frames = 0;                 /* lap time in game frames (~50/s) */
    bool finished = false;
    int finishFrames = 0, finishTimer = 0;
    phys->setMotoLeague(bike);      /* chosen bike */
    shim_set_font(-1);              /* system font for in-game HUD */
    shim_wait_release();
    shim_frame_reset();             /* steady frame pacing (not input-dependent) */

    while (1) {
        int ev = shim_poll_event_nb();   /* non-blocking; pacing is shim_frame_wait */
        if (!finished && (ev & SB_MENU)) {
            const char* pi[4] = { "Resume", "Restart", "Music", "Exit to menu" };
            int p = runMenu(canvas, w, h, "Paused", pi, 4, 0, false);
            shim_wait_release();
            shim_frame_reset();
            if (p == 1) { delete phys; goto restart; }     /* restart level */
            if (p == 2) { musicMenu(canvas, w, h); shim_wait_release(); shim_set_font(-1); shim_frame_reset(); continue; }
            if (p == 3) break;                              /* exit to menu */
            shim_set_font(-1);
            continue;                                       /* resume */
        }

        int vert = 0, horiz = 0;
        if (!finished) {
            if (g_ctrlMode != 2) {       /* clickwheel lean enabled (mode 0/1) */
                if (ev & SB_SCROLL_FWD)  { leanDir = 1;  leanTimer = LEAN_FRAMES; }
                if (ev & SB_SCROLL_BACK) { leanDir = -1; leanTimer = LEAN_FRAMES; }
            }

            int held = shim_held();

            /* keep leaning while the finger stays on the wheel; else decay slowly */
            if (g_ctrlMode != 2 && (held & SB_SCROLL_TOUCH) && leanDir != 0) leanTimer = LEAN_FRAMES;
            if (leanTimer > 0) leanTimer--; else leanDir = 0;

            horiz = leanDir;
            bool leanFromWheel = (leanDir != 0);
            if (held & SB_RIGHT) { horiz = 1; leanFromWheel = false; }   /* button lean: no gas */
            else if (held & SB_LEFT) { horiz = -1; leanFromWheel = false; }

            if (held & SB_SELECT) vert = 1;
            else if (held & SB_PLAY) vert = -1;
            if (g_ctrlMode == 0 && leanFromWheel && vert == 0) vert = 1;   /* auto-gas: wheel mode 0 only */

            frames++;
        }

        phys->method_30(vert, horiz);

        int ret = 0;
        for (int k = 0; k < 2; k++) ret = phys->updatePhysics();
        phys->method_53();
        phys->setMotoComponents();
        if (ret == 5 && !finished) phys->resetSmth(true);   /* no reset once finished (let it flip) */

        /* crossed the finish line -> bike keeps rolling, "FINISH" appears */
        if (!finished && phys->method_38()) {
            finished = true; finishFrames = frames; finishTimer = 0;
        }

        int camX = phys->getCamPosX();
        int camY = phys->getCamPosY();
        canvas.setViewPosition(-camX + cx, camY + cy);
        phys->setRenderMinMaxX(camX - cx, camX - cx + w);
        phys->renderGame(&canvas);

        shim_set_color(0, 0, 0);
        shim_text(2, 2, loader.getName(league, level));
        shim_text_time(w - 62, 2, "", finished ? finishFrames : frames);

        /* track progress bar along the bottom (bike between start & finish) */
        {
            int sx = loader.method_92(), fx = loader.method_91();
            int bx = phys->getNode(0)->xF16;
            int prog = (fx != sx) ? (int)(((int64_t)(bx - sx) * 1000) / (fx - sx)) : 0;
            if (prog < 0) prog = 0;
            if (prog > 1000) prog = 1000;
            int bx0 = 6, bw = w - 12, by = h - 6, bh = 3;
            shim_set_color(205, 205, 205); shim_fillrect(bx0, by, bw, bh);
            shim_set_color(0, 140, 0);     shim_fillrect(bx0, by, bw * prog / 1000, bh);
        }

        if (finished) {
            shim_set_font(g_fontTitle);          /* black FINISH over the scene */
            shim_set_color(0, 0, 0);
            shim_text(cx - 46, cy - 40, "FINISH");
            shim_set_font(-1);
            if (++finishTimer >= FINISH_DELAY) {
                int r = -1;
                if (scored) {        /* base game: leaderboard + progression */
                    /* would this time make the top-3? -> enter a name */
                    for (int i = 0; i < LBN; i++)
                        if (g_prog.lbt[league][level][i] < 0 || finishFrames < g_prog.lbt[league][level][i]) { r = i; break; }
                    bool cheated = false;
                    if (r >= 0) {
                        char nm[4];
                        nameEntry(canvas, w, h, nm);
                        lbInsert(league, level, finishFrames, nm);
                        if (nm[0] == 'R' && nm[1] == 'K' && nm[2] == 'E') { g_prog.cheat = 1; cheated = true; }
                    }
                    /* did this finish unlock the next tier / the bonus bike? */
                    int nextTier = league + 1;
                    bool wasTier  = (nextTier < loader.leagues()) ? tierUnlocked(loader, nextTier) : true;
                    bool wasBonus = allTracksDone(loader);
                    g_prog.done[league][level] = 1;
                    bool nowTier  = (nextTier < loader.leagues()) ? tierUnlocked(loader, nextTier) : true;
                    bool nowBonus = allTracksDone(loader);
                    progressSave();
                    if (cheated) messageScreen(w, h, "ALL UNLOCKED", "code RKE accepted");
                    if (!wasTier && nowTier) {               /* new level + bike */
                        char l2[28]; buildLabel(l2, TIER_NAME[nextTier], " + bike ");
                        int l = 0; while (l2[l]) l++;
                        const char* bn = BIKE_NAME[nextTier];
                        for (int k = 0; bn[k] && l < 27; k++) l2[l++] = bn[k];
                        l2[l] = 0;
                        messageScreen(w, h, "Level unlocked!", l2);
                    } else if (!wasBonus && nowBonus) {       /* finished everything */
                        messageScreen(w, h, "All tracks done!", "Bonus bike 325cc");
                    }
                }
                g_reachedFinish = true;                         /* for mod completion marks */
                int rc = resultsScreen(canvas, phys, w, h, league, level, finishFrames, r, scored);
                if (rc == 0) { delete phys; shim_cpu_boost(0); return 2; }  /* Continue -> next track */
                if (rc == 1) { delete phys; goto restart; }    /* Restart */
                break;                                          /* Exit / back -> track list */
            }
        }
        shim_update();
        shim_frame_wait();          /* keep ~50fps steadily, not input-dependent */
    }
    delete phys;
    shim_cpu_boost(0);
    return 0;
}

/* persisted mod completion: which tracks are done per pack (keyed by filename) */
#define MODPROG_PATH  DATA_DIR "modprog.dat"
#define MODPROG_MAGIC 0x474D5004
#define MAXPACKS 32
struct ModRec  { char name[40]; unsigned char done[256]; };
struct ModProg { int magic; int count; ModRec rec[MAXPACKS]; };
static ModProg g_modProg;

static void modProgLoad(void)
{
    g_modProg.magic = MODPROG_MAGIC; g_modProg.count = 0;
    int fd = shim_open(MODPROG_PATH);
    if (fd < 0) return;
    int r = shim_read(fd, &g_modProg, (int)sizeof(g_modProg));
    shim_close(fd);
    if (r != (int)sizeof(g_modProg) || g_modProg.magic != MODPROG_MAGIC ||
        g_modProg.count < 0 || g_modProg.count > MAXPACKS) {
        g_modProg.magic = MODPROG_MAGIC; g_modProg.count = 0;
    }
}
static void modProgSave(void) { g_modProg.magic = MODPROG_MAGIC; shim_save(MODPROG_PATH, &g_modProg, (int)sizeof(g_modProg)); }

/* find (or create) the completion array for a pack; 0 if the table is full */
static unsigned char* modProgDone(const char* name)
{
    for (int i = 0; i < g_modProg.count; i++) {
        const char* a = g_modProg.rec[i].name;
        bool eq = true;
        for (int k = 0; k < 40; k++) { if (a[k] != name[k]) { eq = false; break; } if (a[k] == 0) break; }
        if (eq) return g_modProg.rec[i].done;
    }
    if (g_modProg.count >= MAXPACKS) return 0;
    ModRec* rc = &g_modProg.rec[g_modProg.count];
    int k = 0; for (; name[k] && k < 39; k++) rc->name[k] = name[k]; rc->name[k] = 0;
    for (int j = 0; j < 256; j++) rc->done[j] = 0;
    g_modProg.count++;
    return rc->done;
}

/* Mods: list user .mrg packs from gravity/mods/, play their tracks (not scored).
 * Each pack + each play runs in its own pool-mark scope so the bump allocator
 * doesn't accumulate across packs/plays. */
#define MODS_DIR DATA_DIR "mods"
static void modsScreen(GameCanvas& canvas, int w, int h)
{
    shim_dir_ensure(MODS_DIR);
    static char buf[64 * 44];
    int cnt = shim_list_mrg(MODS_DIR, buf, (int)sizeof(buf));
    if (cnt <= 0) {
        messageScreen(w, h, "No mod packs", "put .mrg in gravity/mods");
        return;
    }
    if (cnt > 64) cnt = 64;
    static const char* names[64];
    { int off = 0; for (int i = 0; i < cnt; i++) { names[i] = buf + off; while (buf[off]) off++; off++; } }

    int sel = 0;
    while (1) {
        int m = runMenu(canvas, w, h, "Mods", names, cnt, sel, false);
        if (m < 0) return;
        sel = m;

        char path[160];
        { const char* a = MODS_DIR "/"; int i = 0; while (a[i]) { path[i] = a[i]; i++; }
          const char* b = names[m]; int j = 0; while (b[j] && i < 159) path[i++] = b[j++]; path[i] = 0; }

        size_t packMark = gd_pool_mark();
        LevelLoader* pack = new LevelLoader(path);
        if (pack && pack->isOk()) {
            static Entry list[256];
            static bool done[256];
            unsigned char* pd = modProgDone(names[m]);   /* persisted completion */
            int n = 0;
            for (int lg = 0; lg < pack->leagues(); lg++)
                for (int lv = 0; lv < pack->levelsIn(lg) && n < 256; lv++) {
                    list[n].league = lg; list[n].level = lv;
                    done[n] = (pd && pd[n]) ? true : false;
                    n++;
                }
            int s = 0;
            bool pick = true;
            while (n > 0) {
                if (pick) {
                    s = levelSelect(canvas, *pack, w, h, list, n, s, "Mod tracks", false, done);
                    if (s < 0) break;
                }
                size_t playMark = gd_pool_mark();
                LevelLoader* pl = new LevelLoader(path);   /* fresh, transient */
                int pr = 0;
                if (pl && pl->isOk())
                    pr = playLevel(*pl, canvas, w, h, list[s].league, list[s].level, g_curBike, false);
                delete pl;                 /* closes the file */
                gd_pool_release(playMark); /* reclaim this play's memory */
                if (g_reachedFinish && s < 256 && !done[s]) {   /* mark + persist */
                    done[s] = true;
                    if (pd) { pd[s] = 1; modProgSave(); }
                }
                if (pr == 1) break;                                /* exit to main menu */
                if (pr == 2) { if (s + 1 < n) { s++; pick = false; } else pick = true; continue; }  /* next */
                pick = true;                                       /* back -> list */
            }
            if (n == 0) messageScreen(w, h, "Empty pack", names[m]);
        } else {
            messageScreen(w, h, "Cannot open", names[m]);
        }
        delete pack;                   /* closes the pack file */
        gd_pool_release(packMark);
    }
}

extern "C" int cpp_game_run(void)
{
    int w, h;
    shim_screen_size(&w, &h);

    LevelLoader loader(MRG_PATH);
    if (!loader.isOk()) {
        while (1) {
            if (shim_poll_event() & SB_MENU) break;
            drawBg(w, h);
            shim_set_color(200, 40, 40);
            shim_text(6, 20, "levels.mrg not found:");
            shim_text(6, 34, DATA_DIR);
            shim_update();
        }
        return 1;
    }

    GameCanvas canvas(w, h);
    splashScreen(w, h);        /* logo for ~2 s, then frees the sprite pool */
    canvas.loadSprites();
    /* menu items + track list use the player's built-in UI font; only the big
     * title ships its own font */
    g_fontTitle = shim_load_font(DATA_DIR "gravity27b.fnt");
    g_fontBig   = SHIM_FONT_UI;
    g_fontList  = SHIM_FONT_UI;
    if (g_fontTitle < 0) g_fontTitle = SHIM_FONT_UI;

    progressLoad();
    settingsLoad();            /* restore last league/track/bike + options */
    modProgLoad();             /* restore mod track completion */
    LevelLoader::isEnabledPerspective = g_persp;

    demoInit(w, h);            /* background attract demo (honours the Menu demo setting) */

    static Entry list[256];

    while (1) {
        if (!tierUnlocked(loader, g_curTier)) { g_curTier = 0; g_curBike = 0; }
        if (!bikeUnlocked(loader, g_curBike)) g_curBike = g_curTier;   /* keep bike valid */
        int nn = loader.levelsIn(g_curTier);
        if (g_curTrack >= nn) g_curTrack = 0;

        char lvlBuf[24], trkBuf[30], bikeBuf[24];
        buildLabel(lvlBuf,  "Level: ",  TIER_NAME[g_curTier]);
        buildLabel(bikeBuf, "League: ", BIKE_NAME[(g_curBike >= 0 && g_curBike < 4) ? g_curBike : 0]);
        buildLabel(trkBuf,  "Track: ",  loader.getName(g_curTier, g_curTrack));
        const char* items[8] = { "Play", lvlBuf, trkBuf, bikeBuf, "Mods >", "Options >", "About >", "Exit Game" };
        int m = runMenu(canvas, w, h, "MENU", items, 8, 0, true);

        if (m < 0 || m == 7) break;
        if (m == 0 || m == 2) {             /* Play / Track: pick + race (Continue chains) */
            int n = buildLeagueList(loader, g_curTier, list, 256);
            char ttl[20]; buildLabel(ttl, "Track - ", TIER_NAME[g_curTier]);
            int s = g_curTrack; if (s < 0 || s >= n) s = 0;
            bool pick = (m == 2);           /* Track opens the list first; Play starts now */
            while (n > 0) {
                if (pick) {
                    s = levelSelect(canvas, loader, w, h, list, n, s, ttl, true, 0);
                    if (s < 0) break;
                }
                g_curTrack = list[s].level;
                int pr = playLevel(loader, canvas, w, h, list[s].league, list[s].level, g_curBike, true);
                if (pr == 1) break;                                   /* exit to main menu */
                if (pr == 2) { if (s + 1 < n) { s++; pick = false; } else pick = true; continue; }  /* next */
                pick = true;                                          /* back -> list */
            }
            demoRestart();
        } else if (m == 1) {                /* Level: choose the difficulty tier */
            int t = tierSelect(canvas, loader, w, h);
            if (t >= 0) { g_curTier = t; g_curTrack = 0; g_curBike = t; }
        } else if (m == 3) {                /* League: choose the bike */
            int b = bikeSelect(canvas, loader, w, h);
            if (b >= 0) g_curBike = b;
        } else if (m == 4) {                /* Mods: user-supplied .mrg packs */
            modsScreen(canvas, w, h);
            demoRestart();
        } else if (m == 5) {
            optionsMenu(canvas, w, h);
        } else if (m == 6) {
            aboutScreen(w, h);
        }
        settingsSave();     /* persist last league/track/bike + options */
    }

    /* release resources so Rockbox doesn't report leaked file handles on exit */
    delete g_demo;       g_demo = 0;
    delete g_demoLoader; g_demoLoader = 0;   /* closes the demo's level file */
    shim_unload_font(g_fontTitle);           /* font_load keeps the file open */
    return 0;
}
