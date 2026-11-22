#pragma once
#include "gdstd.h"
#include "gdfile.h"
#include "GameLevel.h"
#include "TimerOrMotoPartOrMenuElem.h"

class GameCanvas;

/* Ported from rgimad/gravity_defied_cpp. Parses levels.mrg and provides the
 * track/collision queries the physics needs. std:: -> gd::, exceptions removed,
 * EmbedFileStream path dropped (always a real file). Render methods deferred. */
class LevelLoader {
private:
    FileStream stream;
    gd::vector<gd::vector<int>> levelOffsetInFile; /* [league][level] */
    gd::vector<int>             leagueCount;
    bool ok;

    gd::vector<gd::vector<int>> field_121;         /* per-segment unit normals */
    int field_123[3];
    int field_124[3];
    int field_131;
    int field_132;

    void method_96(GameLevel* gl);

public:
    static int  field_133;
    static int  field_134;
    static int  field_135;
    static int  field_136;
    static bool isEnabledPerspective;
    static bool isEnabledShadows;

    gd::vector<gd::vector<gd::string>> levelNames;
    GameLevel* gameLevel;
    int field_137;
    int field_138;

    explicit LevelLoader(const char* mrgPath);
    ~LevelLoader();

    bool isOk() const { return ok; }
    int  leagues() const { return 3; }
    int  levelsIn(int league) const { return leagueCount[league]; }
    const char* getName(int league, int level);
    bool loadLevel(int league, int level);

    int  method_91();
    int  method_92();
    int  method_93();
    int  method_94();
    int  method_95(int var1);
    void setMinMaxX(int minX, int maxX);
    void method_100(int var1, int var2, int var3);
    int  method_101(TimerOrMotoPartOrMenuElem* var1, int var2);

    void renderLevel3D(GameCanvas* gc, int xF16, int yF16);
    void renderTrackNearestLine(GameCanvas* gc);
};
