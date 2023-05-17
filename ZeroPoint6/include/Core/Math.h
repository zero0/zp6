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
    return b < a ? a : b;
}

template<typename T>
constexpr T zp_min3( const T& a, const T& b, const T& c )
{
    return zp_min( a, zp_min( b, c ) );
}

template<typename T>
constexpr T zp_max3( const T& a, const T& b, const T& c )
{
    return zp_max( a, zp_max( b, c ) );
}

template<typename T>
constexpr T zp_clamp( const T& v, const T& a, const T& b )
{
    return v < a ? a : b < v ? b : v;
}

constexpr zp_float32_t zp_clamp01( const zp_float32_t v )
{
    return zp_clamp( v, 0.f, 1.f );
}

constexpr zp_float32_t zp_floor( const zp_float32_t v )
{
    const zp_int32_t vi { static_cast<zp_int32_t>( v ) };
    return static_cast<zp_float32_t>( vi - ( static_cast<zp_float32_t >(vi) > v ? 1 : 0 ));
}

constexpr zp_int32_t zp_floor_to_int( const zp_float32_t v )
{
    const zp_int32_t vi { static_cast<zp_int32_t>( v ) };
    return vi - ( static_cast<zp_float32_t>(vi) > v ? 1 : 0 );
}

constexpr zp_float32_t zp_ceil( const zp_float32_t v )
{
    const zp_int32_t vi { static_cast<zp_int32_t>( v ) };
    return static_cast<zp_float32_t>( vi + ( static_cast<zp_float32_t>(vi) < v ? 1 : 0 ));
}

constexpr zp_int32_t zp_ceil_to_int( const zp_float32_t v )
{
    const zp_int32_t vi { static_cast<zp_int32_t>( v ) };
    return vi + ( static_cast<zp_float32_t>(vi) < v ? 1 : 0 );
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

constexpr zp_int32_t zp_abs( const zp_int32_t v )
{
    return v < 0 ? -v : v;
}

constexpr zp_float32_t zp_abs( const zp_float32_t v )
{
    return v < 0.f ? -v : v;
}

constexpr zp_float32_t zp_rcp( const zp_float32_t v )
{
    return 1.f / v;
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
        union
        {
            T m[2];
            struct
            {
                T x, y;
            };
        };

        static const Vector2 zero;
        static const Vector2 one;
    };

    typedef Vector2<zp_int32_t> Vector2i;

    typedef Vector2<zp_float32_t> Vector2f;

    template<typename T>
    struct Vector3
    {
        union
        {
            T m[3];
            struct
            {
                T x, y, z;
            };
        };

        static const Vector3 zero;
        static const Vector3 one;
    };

    typedef Vector3<zp_int32_t> Vector3i;

    typedef Vector3<zp_float32_t> Vector3f;

    template<typename T>
    struct Vector4
    {
        union
        {
            T m[4];
            struct
            {
                T x, y, z, w;
            };
        };

        static const Vector4 zero;
        static const Vector4 one;
    };

    typedef Vector4<zp_int32_t> Vector4i;

    typedef Vector4<zp_float32_t> Vector4f;

    struct Quaternion
    {
        zp_float32_t x, y, z, w;

        static const Quaternion identity;
    };

    template<typename T>
    struct Offset2D
    {
        T x, y;
    };

    typedef Offset2D<zp_int32_t> Offset2Di;

    typedef Offset2D<zp_float32_t> Offset2Df;

    template<typename T>
    struct Size2D
    {
        T width, height;
    };

    typedef Size2D<zp_int32_t> Size2Di;

    typedef Size2D<zp_float32_t> Size2Df;

    template<typename T>
    struct Bounds2D
    {
        T xMin, yMin;
        T xMax, yMax;

        void encapsulate( const Vector2<T>& value )
        {
            xMin = zp_min( xMin, value.x );
            yMin = zp_min( yMin, value.y );

            xMax = zp_max( xMax, value.x );
            yMax = zp_max( yMax, value.y );
        }

        void encapsulate( const Offset2D<T>& value )
        {
            xMin = zp_min( xMin, value.x );
            yMin = zp_min( yMin, value.y );

            xMax = zp_max( xMax, value.x );
            yMax = zp_max( yMax, value.y );
        }

        Offset2D<T> min() const
        {
            return { xMin, yMin };
        }

        Offset2D<T> max() const
        {
            return { xMax, yMax };
        }

        Size2D<T> size() const
        {
            return { xMax - xMin, yMax - yMin };
        }

        Offset2D<T> center() const
        {
            return {
                ( xMin + xMax ) / static_cast<T>( 2 ),
                ( yMin + yMax ) / static_cast<T>( 2 )
            };
        }
    };

    typedef Bounds2D<zp_int32_t> Bounds2Di;

    typedef Bounds2D<zp_float32_t> Bounds2Df;

    template<typename T>
    struct Rect2D
    {
        Offset2D<T> offset;
        Size2D<T> size;

        Offset2D<T> min() const
        {
            return offset;
        }

        Offset2D<T> max() const
        {
            return { .x = offset.x + size.width, .y = offset.y + size.height };
        }

        Offset2D<T> center() const
        {
            return {
                .x = ( offset.x + size.width ) / static_cast<T>( 2 ),
                .y = ( offset.y + size.height ) / static_cast<T>( 2 )
            };
        }

        T left() const
        {
            return offset.x;
        }

        T right() const
        {
            return offset.x + size.width;
        }

        T top() const
        {
            return offset.y + size.height;
        }

        T bottom() const
        {
            return offset.y;
        }

        Offset2D<T> topLeft() const
        {
            return {
                .x = offset.x,
                .y = offset.y + size.height
            };
        }

        Offset2D<T> topRight() const
        {
            return {
                .x = offset.x + size.width,
                .y = offset.y + size.height
            };
        }

        Offset2D<T> bottomLeft() const
        {
            return {
                .x = offset.x,
                .y = offset.y
            };
        }

        Offset2D<T> bottomRight() const
        {
            return {
                .x = offset.x + size.width,
                .y = offset.y
            };
        }
    };

    typedef Rect2D<zp_int32_t> Rect2Di;

    typedef Rect2D<zp_float32_t> Rect2Df;

    template<typename T>
    struct Size3D
    {
        T width, height, depth;
    };

    typedef Size3D<zp_int32_t> Size3Di;

    typedef Size3D<zp_float32_t> Size3Df;

    template<typename T>
    struct Offset3D
    {
        T x, y, z;
    };

    typedef Offset3D<zp_int32_t> Offset3Di;

    typedef Offset3D<zp_float32_t> Offset3Df;

    template<typename T>
    struct Bounds3D
    {
        T xMin, yMin, zMin;
        T xMax, yMax, zMax;

        const static Bounds3D invalid;
        const static Bounds3D empty;

        void encapsulate( const Vector3<T>& value )
        {
            xMin = zp_min( xMin, value.x );
            yMin = zp_min( yMin, value.y );
            zMin = zp_min( zMin, value.z );

            xMax = zp_max( xMax, value.x );
            yMax = zp_max( yMax, value.y );
            zMax = zp_max( zMax, value.z );
        }

        void encapsulate( const Offset3D<T>& value )
        {
            xMin = zp_min( xMin, value.x );
            yMin = zp_min( yMin, value.y );
            zMin = zp_min( zMin, value.z );

            xMax = zp_max( xMax, value.x );
            yMax = zp_max( yMax, value.y );
            zMax = zp_max( zMax, value.z );
        }

        Offset3D<T> min() const
        {
            return { xMin, yMin, zMin };
        }

        Offset3D<T> max() const
        {
            return { xMax, yMax, zMax };
        }

        Size3D<T> size() const
        {
            return { xMax - xMin, yMax - yMin, zMax - zMin };
        }

        Size3D<T> extents() const
        {
            return {
                ( xMax - xMin ) / static_cast<T>( 2 ),
                ( yMax - yMin ) / static_cast<T>( 2 ),
                ( zMax - zMin ) / static_cast<T>( 2 )
            };
        }

        Offset3D<T> center() const
        {
            return {
                ( xMin + xMax ) / static_cast<T>( 2 ),
                ( yMin + yMax ) / static_cast<T>( 2 ),
                ( zMin + zMax ) / static_cast<T>( 2 )
            };
        }
    };

    typedef Bounds3D<zp_int32_t> Bounds3Di;

    typedef Bounds3D<zp_float32_t> Bounds3Df;

    struct Color
    {
        union
        {
            zp_float32_t rgba[4];
            struct
            {
                zp_float32_t r, g, b, a;
            };
        };

        static const Color clear;
        static const Color black;
        static const Color white;

        static const Color red;
        static const Color green;
        static const Color blue;

        static const Color yellow;
        static const Color cyan;
        static const Color magenta;

        static const Color gray25;
        static const Color gray50;
        static const Color gray75;
    };

    struct Color32
    {
        union
        {
            zp_uint32_t rgba;
            struct
            {
                zp_uint8_t r, g, b, a;
            };
        };

        static const Color32 clear;
        static const Color32 black;
        static const Color32 white;

        static const Color32 red;
        static const Color32 green;
        static const Color32 blue;

        static const Color32 yellow;
        static const Color32 cyan;
        static const Color32 magenta;

        static const Color32 gray25;
        static const Color32 gray50;
        static const Color32 gray75;
    };


    template<typename T>
    struct Ray3D
    {
        Vector3<T> position;
        Vector3<T> direction;
    };

    typedef Ray3D<zp_float32_t> Ray3Df;

    template<typename T>
    struct OptimizedRay3D
    {
        Vector3<T> position;
        Vector3<T> invDirection;
    };

    typedef OptimizedRay3D<zp_float32_t> OptimizedRay3Df;

    template<typename T>
    struct Plane3D
    {
        Vector3<T> normal;
        T d;
    };

    typedef Plane3D<zp_float32_t> Plane3Df;

    struct Frustum
    {
        Plane3Df planes[6];
    };

    template<typename T>
    struct Matrix4x4
    {
        union
        {
            struct
            {
                Vector4<T> c0;
                Vector4<T> c1;
                Vector4<T> c2;
                Vector4<T> c3;
            };
            Vector4<T> c[4];
            T m[4][4];
            T v[16];
        };

        static const Matrix4x4<T> identity;
        static const Matrix4x4<T> zero;
    };

    typedef Matrix4x4<zp_float32_t> Matrix4x4f;

    namespace Math
    {

        constexpr Vector2f Vec2f( zp_float32_t x, zp_float32_t y )
        {
            return { .x = x, .y = y };
        }

        constexpr Vector2f Vec2f( const Vector3f& v )
        {
            return { .x = v.x, .y = v.y };
        }

        constexpr Vector2f Vec2f( const Vector4f& v )
        {
            return { .x = v.x, .y = v.y };
        }

        constexpr Vector3f Vec3f( zp_float32_t x, zp_float32_t y, zp_float32_t z )
        {
            return { .x = x, .y = y, .z = z };
        }

        constexpr Vector3f Vec3f( const Vector2f& v, zp_float32_t z )
        {
            return { .x = v.x, .y = v.y, .z = z };
        }

        constexpr Vector3f Vec3f( const Vector4f& v )
        {
            return { .x = v.x, .y = v.y, .z = v.z };
        }

        constexpr Vector4f Vec4f( zp_float32_t x, zp_float32_t y, zp_float32_t z, zp_float32_t w )
        {
            return { .x = x, .y = y, .z = z, .w = w };
        }

        constexpr Vector4f Vec4f( const Vector2f& v, zp_float32_t z, zp_float32_t w )
        {
            return { .x = v.x, .y = v.y, .z = z, .w = w };
        }

        constexpr Vector4f Vec4f( const Vector3f& v, zp_float32_t w )
        {
            return { .x = v.x, .y = v.y, .z = v.z, .w = w };
        }

        Vector3f PerspectiveDivide( const Vector4f& v );

        Color Lerp( const Color& x, const Color& y, zp_float32_t a );

        Color LerpUnclamp( const Color& x, const Color& y, zp_float32_t a );

        Color LerpExact( const Color& x, const Color& y, zp_float32_t a );

        Color Mul( const Color& x, const Color& y );

        Matrix4x4f Mul( const Matrix4x4f& lh, const Matrix4x4f& rh );

        Matrix4x4f Transpose( const Matrix4x4f& m );

        Vector4f Mul( const Matrix4x4f& lh, const Vector4f& rh );

        Bounds3Df Mul( const Matrix4x4f& lh, const Bounds3Df& rh );

        zp_float32_t Dot( const Vector2f& lh, const Vector2f& rh );

        zp_float32_t Dot( const Vector3f& lh, const Vector3f& rh );

        zp_float32_t Dot( const Vector4f& lh, const Vector4f& rh );

        Vector3f Cross( const Vector3f& lh, const Vector3f& rh );

        zp_float32_t Length( const Vector2f& lh );

        zp_float32_t Length( const Vector3f& lh );

        zp_float32_t Length( const Vector4f& lh );

        zp_float32_t LengthSq( const Vector2f& lh );

        zp_float32_t LengthSq( const Vector3f& lh );

        zp_float32_t LengthSq( const Vector4f& lh );

        Vector2f Normalize( const Vector2f& lh );

        Vector3f Normalize( const Vector3f& lh );

        Vector4f Normalize( const Vector4f& lh );

        Vector2i Cmp( const Vector2f& lh, const Vector2f& rh );

        Vector3i Cmp( const Vector3f& lh, const Vector3f& rh );

        Vector4i Cmp( const Vector4f& lh, const Vector4f& rh );

        Matrix4x4f OrthoLH( const Rect2Df& orthoRect, zp_float32_t zNear, zp_float32_t zFar, zp_float32_t orthoScale = 2.f );
    }
}

//
//
//

constexpr zp::Color zp_debug_color( zp_size_t index, zp_size_t count )
{
    zp::Color c { .r = 0, .g = 0, .b = 0, .a = 1 };

    const zp_float32_t h = static_cast<zp_float32_t>( index + 1 ) / static_cast<zp_float32_t>( count );
    const zp_int32_t i = zp_floor_to_int( h * 6 );
    const zp_float32_t f = h * static_cast<zp_float32_t>( 5 - i );

    switch( index % 6 )
    {
        case 0:
            c.r = 1;
            c.g = f;
            c.b = 0;
            break;

        case 1:
            c.r = 1;
            c.g = zp_abs( 1 - f );
            c.b = 0;
            break;

        case 2:
            c.r = 0;
            c.g = 1;
            c.b = f;
            break;

        case 3:
            c.r = 0;
            c.g = zp_abs( 1 - f );
            c.b = 1;
            break;

        case 4:
            c.r = f;
            c.g = 0;
            c.b = 1;
            break;

        case 5:
            c.r = 1;
            c.g = 0;
            c.b = zp_abs( 1 - f );
            break;
    }

    return c;
}

constexpr zp::Color32 zp_debug_color32( zp_size_t index, zp_size_t count )
{
    const zp::Color color = zp_debug_color( index, count );

    zp::Color32 r {
        .r = static_cast<zp_uint8_t>( 0xFF & zp_floor_to_int( color.r * 0xFF )),
        .g = static_cast<zp_uint8_t>( 0xFF & zp_floor_to_int( color.g * 0xFF )),
        .b = static_cast<zp_uint8_t>( 0xFF & zp_floor_to_int( color.b * 0xFF )),
        .a = static_cast<zp_uint8_t>( 0xFF & zp_floor_to_int( color.a * 0xFF )),
    };
    return r;
}

#endif //ZP_MATH_H
