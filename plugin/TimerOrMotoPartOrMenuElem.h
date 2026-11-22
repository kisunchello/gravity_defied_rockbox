#pragma once
/* Physics-only trim of the original multi-purpose class: keeps just the fields
 * GamePhysics uses (a moto part's position/angle). Menu/text/Graphics parts of
 * the original are dropped until menus are ported. */
class TimerOrMotoPartOrMenuElem {
public:
    int xF16;
    int yF16;
    int angleF16;
    int field_382;
    int field_383;
    int field_384;
    int field_385;
    int field_386;
    int field_387;
    int timerNo;

    TimerOrMotoPartOrMenuElem() { timerNo = 0; setToZeros(); }

    void setToZeros()
    {
        xF16 = yF16 = angleF16 = 0;
        field_382 = field_383 = field_384 = 0;
        field_385 = field_386 = field_387 = 0;
    }
};
