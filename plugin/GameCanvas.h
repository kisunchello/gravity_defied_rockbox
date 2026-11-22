#pragma once
/* Rendering canvas backed by rbshim. Camera via setViewPosition; the engine's
 * coordinate convention is screenX = x + dx, screenY = -y + dy.
 * Sprites are pre-rotated sheets (6 columns); a frame is picked by angle. */
class GameCanvas {
    int dx, dy, width, height;

    bool spritesLoaded;
    int  idSprites, idHelmet, idEngine, idFender, idBody[3];
    int  helmetCW, helmetCH, engineCW, engineCH, fenderCW, fenderCH;
    int  bodyCW[3], bodyCH[3];

    static const int spriteOffsetX[18];
    static const int spriteOffsetY[18];
    static const int spriteSizeX[18];
    static const int spriteSizeY[18];
    static const int startFlagSprite[4];
    static const int finishFlagSprite[4];

public:
    static int field_226;
    static int flagAnimationTime;
    static void method_151();

    GameCanvas(int w, int h);

    bool faint = false;                        /* pale colours (menu demo bg) */
    void setFaint(bool f) { faint = f; }
    void setViewPosition(int dxx, int dyy) { dx = dxx; dy = dyy; }
    int  addDx(int x) { return x + dx; }
    int  addDy(int y) { return -y + dy; }
    int  getWidth()  { return width; }
    int  getHeight() { return height; }
    bool hasSprites() { return spritesLoaded; }

    void loadSprites();

    void setColor(int r, int g, int b);
    void clearScreenWithWhite();
    void drawLine(int x, int y, int x2, int y2);
    void drawLineF16(int x, int y, int x2, int y2);
    void drawArc(int x, int y, int w, int h, int startDeg, int arcDeg);   /* screen coords */
    void fillArc(int x, int y, int w, int h, int startDeg, int arcDeg);   /* screen coords */
    void method_142(int var1, int var2, int var3, int var4);
    void drawCircle(int x, int y, int size);

    /* sprites */
    void drawSprite(int spriteNo, int screenX, int screenY);  /* sprites.png cell */
    void drawForthSpriteByCenter(int centerX, int centerY);
    void drawWheelTires(int x, int y, int wheelIsThin);
    int  calcSpriteNo(int angleF16, int var2, int var3, int var4, bool var5);
    void renderBodyPart(int x1F16, int y1F16, int x2F16, int y2F16, int bodyPartNo);
    void renderBodyPart(int x1F16, int y1F16, int x2F16, int y2F16, int bodyPartNo, int tF16);
    void drawHelmet(int x, int y, int angleF16);
    void renderEngine(int x, int y, int angleF16);
    void renderFender(int x, int y, int angleF16);

    void renderStartFlag(int x, int y);
    void renderFinishFlag(int x, int y);

    /* UI: spinning helmet cursor at screen pos (frame 0..31) */
    void drawMenuHelmet(int sx, int sy, int frame);
    int  helmetCellW() { return helmetCW; }
    int  helmetCellH() { return helmetCH; }
};
