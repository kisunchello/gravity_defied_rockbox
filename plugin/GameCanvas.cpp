#include "GameCanvas.h"
#include "MathF16.h"
#include "rbshim.h"
#include <stdint.h>

#define SPR_DIR "/.rockbox/rocks/games/gravity/"

int GameCanvas::field_226 = 0;
int GameCanvas::flagAnimationTime = 0;

const int GameCanvas::spriteOffsetX[18] = { 0, 0, 15, 15, 15, 0, 6, 12, 18, 18, 25, 25, 25, 37, 37, 37, 15, 32 };
const int GameCanvas::spriteOffsetY[18] = { 10, 25, 16, 20, 10, 0, 0, 0, 8, 0, 0, 6, 12, 0, 6, 12, 29, 18 };
const int GameCanvas::spriteSizeX[18]   = { 15, 15, 8, 8, 3, 6, 6, 6, 7, 7, 12, 12, 12, 12, 12, 12, 16, 17 };
const int GameCanvas::spriteSizeY[18]   = { 15, 15, 4, 4, 3, 10, 10, 10, 8, 8, 6, 6, 6, 6, 6, 6, 11, 22 };
const int GameCanvas::startFlagSprite[4]  = { 12, 10, 11, 10 };
const int GameCanvas::finishFlagSprite[4] = { 14, 13, 15, 13 };

GameCanvas::GameCanvas(int w, int h)
    : dx(0), dy(0), width(w), height(h), spritesLoaded(false),
      idSprites(-1), idHelmet(-1), idEngine(-1), idFender(-1),
      helmetCW(0), helmetCH(0), engineCW(0), engineCH(0), fenderCW(0), fenderCH(0)
{
    idBody[0] = idBody[1] = idBody[2] = -1;
    bodyCW[0] = bodyCW[1] = bodyCW[2] = 0;
    bodyCH[0] = bodyCH[1] = bodyCH[2] = 0;
}

void GameCanvas::method_151()
{
    field_226 += 655;
    int s = MathF16::sinF16(field_226);
    int var0 = 32768 + ((s < 0 ? -s : s) >> 1);
    flagAnimationTime += (int)(6553L * (int64_t)var0 >> 16);
}

void GameCanvas::loadSprites()
{
    int w, h;
    idSprites = shim_load_sprite(SPR_DIR "sprites.bmp");
    idHelmet  = shim_load_sprite(SPR_DIR "helmet.bmp");
    shim_sprite_size(idHelmet, &w, &h); helmetCW = w / 6; helmetCH = h / 6;
    idEngine  = shim_load_sprite(SPR_DIR "engine.bmp");
    shim_sprite_size(idEngine, &w, &h); engineCW = w / 6; engineCH = h / 6;
    idFender  = shim_load_sprite(SPR_DIR "fender.bmp");
    shim_sprite_size(idFender, &w, &h); fenderCW = w / 6; fenderCH = h / 6;
    idBody[0] = shim_load_sprite(SPR_DIR "bluearm.bmp");
    shim_sprite_size(idBody[0], &w, &h); bodyCW[0] = w / 6; bodyCH[0] = h / 3;
    idBody[1] = shim_load_sprite(SPR_DIR "blueleg.bmp");
    shim_sprite_size(idBody[1], &w, &h); bodyCW[1] = w / 6; bodyCH[1] = h / 3;
    idBody[2] = shim_load_sprite(SPR_DIR "bluebody.bmp");
    shim_sprite_size(idBody[2], &w, &h); bodyCW[2] = w / 6; bodyCH[2] = h / 3;

    spritesLoaded = (idSprites >= 0 && idHelmet >= 0 && idEngine >= 0 &&
                     idFender >= 0 && idBody[0] >= 0 && idBody[1] >= 0 && idBody[2] >= 0);
}

void GameCanvas::setColor(int r, int g, int b)
{
    if (faint) {                 /* blend ~80% toward white for the menu backdrop */
        r = (r + 255 * 4) / 5;
        g = (g + 255 * 4) / 5;
        b = (b + 255 * 4) / 5;
    }
    shim_set_color(r, g, b);
}

void GameCanvas::clearScreenWithWhite()
{
    shim_set_color(255, 255, 255);
    shim_fillrect(0, 0, width, height);
}

void GameCanvas::drawLine(int x, int y, int x2, int y2)
{
    shim_line(addDx(x), addDy(y), addDx(x2), addDy(y2));
}

void GameCanvas::drawLineF16(int x, int y, int x2, int y2)
{
    shim_line(addDx(x << 2 >> 16), addDy(y << 2 >> 16),
              addDx(x2 << 2 >> 16), addDy(y2 << 2 >> 16));
}

static void arc_point(int cx, int cy, int rx, int ry, int deg, int* ox, int* oy)
{
    int angF16 = (int)(((int64_t)deg * (2 * MathF16::PiF16)) / 360);
    int c = MathF16::cosF16(angF16);
    int s = MathF16::sinF16(angF16);
    *ox = cx + (int)(((int64_t)rx * c) >> 16);
    *oy = cy - (int)(((int64_t)ry * s) >> 16);
}

void GameCanvas::drawArc(int x, int y, int w, int h, int startDeg, int arcDeg)
{
    int cx = x + w / 2, cy = y + h / 2, rx = w / 2, ry = h / 2;
    int steps = (arcDeg < 0 ? -arcDeg : arcDeg) / 12 + 2;
    int px = 0, py = 0;
    for (int i = 0; i <= steps; i++) {
        int nx, ny;
        arc_point(cx, cy, rx, ry, startDeg + arcDeg * i / steps, &nx, &ny);
        if (i > 0) shim_line(px, py, nx, ny);
        px = nx; py = ny;
    }
}

void GameCanvas::fillArc(int x, int y, int w, int h, int startDeg, int arcDeg)
{
    int cx = x + w / 2, cy = y + h / 2, rx = w / 2, ry = h / 2;
    int steps = (arcDeg < 0 ? -arcDeg : arcDeg) / 12 + 2;
    int px = 0, py = 0;
    for (int i = 0; i <= steps; i++) {
        int nx, ny;
        arc_point(cx, cy, rx, ry, startDeg + arcDeg * i / steps, &nx, &ny);
        if (i > 0) { shim_line(cx, cy, nx, ny); shim_line(px, py, nx, ny); }
        px = nx; py = ny;
    }
}

void GameCanvas::method_142(int var1, int var2, int var3, int var4)
{
    ++var3;
    int var5 = addDx(var1 - var3);
    int var6 = addDy(var2 + var3);
    int var7 = var3 << 1;
    if ((var4 = -((int)(((int64_t)((int)((int64_t)var4 * 11796480L >> 16)) << 32) / 205887L >> 16))) < 0) {
        var4 += 360;
    }
    drawArc(var5, var6, var7, var7, (var4 >> 16) + 170, 90);
}

void GameCanvas::drawCircle(int x, int y, int size)
{
    int radius = size / 2;
    drawArc(addDx(x - radius), addDy(y + radius), size, size, 0, 360);
}

/* ---- sprites ---- */

void GameCanvas::drawSprite(int spriteNo, int screenX, int screenY)
{
    if (idSprites < 0) return;
    shim_blit(idSprites, spriteOffsetX[spriteNo], spriteOffsetY[spriteNo],
              screenX, screenY, spriteSizeX[spriteNo], spriteSizeY[spriteNo]);
}

void GameCanvas::drawForthSpriteByCenter(int centerX, int centerY)
{
    int halfX = spriteSizeX[4] / 2;
    int halfY = spriteSizeY[4] / 2;
    drawSprite(4, addDx(centerX - halfX), addDy(centerY + halfY));
}

void GameCanvas::drawWheelTires(int x, int y, int wheelIsThin)
{
    if (!spritesLoaded) { setColor(0, 0, 0); drawCircle(x, y, 14); return; }
    int spriteNo = (wheelIsThin == 1) ? 0 : 1;
    int centerX = addDx(x - spriteSizeX[spriteNo] / 2);
    int centerY = addDy(y + spriteSizeY[spriteNo] / 2);
    drawSprite(spriteNo, centerX, centerY);
}

int GameCanvas::calcSpriteNo(int angleF16, int var2, int var3, int var4, bool var5)
{
    for (angleF16 += var2; angleF16 < 0; angleF16 += var3) {
    }
    while (angleF16 >= var3) {
        angleF16 -= var3;
    }
    if (var5) {
        angleF16 = var3 - angleF16;
    }
    int var6;
    return (var6 = (int)((int64_t)((int)(((int64_t)angleF16 << 32) / (int64_t)var3 >> 16)) * (int64_t)(var4 << 16) >> 16)) >> 16 < var4 - 1 ? var6 >> 16 : var4 - 1;
}

void GameCanvas::renderBodyPart(int x1F16, int y1F16, int x2F16, int y2F16, int bodyPartNo)
{
    renderBodyPart(x1F16, y1F16, x2F16, y2F16, bodyPartNo, 32768);
}

void GameCanvas::renderBodyPart(int x1F16, int y1F16, int x2F16, int y2F16, int bodyPartNo, int tF16)
{
    int x = addDx(((int)((int64_t)x2F16 * (int64_t)tF16 >> 16) + (int)((int64_t)x1F16 * (int64_t)(65536 - tF16) >> 16)) >> 16);
    int y = addDy(((int)((int64_t)y2F16 * (int64_t)tF16 >> 16) + (int)((int64_t)y1F16 * (int64_t)(65536 - tF16) >> 16)) >> 16);
    int angleFP16 = MathF16::atan2F16(x2F16 - x1F16, y2F16 - y1F16);
    int frame = calcSpriteNo(angleFP16, 0, 205887, 16, false);
    if (idBody[bodyPartNo] >= 0) {
        x -= bodyCW[bodyPartNo] / 2;
        y -= bodyCH[bodyPartNo] / 2;
        shim_blit(idBody[bodyPartNo], bodyCW[bodyPartNo] * (frame % 6), bodyCH[bodyPartNo] * (frame / 6), x, y, bodyCW[bodyPartNo], bodyCH[bodyPartNo]);
    }
}

void GameCanvas::drawHelmet(int x, int y, int angleF16)
{
    int frame = calcSpriteNo(angleF16, -102943, 411774, 32, true);
    if (idHelmet >= 0) {
        int sx = addDx(x) - helmetCW / 2;
        int sy = addDy(y) - helmetCH / 2;
        shim_blit(idHelmet, helmetCW * (frame % 6), helmetCH * (frame / 6), sx, sy, helmetCW, helmetCH);
    }
}

void GameCanvas::renderEngine(int x, int y, int angleF16)
{
    int frame = calcSpriteNo(angleF16, -247063, 411774, 32, true);
    if (idEngine >= 0) {
        int sx = addDx(x) - engineCW / 2;
        int sy = addDy(y) - engineCH / 2;
        shim_blit(idEngine, engineCW * (frame % 6), engineCH * (frame / 6), sx, sy, engineCW, engineCH);
    }
}

void GameCanvas::renderFender(int x, int y, int angleF16)
{
    int frame = calcSpriteNo(angleF16, -185297, 411774, 32, true);
    if (idFender >= 0) {
        int sx = addDx(x) - fenderCW / 2;
        int sy = addDy(y) - fenderCH / 2;
        shim_blit(idFender, fenderCW * (frame % 6), fenderCH * (frame / 6), sx, sy, fenderCW, fenderCH);
    }
}

void GameCanvas::drawMenuHelmet(int sx, int sy, int frame)
{
    if (idHelmet < 0) return;
    frame %= 32; if (frame < 0) frame += 32;
    shim_blit(idHelmet, helmetCW * (frame % 6), helmetCH * (frame / 6), sx, sy, helmetCW, helmetCH);
}

void GameCanvas::renderStartFlag(int x, int y)
{
    if (flagAnimationTime > 229376) flagAnimationTime = 0;
    setColor(0, 0, 0);
    drawLine(x, y, x, y + 32);
    if (spritesLoaded) drawSprite(startFlagSprite[flagAnimationTime >> 16], addDx(x), addDy(y) - 32);
    else { setColor(80, 160, 255); shim_fillrect(addDx(x), addDy(y) - 32, 11, 8); }
}

void GameCanvas::renderFinishFlag(int x, int y)
{
    if (flagAnimationTime > 229376) flagAnimationTime = 0;
    setColor(0, 0, 0);
    drawLine(x, y, x, y + 32);
    if (spritesLoaded) drawSprite(finishFlagSprite[flagAnimationTime >> 16], addDx(x), addDy(y) - 32);
    else { setColor(255, 220, 60); shim_fillrect(addDx(x), addDy(y) - 32, 11, 8); }
}
