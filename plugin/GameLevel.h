#pragma once
#include "gdstd.h"
#include "gdfile.h"

class GameCanvas;

/* Ported from rgimad/gravity_defied_cpp (GameLevel). Loads one level's track
 * points from the .mrg stream. Render-through-GameCanvas methods are omitted
 * for now; coordinates are stored as (value << 16 >> 3) == value << 13. */
class GameLevel {
private:
    int minX = 0;
    int maxX = 0;
    int field_264 = 0;
    int field_265 = 0;
    int field_266 = 0;
    int field_277 = 0;

public:
    int startPosX;
    int startPosY;
    int finishPosX;
    int finishPosY;
    int startFlagPoint = 0;
    int finishFlagPoint = 0;
    int pointsCount;
    int field_274;
    gd::vector<gd::vector<int>> pointPositions;

    GameLevel();
    void init();

    int getStartPosX();
    int getStartPosY();
    int getFinishPosX();
    int getFinishPosY();
    int getPointX(int pointNo);
    int getPointY(int pointNo);

    void setMinMaxX(int minX, int maxX);
    int  method_181(int var1);
    void method_183(int var1, int var2);
    void method_184(int var1, int var2, int var3);
    void renderShadow(GameCanvas* gameCanvas, int var2, int var3);
    void renderLevel3D(GameCanvas* gameCanvas, int xF16, int yF16);
    void renderTrackNearestGreenLine(GameCanvas* canvas);
    void addPointSimple(int var1, int var2);
    void addPoint(int x, int y);
    void load(FileStream* inStream);
};
