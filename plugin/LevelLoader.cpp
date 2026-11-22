#include "LevelLoader.h"
#include "GamePhysics.h"   /* getSmthLikeMaxAbs */
#include <limits.h>

int  LevelLoader::field_133 = 0;
int  LevelLoader::field_134 = 0;
int  LevelLoader::field_135 = 0;
int  LevelLoader::field_136 = 0;
bool LevelLoader::isEnabledPerspective = true;
bool LevelLoader::isEnabledShadows = true;

LevelLoader::LevelLoader(const char* mrgPath)
    : ok(false), field_131(0), field_132(0), gameLevel(0), field_137(0), field_138(0)
{
    /* same 3 constants as GamePhysics::const175_1_half */
    static const int c175[3] = { 114688, 65536, 32768 };
    for (int i = 0; i < 3; ++i) {
        field_123[i] = (int)((int64_t)((c175[i] + 19660) >> 1) * (int64_t)((c175[i] + 19660) >> 1) >> 16);
        field_124[i] = (int)((int64_t)((c175[i] - 19660) >> 1) * (int64_t)((c175[i] - 19660) >> 1) >> 16);
    }

    levelOffsetInFile.resize(3);
    levelNames.resize(3);
    leagueCount.resize(3);

    if (!stream.open(mrgPath))
        return;

    for (int league = 0; league < 3; ++league) {
        int cnt = 0;
        stream.readVariable(&cnt, true);
        if (cnt < 0 || cnt > 1000) return;
        leagueCount[league] = cnt;
        levelOffsetInFile[league].resize(cnt);
        levelNames[league].resize(cnt);

        for (int lv = 0; lv < cnt; ++lv) {
            int off = 0;
            stream.readVariable(&off, true);
            levelOffsetInFile[league][lv] = off;

            char name[41];
            int k = 0;
            for (; k < 40; ++k) {
                int8_t b;
                stream.readVariable(&b, true);
                name[k] = (char)b;
                if (b == 0) break;
            }
            name[k] = 0;
            for (int i = 0; i < k; i++)
                if (name[i] == '_') name[i] = ' ';
            levelNames[league][lv] = name;
        }
    }
    ok = true;
}

LevelLoader::~LevelLoader()
{
    delete gameLevel;
}

const char* LevelLoader::getName(int league, int level)
{
    if (league < 0 || league >= 3) return "";
    if (level < 0 || level >= (int)levelNames[league].size()) return "";
    return levelNames[league][level].c_str();
}

void LevelLoader::method_96(GameLevel* gl)
{
    field_131 = INT_MIN;
    this->gameLevel = gl;
    int var2 = gl->pointsCount;
    if (field_121.empty() || field_132 < var2) {
        field_132 = var2 < 100 ? 100 : var2;
        field_121.clear();
        field_121.resize(field_132, gd::vector<int>(2));
    }

    field_133 = 0;
    field_134 = 0;
    field_135 = gl->pointPositions[field_133][0];
    field_136 = gl->pointPositions[field_134][0];

    for (int var3 = 0; var3 < var2; ++var3) {
        int var4 = gl->pointPositions[(var3 + 1) % var2][0] - gl->pointPositions[var3][0];
        int var5 = gl->pointPositions[(var3 + 1) % var2][1] - gl->pointPositions[var3][1];
        if (var3 != 0 && var3 != var2 - 1) {
            field_131 = field_131 < gl->pointPositions[var3][0] ? gl->pointPositions[var3][0] : field_131;
        }

        int var6 = -var5;
        int var8 = GamePhysics::getSmthLikeMaxAbs(var6, var4);
        field_121[var3][0] = (int)(((int64_t)var6 << 32) / (int64_t)var8 >> 16);
        field_121[var3][1] = (int)(((int64_t)var4 << 32) / (int64_t)var8 >> 16);
        if (gl->startFlagPoint == 0 && gl->pointPositions[var3][0] > gl->startPosX) {
            gl->startFlagPoint = var3 + 1;
        }
        if (gl->finishFlagPoint == 0 && gl->pointPositions[var3][0] > gl->finishPosX) {
            gl->finishFlagPoint = var3;
        }
    }

    field_133 = 0;
    field_134 = 0;
    field_135 = 0;
    field_136 = 0;
}

bool LevelLoader::loadLevel(int league, int level)
{
    if (!ok) return false;
    if (league < 0 || league >= 3) return false;
    if (level < 0 || level >= leagueCount[league]) return false;

    stream.setPos(levelOffsetInFile[league][level]);
    if (gameLevel == 0)
        gameLevel = new GameLevel();
    gameLevel->load(&stream);
    method_96(gameLevel);
    return true;
}

int LevelLoader::method_91() { return gameLevel->pointPositions[gameLevel->finishFlagPoint][0] << 1; }
int LevelLoader::method_92() { return gameLevel->pointPositions[gameLevel->startFlagPoint][0] << 1; }
int LevelLoader::method_93() { return gameLevel->startPosX << 1; }
int LevelLoader::method_94() { return gameLevel->startPosY << 1; }
int LevelLoader::method_95(int var1) { return gameLevel->method_181(var1 >> 1); }

void LevelLoader::setMinMaxX(int minX, int maxX) { gameLevel->setMinMaxX(minX, maxX); }

void LevelLoader::renderLevel3D(GameCanvas* gc, int xF16, int yF16)
{
    /* The original Java halves the moto coords in Loader._aiIV *before* calling
     * Level._aiIV (renderLevel3D), so inside the method the bike and the track
     * points share the same 2^13 fixed-point scale. rgimad dropped this wrapper
     * and passed the raw 2^14 moto coords, putting the perspective vanishing
     * point at 2x the bike's position -> track skews/"spins" and the bike floats
     * above the track on steep/long levels. Restore the >>1 to match the original. */
    gameLevel->renderLevel3D(gc, xF16 >> 1, yF16 >> 1);
}
void LevelLoader::renderTrackNearestLine(GameCanvas* gc) { gameLevel->renderTrackNearestGreenLine(gc); }

void LevelLoader::method_100(int var1, int var2, int var3)
{
    gameLevel->method_184((var1 + 98304) >> 1, (var2 - 98304) >> 1, var3 >> 1);
    var2 >>= 1;
    var1 >>= 1;
    field_134 = field_134 < gameLevel->pointsCount - 1 ? field_134 : gameLevel->pointsCount - 1;
    field_133 = field_133 < 0 ? 0 : field_133;
    if (var2 > field_136) {
        while (field_134 < gameLevel->pointsCount - 1 && var2 > gameLevel->pointPositions[++field_134][0]) {
        }
    } else if (var1 < field_135) {
        while (field_133 > 0 && var1 < gameLevel->pointPositions[--field_133][0]) {
        }
    } else {
        while (field_133 < gameLevel->pointsCount && var1 > gameLevel->pointPositions[++field_133][0]) {
        }
        if (field_133 > 0) {
            --field_133;
        }
        while (field_134 > 0 && var2 < gameLevel->pointPositions[--field_134][0]) {
        }
        field_134 = field_134 + 1 < gameLevel->pointsCount - 1 ? field_134 + 1 : gameLevel->pointsCount - 1;
    }

    field_135 = gameLevel->pointPositions[field_133][0];
    field_136 = gameLevel->pointPositions[field_134][0];
}

int LevelLoader::method_101(TimerOrMotoPartOrMenuElem* var1, int var2)
{
    int var16 = 0;
    int8_t var17 = 2;
    int var18 = var1->xF16 >> 1;
    int var19 = var1->yF16 >> 1;
    if (isEnabledPerspective) {
        var19 -= 65536;
    }

    int var20 = 0, var21 = 0;

    for (int var22 = field_133; var22 < field_134; ++var22) {
        int var4 = gameLevel->pointPositions[var22][0];
        int var5 = gameLevel->pointPositions[var22][1];
        int var6 = gameLevel->pointPositions[var22 + 1][0];
        int var7;
        if ((var7 = gameLevel->pointPositions[var22 + 1][1]) < var5) {
            ;
        }

        if (var18 - field_123[var2] <= var6 && var18 + field_123[var2] >= var4) {
            int var8 = var4 - var6;
            int var9 = var5 - var7;
            int var10 = (int)((int64_t)var8 * (int64_t)var8 >> 16) + (int)((int64_t)var9 * (int64_t)var9 >> 16);
            int var11 = (int)((int64_t)(var18 - var4) * (int64_t)(-var8) >> 16) + (int)((int64_t)(var19 - var5) * (int64_t)(-var9) >> 16);
            int var12;
            if ((var10 < 0 ? -var10 : var10) >= 3) {
                var12 = (int)(((int64_t)var11 << 32) / (int64_t)var10 >> 16);
            } else {
                var12 = (var11 > 0 ? 1 : -1) * (var10 > 0 ? 1 : -1) * INT_MAX;
            }

            if (var12 < 0) var12 = 0;
            if (var12 > 65536) var12 = 65536;

            int var13 = var4 + (int)((int64_t)var12 * (int64_t)(-var8) >> 16);
            int var14 = var5 + (int)((int64_t)var12 * (int64_t)(-var9) >> 16);
            var8 = var18 - var13;
            var9 = var19 - var14;
            int8_t var3;
            int64_t var23;
            if ((var23 = ((int64_t)var8 * (int64_t)var8 >> 16) + ((int64_t)var9 * (int64_t)var9 >> 16)) < (int64_t)field_123[var2]) {
                if (var23 >= (int64_t)field_124[var2]) {
                    var3 = 1;
                } else {
                    var3 = 0;
                }
            } else {
                var3 = 2;
            }

            if (var3 == 0 && (int)((int64_t)field_121[var22][0] * (int64_t)var1->field_382 >> 16) + (int)((int64_t)field_121[var22][1] * (int64_t)var1->field_383 >> 16) < 0) {
                field_137 = field_121[var22][0];
                field_138 = field_121[var22][1];
                return 0;
            }

            if (var3 == 1 && (int)((int64_t)field_121[var22][0] * (int64_t)var1->field_382 >> 16) + (int)((int64_t)field_121[var22][1] * (int64_t)var1->field_383 >> 16) < 0) {
                ++var16;
                var17 = 1;
                if (var16 == 1) {
                    var20 = field_121[var22][0];
                    var21 = field_121[var22][1];
                } else {
                    var20 += field_121[var22][0];
                    var21 += field_121[var22][1];
                }
            }
        }
    }

    if (var17 == 1) {
        if ((int)((int64_t)var20 * (int64_t)var1->field_382 >> 16) + (int)((int64_t)var21 * (int64_t)var1->field_383 >> 16) >= 0) {
            return 2;
        }
        field_137 = var20;
        field_138 = var21;
    }

    return var17;
}
