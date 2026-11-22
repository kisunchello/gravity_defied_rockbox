#pragma once
#include "gdstd.h"
#include "TimerOrMotoPartOrMenuElem.h"

class LevelLoader;
class class_10;
class GameCanvas;

/* Ported core of rgimad/gravity_defied_cpp GamePhysics.
 * unique_ptr -> raw owning pointers, std::vector -> gd::vector,
 * hardcoded geometry arrays + render methods deferred to Step 4. */
class GamePhysics {
private:
    int index01 = 0;
    int index10 = 1;
    int field_28 = -1;
    gd::vector<TimerOrMotoPartOrMenuElem*> field_30;   /* 10 */

    int field_31 = 0;
    LevelLoader* levelLoader;
    int field_33 = 0;
    int field_34 = 0;
    bool field_35 = false;
    bool field_36 = false;
    int field_37 = 32768;
    const int field_38 = 3276;
    int field_39 = 0;
    bool field_42 = false;
    gd::vector<TimerOrMotoPartOrMenuElem*> motoComponents; /* 6 */
    int field_44;
    bool isInputAcceleration;
    bool isInputBreak;
    bool isInputBack;
    bool isInputForward;
    bool isInputUp;
    bool isInputDown;
    bool isInputLeft;
    bool isInputRight;
    bool field_68;
    bool isEnableLookAhead;
    int camShiftX;
    int camShiftY;
    int field_73;

    void method_27(int var1, int var2);
    void setInputFromAI();
    void method_35();
    int  method_39(int var1);
    void method_40(int var1);
    void method_42(class_10* var1, TimerOrMotoPartOrMenuElem* var2, class_10* var3, int var4, int var5);
    void method_43(int var1, int var2, int var3);
    void method_44(int var1, int var2, int var3);
    void method_45(int var1);
    int  method_46(int var1);
    void method_47(int var1);
    void renderEngine(GameCanvas* gameCanvas, int var2, int var3);
    void renderMotoFork(GameCanvas* canvas);
    void renderWheelTires(GameCanvas* canvas);
    void renderWheelSpokes(GameCanvas* gameCanvas);
    void renderSmth(GameCanvas* gameCanvas, int var2, int var3, int var4, int var5);
    void renderMotoAsLines(GameCanvas* gameCanvas, int var2, int var3, int var4, int var5);

public:
    inline static int field_7;
    inline static int field_8;
    inline static int field_9;
    inline static int field_10;
    inline static int field_11;
    inline static int motoParam1;
    inline static int motoParam2;
    inline static int field_14;
    inline static int motoParam10;
    inline static int field_16;
    inline static const int const175_1_half[3] = { 114688, 65536, 32768 };
    inline static int motoParam3;
    inline static int field_19;
    inline static int motoParam4;
    inline static int motoParam5;
    inline static int motoParam6;
    inline static int motoParam7;
    inline static int motoParam8;
    inline static int motoParam9;
    gd::vector<class_10*> field_29;   /* 6 */
    bool field_41 = false;
    int field_45;
    bool field_46;
    bool isRenderMotoWithSprites;
    inline static int curentMotoLeague = 0;
    bool field_69;
    bool isGenerateInputAI = false;
    bool aiForceGas = false;   /* menu demo: always accelerate, AI only steers lean */

    GamePhysics(LevelLoader* levelLoader);
    ~GamePhysics();
    int method_21();
    void method_22(int var1);
    void setMode(int mode);
    void setMotoLeague(int league);
    void resetSmth(bool unused);
    void method_26(bool var1);
    void setRenderMinMaxX(int minX, int maxX);
    void processPointerReleased();
    void method_30(int var1, int var2);
    void enableGenerateInputAI();
    void disableGenerateInputAI();
    int updatePhysics();
    bool isTrackStarted();
    bool method_38();
    static int getSmthLikeMaxAbs(int xF16, int yF16);
    void setEnableLookAhead(bool value);
    void setMinimalScreenWH(int minWH);
    int getCamPosX();
    int getCamPosY();
    int method_52();
    void method_53();
    void setMotoComponents();
    void renderGame(GameCanvas* gameCanvas);

    /* expose the 6 visual moto nodes (HUD/diagnostics) */
    TimerOrMotoPartOrMenuElem* getNode(int i) { return motoComponents[i]; }
};
