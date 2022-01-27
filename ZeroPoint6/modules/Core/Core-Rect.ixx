module;

export module Core:Rect;

import :Types;

export namespace zp
{
    struct Rect
    {
        zp_float32_t xMin, yMin, xMax, yMax;

        zp_float32_t width() const { return xMax - xMin; }

        zp_float32_t height() const { return yMax - yMin; }
    };
}