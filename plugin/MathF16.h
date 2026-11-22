#pragma once
#include <stdint.h>

/* Fixed-point 16.16 math, integer-only (no libm / no float) so it runs in a
 * Rockbox plugin. Angles are in 16.16 radians, same convention as the
 * original Gravity Defied port. */
class MathF16 {
private:
    static int atanF16(int angle);

public:
    static const int PiHalfF16 = 102944;
    static const int PiF16 = 205887;

    static int multiplyF16(int a, int b);
    static int divideF16(int a, int b);

    static int sinF16(int angle);
    static int cosF16(int angle);
    static int atan2F16(int dx, int dy);
};
