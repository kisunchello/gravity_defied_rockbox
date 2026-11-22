#pragma once
#include "gdstd.h"
#include "TimerOrMotoPartOrMenuElem.h"

/* Physics node (a point mass carrying 6 moto parts). unique_ptr replaced with
 * raw owning pointers managed by this class. */
class class_10 {
public:
    bool unusedBool;
    int field_257;
    int field_258;
    int field_259;
    int field_260;
    gd::vector<TimerOrMotoPartOrMenuElem*> motoComponents;

    class_10();
    ~class_10();
    void reset();
};
