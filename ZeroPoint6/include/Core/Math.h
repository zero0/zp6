//
// Created by phosg on 1/30/2022.
//

#ifndef ZP_MATH_H
#define ZP_MATH_H

#include "Core/Defines.h"
#include "Core/Types.h"

template<typename T>
constexpr T zp_min( const T& a, const T& b )
{
    return a < b ? a : b;
}

template<typename T>
constexpr T zp_max( const T& a, const T& b )
{
    return a > b ? a : b;
}

template<typename T>
constexpr T zp_clamp( const T& v, const T& a, const T& b )
{
    return v < a ? a : v > b ? b : v;
}

constexpr zp_float32_t zp_clamp01( const zp_float32_t v )
{
    return zp_clamp( v, 0.f, 1.f );
}

constexpr zp_float32_t zp_floor( const zp_float32_t v )
{
    const zp_int32_t vi { static_cast<zp_int32_t>( v ) };
    return static_cast<zp_float32_t>( vi - (static_cast<zp_float32_t >(vi) > v ? 1 : 0));
}

constexpr zp_int32_t zp_floor_to_int( const zp_float32_t v )
{
    const zp_int32_t vi { static_cast<zp_int32_t>( v ) };
    return vi - (static_cast<zp_float32_t >(vi) > v ? 1 : 0);
}

constexpr zp_float32_t zp_ceil( const zp_float32_t v )
{
    const zp_int32_t vi { static_cast<zp_int32_t>( v ) };
    return static_cast<zp_float32_t>( vi + (static_cast<zp_float32_t>(vi) < v ? 1 : 0));
}

constexpr zp_int32_t zp_ceil_to_int( const zp_float32_t v )
{
    const zp_int32_t vi { static_cast<zp_int32_t>( v ) };
    return vi + (static_cast<zp_float32_t>(vi) < v ? 1 : 0);
}

constexpr zp_int32_t zp_sign( const zp_int32_t v )
{
    return v < 0 ? -1 : 1;
}

constexpr zp_int64_t zp_sign( const zp_int64_t v )
{
    return v < 0 ? -1 : 1;
}

constexpr zp_float32_t zp_sign( const zp_float32_t v )
{
    return v < 0.f ? -1.f : 1.f;
}

constexpr zp_float32_t zp_abs( const zp_float32_t v )
{
    return v < 0.f ? -v : v;
}

zp_float32_t zp_sinf( zp_float32_t v );

zp_float32_t zp_cosf( zp_float32_t v );

//
//
//

namespace zp
{
    template<typename T>
    struct Vector2
    {
        T x, y;
    };

    typedef Vector2<zp_int32_t> Vector2i;
    typedef Vector2<zp_float32_t> Vector2f;

    template<typename T>
    struct Vector3
    {
        T x, y, z;
    };

    typedef Vector3<zp_int32_t> Vector3i;
    typedef Vector3<zp_float32_t> Vector3f;

    template<typename T>
    struct Vector4
    {
        T x, y, z, w;
    };

    typedef Vector4<zp_int32_t> Vector4i;
    typedef Vector4<zp_float32_t> Vector4f;

    struct Quaternion
    {
        zp_float32_t x, y, z, w;
    };

    template<typename T>
    struct Rect2D
    {
        T xMin;
        T yMin;

        T xMax;
        T yMax;
    };

    typedef Rect2D<zp_int32_t> Rect2Di;
    typedef Rect2D<zp_float32_t> Rect2Df;

    union Color
    {
        zp_float32_t rgba[4];
        struct
        {
            zp_float32_t r, g, b, a;
        };
    };

    union Color32
    {
        zp_uint32_t rgba;
        struct
        {
            zp_uint8_t r, g, b, a;
        };
    };

    struct Ray
    {
        Vector3f position;
        Vector3f direction;
    };

    struct OptimizedRay
    {
        Vector3f position;
        Vector3f invDirection;
    };

    struct Plane
    {
        Vector3f normal;
        zp_float32_t d;
    };

    struct Frustum
    {
        Plane planes[6];
    };

    struct Matrix4x4f
    {
        union
        {
            struct
            {
                Vector4f r0;
                Vector4f r1;
                Vector4f r2;
                Vector4f r3;
            };
            zp_float32_t m[4][4];
            zp_float32_t v[16];
        };
    };
}

#endif //ZP_MATH_H
