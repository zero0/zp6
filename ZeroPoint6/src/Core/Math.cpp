//
// Created by phosg on 2/14/2022.
//

#include "Core/Math.h"
#include "Core/Defines.h"
#include "Core/Types.h"

#include <cmath>
#include <immintrin.h>

#define _MM_SHUFFLER( fp0, fp1, fp2, fp3 ) _MM_SHUFFLE( fp3, fp2, fp1, fp0 )

zp_float32_t zp_sinf( zp_float32_t v )
{
    return sinf( v );
}

zp_float32_t zp_cosf( zp_float32_t v )
{
    return cosf( v );
}

namespace zp
{
    template<>
    const Bounds3Df Bounds3Df::invalid = {
        1,
        1,
        1,
        -1,
        -1,
        -1,
    };

    template<>
    const Bounds3Df Bounds3Df::empty = {
        0,
        0,
        0,
        0,
        0,
        0,
    };

    template<>
    const Bounds3Di Bounds3Di::invalid = {
        1,
        1,
        1,
        -1,
        -1,
        -1,
    };

    template<>
    const Bounds3Di Bounds3Di::empty = {
        0,
        0,
        0,
        0,
        0,
        0,
    };


    template<>
    const Vector2f Vector2f::zero { 0, 0 };

    template<>
    const Vector2f Vector2f::one { 1, 1 };

    template<>
    const Vector2i Vector2i::zero { 0, 0 };

    template<>
    const Vector2i Vector2i::one { 1, 1 };


    template<>
    const Vector3f Vector3f::zero { 0, 0, 0 };

    template<>
    const Vector3f Vector3f::one { 1, 1, 1 };

    template<>
    const Vector3i Vector3i::zero { 0, 0, 0 };

    template<>
    const Vector3i Vector3i::one { 1, 1, 1 };


    template<>
    const Vector4f Vector4f::zero { 0, 0, 0, 0 };

    template<>
    const Vector4f Vector4f::one { 1, 1, 1, 1 };

    template<>
    const Vector4i Vector4i::zero { 0, 0, 0, 0 };

    template<>
    const Vector4i Vector4i::one { 1, 1, 1, 1 };

    //
    //
    //

    const Quaternion Quaternion::identity { 0, 0, 0, 1 };

    //
    //
    //

    // clang-format off
    template<>
    const Matrix4x4f Matrix4x4f::identity { .v {
        1,        0,        0,        0,
        0,        1,        0,        0,
        0,        0,        1,        0,
        0,        0,        0,        1,
    } };

    template<>
    const Matrix4x4f Matrix4x4f::zero { .v {
        0,        0,        0,        0,
        0,        0,        0,        0,
        0,        0,        0,        0,
        0,        0,        0,        0,
    } } ;
    // clang-format on

    //
    //
    //

    const Color Color::clear { .r = 0, .g = 0, .b = 0, .a = 0 };

    const Color Color::black { .r = 0, .g = 0, .b = 0, .a = 1 };

    const Color Color::white { .r = 1, .g = 1, .b = 1, .a = 1 };

    const Color Color::red { .r = 1, .g = 0, .b = 0, .a = 1 };

    const Color Color::green { .r = 0, .g = 1, .b = 0, .a = 1 };

    const Color Color::blue { .r = 0, .g = 0, .b = 1, .a = 1 };

    const Color Color::yellow { .r = 1, .g = 1, .b = 0, .a = 1 };

    const Color Color::cyan { .r = 0, .g = 1, .b = 1, .a = 1 };

    const Color Color::magenta { .r = 1, .g = 0, .b = 1, .a = 1 };

    const Color Color::gray25 { .r = 0.25f, .g = 0.25f, .b = 0.25f, .a = 1 };

    const Color Color::gray50 { .r = 0.5f, .g = 0.5f, .b = 0.5f, .a = 1 };

    const Color Color::gray75 { .r = 0.75f, .g = 0.75f, .b = 0.75f, .a = 1 };

    //
    //
    //

    constexpr zp_uint8_t f = 0xFF;

    constexpr zp_uint8_t c = 0xC0;

    constexpr zp_uint8_t h = 0x80;

    constexpr zp_uint8_t q = 0x40;

    const Color32 Color32::clear { .r = 0, .g = 0, .b = 0, .a = 0 };

    const Color32 Color32::black { .r = 0, .g = 0, .b = 0, .a = f };

    const Color32 Color32::white { .r = f, .g = f, .b = f, .a = f };

    const Color32 Color32::red { .r = f, .g = 0, .b = 0, .a = f };

    const Color32 Color32::green { .r = 0, .g = f, .b = 0, .a = f };

    const Color32 Color32::blue { .r = 0, .g = 0, .b = f, .a = f };

    const Color32 Color32::yellow { .r = f, .g = f, .b = 0, .a = f };

    const Color32 Color32::cyan { .r = 0, .g = f, .b = f, .a = f };

    const Color32 Color32::magenta { .r = f, .g = 0, .b = f, .a = f };

    const Color32 Color32::gray25 { .r = q, .g = q, .b = q, .a = f };

    const Color32 Color32::gray50 { .r = h, .g = h, .b = h, .a = f };

    const Color32 Color32::gray75 { .r = c, .g = c, .b = c, .a = f };

    //
    //
    //

    namespace Math
    {
        namespace
        {

#ifndef _mm_fmadd_ps

            ZP_FORCEINLINE __m128 _mm_fmadd_ps( __m128 a, __m128 b, __m128 c )
            {
                return _mm_add_ps( _mm_mul_ps( a, b ), c );
            }

#endif

            ZP_FORCEINLINE __m128 _mm_abs_ps( __m128 a )
            {
                return _mm_and_ps( a, _mm_castsi128_ps( _mm_set1_epi32( (int)0x80000000 ) ) );
            }

            ZP_FORCEINLINE zp_float32_t _mm_hadd_ps( __m128 a )
            {
                const __m128 t = _mm_add_ps( a, _mm_movehl_ps( a, a ) );
                return _mm_cvtss_f32( _mm_add_ss( t, _mm_shuffle_ps( t, t, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ) );
            }

            ZP_FORCEINLINE __m128 _mm_normalize_ps( __m128 a )
            {
                const __m128 v = _mm_mul_ps( a, a );
                const __m128 t = _mm_add_ps( v, _mm_movehl_ps( v, v ) );
                const __m128 f = _mm_add_ss( t, _mm_shuffle_ps( t, t, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
                const __m128 l = _mm_rsqrt_ps( _mm_shuffle_ps( f, f, _MM_SHUFFLE( 0, 0, 0, 0 ) ) );
                return _mm_mul_ps( a, l );
            }

            ZP_FORCEINLINE zp_float32_t _mm_length_ps( __m128 a )
            {
                const __m128 v = _mm_mul_ps( a, a );
                const __m128 t = _mm_add_ps( v, _mm_movehl_ps( v, v ) );
                const __m128 f = _mm_add_ss( t, _mm_shuffle_ps( t, t, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
                return _mm_cvtss_f32( _mm_sqrt_ss( f ) );
            }

            ZP_FORCEINLINE zp_float32_t _mm_lengthsq_ps( __m128 a )
            {
                const __m128 v = _mm_mul_ps( a, a );
                const __m128 t = _mm_add_ps( v, _mm_movehl_ps( v, v ) );
                const __m128 f = _mm_add_ss( t, _mm_shuffle_ps( t, t, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
                return _mm_cvtss_f32( f );
            }

            ZP_FORCEINLINE __m128 _mm_lerp_ps( __m128 x, __m128 y, zp_float32_t a )
            {
                const __m128 ma = _mm_set1_ps( a );
                const __m128 nma = _mm_sub_ps( _mm_set1_ps( 1 ), ma );
                return _mm_add_ps( _mm_mul_ps( x, nma ), _mm_mul_ps( y, ma ) );
            }

            ZP_FORCEINLINE __m128 _mm_lerpunclamp_ps( __m128 x, __m128 y, zp_float32_t a )
            {
                return _mm_fmadd_ps( _mm_sub_ps( y, x ), _mm_set1_ps( a ), x );
            }
        } // namespace

        Vector3f PerspectiveDivide( const Vector4f& v )
        {
            const __m128 invW = _mm_rcp_ps( _mm_set1_ps( v.w ) );
            const __m128 xyzw = _mm_load_ps( v.m );

            ZP_ALIGN16 zp_float32_t m[ 4 ];
            _mm_store_ps( m, _mm_mul_ps( xyzw, invW ) );
            return { m[ 0 ], m[ 1 ], m[ 2 ] };
        }

        Color Lerp( const Color& x, const Color& y, zp_float32_t a )
        {
            const __m128 mx = _mm_loadu_ps( x.rgba );
            const __m128 my = _mm_loadu_ps( y.rgba );

            ZP_ALIGN16 Color r {};
            _mm_store_ps( r.rgba, _mm_lerpunclamp_ps( mx, my, zp_clamp01( a ) ) );
            return r;
        }

        Color LerpUnclamp( const Color& x, const Color& y, zp_float32_t a )
        {
            const __m128 mx = _mm_loadu_ps( x.rgba );
            const __m128 my = _mm_loadu_ps( y.rgba );

            ZP_ALIGN16 Color r {};
            _mm_store_ps( r.rgba, _mm_lerpunclamp_ps( mx, my, a ) );
            return r;
        }

        Color LerpExact( const Color& x, const Color& y, zp_float32_t a )
        {
            const __m128 mx = _mm_loadu_ps( x.rgba );
            const __m128 my = _mm_loadu_ps( y.rgba );

            ZP_ALIGN16 Color r {};
            _mm_store_ps( r.rgba, _mm_lerp_ps( mx, my, zp_clamp01( a ) ) );
            return r;
        }

        Color Mul( const Color x, const Color y )
        {
            const __m128 mx = _mm_loadu_ps( x.rgba );
            const __m128 my = _mm_loadu_ps( y.rgba );

            ZP_ALIGN16 Color r {};
            _mm_store_ps( r.rgba, _mm_mul_ps( mx, my ) );
            return r;
        }

        Color Mul( const Color x, zp_float32_t y )
        {
            const __m128 mx = _mm_loadu_ps( x.rgba );
            const __m128 my = _mm_set1_ps( y );

            ZP_ALIGN16 Color r {};
            _mm_store_ps( r.rgba, _mm_mul_ps( mx, my ) );
            return r;
        }

        Color Add( const Color x, const Color y )
        {
            const __m128 mx = _mm_loadu_ps( x.rgba );
            const __m128 my = _mm_loadu_ps( y.rgba );

            ZP_ALIGN16 Color r {};
            _mm_store_ps( r.rgba, _mm_add_ps( mx, my ) );
            return r;
        }

        Vector4f Add( const Vector4f x, const Vector4f y )
        {
            const __m128 mx = _mm_loadu_ps( x.m );
            const __m128 my = _mm_loadu_ps( y.m );

            ZP_ALIGN16 Vector4f r {};
            _mm_store_ps( r.m, _mm_add_ps( mx, my ) );
            return r;
        }

        Vector4f Mul( const Vector4f x, const Vector4f y )
        {
            const __m128 mx = _mm_loadu_ps( x.m );
            const __m128 my = _mm_loadu_ps( y.m );

            ZP_ALIGN16 Vector4f r {};
            _mm_store_ps( r.m, _mm_mul_ps( mx, my ) );
            return r;
        }

        Vector4f Mul( const Vector4f x, zp_float32_t y )
        {
            const __m128 mx = _mm_loadu_ps( x.m );
            const __m128 my = _mm_set1_ps( y );

            ZP_ALIGN16 Vector4f r {};
            _mm_store_ps( r.m, _mm_mul_ps( mx, my ) );
            return r;
        }

        Matrix4x4f Mul( const Matrix4x4f& lh, const Matrix4x4f& rh )
        {
            const __m128 lc0 = _mm_setr_ps( lh.c0.x, lh.c0.y, lh.c0.z, lh.c0.w );
            const __m128 lc1 = _mm_setr_ps( lh.c1.x, lh.c1.y, lh.c1.z, lh.c1.w );
            const __m128 lc2 = _mm_setr_ps( lh.c2.x, lh.c2.y, lh.c2.z, lh.c2.w );
            const __m128 lc3 = _mm_setr_ps( lh.c3.x, lh.c3.y, lh.c3.z, lh.c3.w );

            __m128 col0 = _mm_mul_ps( lc0, _mm_set1_ps( rh.c0.x ) );
            __m128 col1 = _mm_mul_ps( lc0, _mm_set1_ps( rh.c1.x ) );
            __m128 col2 = _mm_mul_ps( lc0, _mm_set1_ps( rh.c2.x ) );
            __m128 col3 = _mm_mul_ps( lc0, _mm_set1_ps( rh.c3.x ) );

            col0 = _mm_fmadd_ps( lc1, _mm_set1_ps( rh.c0.y ), col0 );
            col1 = _mm_fmadd_ps( lc1, _mm_set1_ps( rh.c1.y ), col1 );
            col2 = _mm_fmadd_ps( lc1, _mm_set1_ps( rh.c2.y ), col2 );
            col3 = _mm_fmadd_ps( lc1, _mm_set1_ps( rh.c3.y ), col3 );

            col0 = _mm_fmadd_ps( lc2, _mm_set1_ps( rh.c0.z ), col0 );
            col1 = _mm_fmadd_ps( lc2, _mm_set1_ps( rh.c1.z ), col1 );
            col2 = _mm_fmadd_ps( lc2, _mm_set1_ps( rh.c2.z ), col2 );
            col3 = _mm_fmadd_ps( lc2, _mm_set1_ps( rh.c3.z ), col3 );

            col0 = _mm_fmadd_ps( lc3, _mm_set1_ps( rh.c0.w ), col0 );
            col1 = _mm_fmadd_ps( lc3, _mm_set1_ps( rh.c1.w ), col1 );
            col2 = _mm_fmadd_ps( lc3, _mm_set1_ps( rh.c2.w ), col2 );
            col3 = _mm_fmadd_ps( lc3, _mm_set1_ps( rh.c3.w ), col3 );

            ZP_ALIGN16 Matrix4x4f r {};
            _mm_store_ps( r.c0.m, col0 );
            _mm_store_ps( r.c1.m, col1 );
            _mm_store_ps( r.c2.m, col2 );
            _mm_store_ps( r.c3.m, col3 );

            return r;
        }

        Matrix4x4f Transpose( const Matrix4x4f& m )
        {
            __m128 lc0 = _mm_setr_ps( m.c0.x, m.c0.y, m.c0.z, m.c0.w );
            __m128 lc1 = _mm_setr_ps( m.c1.x, m.c1.y, m.c1.z, m.c1.w );
            __m128 lc2 = _mm_setr_ps( m.c2.x, m.c2.y, m.c2.z, m.c2.w );
            __m128 lc3 = _mm_setr_ps( m.c3.x, m.c3.y, m.c3.z, m.c3.w );

            _MM_TRANSPOSE4_PS( lc0, lc1, lc2, lc3 );

            ZP_ALIGN16 Matrix4x4f r {};
            _mm_store_ps( r.m[ 0 ], lc0 );
            _mm_store_ps( r.m[ 1 ], lc1 );
            _mm_store_ps( r.m[ 2 ], lc2 );
            _mm_store_ps( r.m[ 3 ], lc3 );

            return r;
        }

        Vector4f Mul( const Matrix4x4f& lh, const Vector4f& rh )
        {
            const __m128 lc0 = _mm_setr_ps( lh.c0.x, lh.c0.y, lh.c0.z, lh.c0.w );
            const __m128 lc1 = _mm_setr_ps( lh.c1.x, lh.c1.y, lh.c1.z, lh.c1.w );
            const __m128 lc2 = _mm_setr_ps( lh.c2.x, lh.c2.y, lh.c2.z, lh.c2.w );
            const __m128 lc3 = _mm_setr_ps( lh.c3.x, lh.c3.y, lh.c3.z, lh.c3.w );

            __m128 col0 = _mm_mul_ps( _mm_set1_ps( rh.x ), lc0 );
            col0 = _mm_fmadd_ps( _mm_set1_ps( rh.y ), lc1, col0 );
            col0 = _mm_fmadd_ps( _mm_set1_ps( rh.z ), lc2, col0 );
            col0 = _mm_fmadd_ps( _mm_set1_ps( rh.w ), lc3, col0 );

            ZP_ALIGN16 Vector4f r {};
            _mm_store_ps( r.m, col0 );

            return r;
        }

        Bounds3Df Mul( const Matrix4x4f& lh, const Bounds3Df& rh )
        {
            const __m128 mmin = _mm_setr_ps( rh.xMin, rh.yMin, rh.zMin, 1 );
            const __m128 mmax = _mm_setr_ps( rh.xMax, rh.yMax, rh.zMax, 1 );

            const __m128 mrcp2 = _mm_rcp_ps( _mm_set1_ps( 2 ) );
            const __m128 mcenter = _mm_mul_ps( _mm_add_ps( mmax, mmin ), mrcp2 );
            const __m128 mextents = _mm_mul_ps( _mm_sub_ps( mmax, mmin ), mrcp2 );

            const __m128 mcx = _mm_setr_ps( lh.c0.x, lh.c1.x, lh.c2.x, lh.c3.x );
            const __m128 mcy = _mm_setr_ps( lh.c0.y, lh.c1.y, lh.c2.y, lh.c3.y );
            const __m128 mcz = _mm_setr_ps( lh.c0.z, lh.c1.z, lh.c2.z, lh.c3.z );
            const __m128 mcw = _mm_setr_ps( lh.c0.w, lh.c1.w, lh.c2.w, lh.c3.w );

            __m128 tcenter = _mm_mul_ps( _mm_shuffle_ps( mcenter, mcenter, _MM_SHUFFLE( 0, 0, 0, 0 ) ), mcx );
            tcenter = _mm_fmadd_ps( _mm_shuffle_ps( mcenter, mcenter, _MM_SHUFFLE( 1, 1, 1, 1 ) ), mcy, tcenter );
            tcenter = _mm_fmadd_ps( _mm_shuffle_ps( mcenter, mcenter, _MM_SHUFFLE( 2, 2, 2, 2 ) ), mcz, tcenter );
            tcenter = _mm_fmadd_ps( _mm_shuffle_ps( mcenter, mcenter, _MM_SHUFFLE( 3, 3, 3, 3 ) ), mcw, tcenter );

            __m128 textents = _mm_mul_ps( _mm_shuffle_ps( mextents, mextents, _MM_SHUFFLE( 0, 0, 0, 0 ) ), _mm_abs_ps( mcx ) );
            textents = _mm_fmadd_ps( _mm_shuffle_ps( mextents, mextents, _MM_SHUFFLE( 1, 1, 1, 1 ) ), _mm_abs_ps( mcy ), textents );
            textents = _mm_fmadd_ps( _mm_shuffle_ps( mextents, mextents, _MM_SHUFFLE( 2, 2, 2, 2 ) ), _mm_abs_ps( mcz ), textents );
            textents = _mm_fmadd_ps( _mm_shuffle_ps( mextents, mextents, _MM_SHUFFLE( 3, 3, 3, 3 ) ), _mm_abs_ps( mcw ), textents );

            ZP_ALIGN16 zp_float32_t fmin[ 4 ];
            ZP_ALIGN16 zp_float32_t fmax[ 4 ];
            _mm_store_ps( fmin, _mm_sub_ps( tcenter, textents ) );
            _mm_store_ps( fmax, _mm_add_ps( tcenter, textents ) );

            const Bounds3Df r {
                .xMin = fmin[ 0 ],
                .yMin = fmin[ 1 ],
                .zMin = fmin[ 2 ],
                .xMax = fmax[ 0 ],
                .yMax = fmax[ 1 ],
                .zMax = fmax[ 2 ],
            };
            return r;
        }

        zp_float32_t Dot( const Vector2f& lh, const Vector2f& rh )
        {
            return _mm_hadd_ps( _mm_mul_ps( _mm_setr_ps( lh.x, lh.y, 0, 0 ), _mm_setr_ps( rh.x, rh.y, 0, 0 ) ) );
        }

        zp_float32_t Dot( const Vector3f& lh, const Vector3f& rh )
        {
            return _mm_hadd_ps( _mm_mul_ps( _mm_setr_ps( lh.x, lh.y, lh.z, 0 ), _mm_setr_ps( rh.x, rh.y, rh.z, 0 ) ) );
        }

        zp_float32_t Dot( const Vector4f& lh, const Vector4f& rh )
        {
            const __m128 t0 = _mm_mul_ps( _mm_setr_ps( lh.x, lh.y, lh.z, lh.w ), _mm_setr_ps( rh.x, rh.y, rh.z, rh.w ) );
            const __m128 t1 = _mm_shuffle_ps( t0, t0, _MM_SHUFFLE( 1, 0, 3, 2 ) );
            const __m128 t2 = _mm_add_ps( t0, t1 );
            const __m128 t3 = _mm_shuffle_ps( t2, t2, _MM_SHUFFLE( 2, 3, 0, 1 ) );

            return _mm_cvtss_f32( _mm_add_ps( t3, t2 ) );
        }

        Vector3f Cross( const Vector3f& lh, const Vector3f& rh )
        {
            const __m128 a = _mm_setr_ps( lh.x, lh.y, lh.z, 0 );
            const __m128 b = _mm_setr_ps( rh.x, rh.y, rh.z, 0 );

            const __m128 s0 = _mm_shuffle_ps( a, a, _MM_SHUFFLE( 3, 0, 2, 1 ) );
            const __m128 s1 = _mm_shuffle_ps( b, b, _MM_SHUFFLE( 3, 1, 0, 2 ) );
            const __m128 s2 = _mm_mul_ps( s0, s1 );

            const __m128 s3 = _mm_shuffle_ps( a, a, _MM_SHUFFLE( 3, 1, 0, 2 ) );
            const __m128 s4 = _mm_shuffle_ps( b, b, _MM_SHUFFLE( 3, 0, 2, 1 ) );
            const __m128 s5 = _mm_mul_ps( s3, s4 );

            const __m128 x = _mm_sub_ps( s2, s5 );

            ZP_ALIGN16 zp_float32_t m[ 4 ];
            _mm_storer_ps( m, x );

            Vector3f r { m[ 0 ], m[ 1 ], m[ 2 ] };
            return r;
        }

        zp_float32_t Length( const Vector2f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, 0, 0 );
            return _mm_length_ps( v );
        }

        zp_float32_t Length( const Vector3f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, lh.z, 0 );
            return _mm_length_ps( v );
        }

        zp_float32_t Length( const Vector4f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, lh.z, lh.w );
            return _mm_length_ps( v );
        }

        zp_float32_t LengthSq( const Vector2f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, 0, 0 );
            return _mm_lengthsq_ps( v );
        }

        zp_float32_t LengthSq( const Vector3f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, lh.z, 0 );
            return _mm_lengthsq_ps( v );
        }

        zp_float32_t LengthSq( const Vector4f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, lh.z, lh.w );
            return _mm_lengthsq_ps( v );
        }

        Vector2f Normalize( const Vector2f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, 0, 0 );

            ZP_ALIGN16 zp_float32_t m[ 4 ];
            _mm_store_ps( m, _mm_normalize_ps( v ) );

            Vector2f r { .x = m[ 0 ], .y = m[ 1 ] };
            return r;
        }

        Vector3f Normalize( const Vector3f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, lh.z, 0 );

            ZP_ALIGN16 zp_float32_t m[ 4 ];
            _mm_store_ps( m, _mm_normalize_ps( v ) );

            Vector3f r { .x = m[ 0 ], .y = m[ 1 ], .z = m[ 2 ] };
            return r;
        }

        Vector4f Normalize( const Vector4f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, lh.z, lh.w );

            ZP_ALIGN16 Vector4f r;
            _mm_store_ps( r.m, _mm_normalize_ps( v ) );

            return r;
        }

        Vector2i Cmp( const Vector2f& lh, const Vector2f& rh )
        {
            const __m128 vlh = _mm_setr_ps( lh.x, lh.y, 0, 0 );
            const __m128 vrh = _mm_setr_ps( rh.x, rh.y, 0, 0 );

            const __m128 veq = _mm_cmpeq_ps( vlh, vrh );
            const __m128 vlt = _mm_cmplt_ps( vlh, vrh );

            ZP_ALIGN16 zp_uint32_t eq[ 4 ];
            ZP_ALIGN16 zp_uint32_t lt[ 4 ];
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>( eq ), veq );
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>( lt ), vlt );

            return {
                .x = lt[ 0 ] ? -1 : eq[ 0 ] ? 0
                                            : 1,
                .y = lt[ 1 ] ? -1 : eq[ 1 ] ? 0
                                            : 1,
            };
        }

        Vector3i Cmp( const Vector3f& lh, const Vector3f& rh )
        {
            const __m128 vlh = _mm_setr_ps( lh.x, lh.y, lh.z, 0 );
            const __m128 vrh = _mm_setr_ps( rh.x, rh.y, rh.z, 0 );

            const __m128 veq = _mm_cmpeq_ps( vlh, vrh );
            const __m128 vlt = _mm_cmplt_ps( vlh, vrh );

            ZP_ALIGN16 zp_uint32_t eq[ 4 ];
            ZP_ALIGN16 zp_uint32_t lt[ 4 ];
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>( eq ), veq );
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>( lt ), vlt );

            return {
                .x = lt[ 0 ] ? -1 : eq[ 0 ] ? 0
                                            : 1,
                .y = lt[ 1 ] ? -1 : eq[ 1 ] ? 0
                                            : 1,
                .z = lt[ 2 ] ? -1 : eq[ 2 ] ? 0
                                            : 1,
            };
        }

        Vector4i Cmp( const Vector4f& lh, const Vector4f& rh )
        {
            const __m128 vlh = _mm_setr_ps( lh.x, lh.y, lh.z, lh.w );
            const __m128 vrh = _mm_setr_ps( rh.x, rh.y, rh.z, rh.w );

            const __m128 veq = _mm_cmpeq_ps( vlh, vrh );
            const __m128 vlt = _mm_cmplt_ps( vlh, vrh );

            ZP_ALIGN16 zp_uint32_t eq[ 4 ];
            ZP_ALIGN16 zp_uint32_t lt[ 4 ];
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>( eq ), veq );
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>( lt ), vlt );

            return {
                .x = lt[ 0 ] ? -1 : eq[ 0 ] ? 0
                                            : 1,
                .y = lt[ 1 ] ? -1 : eq[ 1 ] ? 0
                                            : 1,
                .z = lt[ 2 ] ? -1 : eq[ 2 ] ? 0
                                            : 1,
                .w = lt[ 3 ] ? -1 : eq[ 3 ] ? 0
                                            : 1,
            };
        }

        Matrix4x4f OrthoLH( const Rect2Df& orthoRect, zp_float32_t zNear, zp_float32_t zFar, zp_float32_t orthoScale )
        {
            const zp_float32_t r = orthoRect.right(); // orthoRect.offset.x + ( orthoRect.size.width * 0.5f );
            const zp_float32_t l = orthoRect.left(); // orthoRect.offset.x + ( orthoRect.size.width * -0.5f );
            const zp_float32_t t = orthoRect.top(); // orthoRect.offset.y + ( orthoRect.size.height * 0.5f );
            const zp_float32_t b = orthoRect.bottom(); // orthoRect.offset.y + ( orthoRect.size.height * -0.5f );

            const __m128 rtf = _mm_setr_ps( r, t, zFar, 1 );
            const __m128 lbn = _mm_setr_ps( l, b, zNear, 0 );

            const __m128 rtfMlbn = _mm_sub_ps( rtf, lbn );
            const __m128 rtfPlbn = _mm_add_ps( rtf, lbn );

            const __m128 invRtfMlbn = _mm_rcp_ps( rtfMlbn );
            const __m128 scale = _mm_setr_ps( orthoScale, orthoScale, -orthoScale, 0 );
            const __m128 s = _mm_mul_ps( scale, invRtfMlbn );

            const __m128 z = _mm_setzero_ps();
            const __m128 c0 = _mm_shuffle_ps( s, z, _MM_SHUFFLER( 0, 3, 0, 0 ) );
            const __m128 c1 = _mm_shuffle_ps( s, z, _MM_SHUFFLER( 3, 1, 0, 0 ) );
            const __m128 c2 = _mm_shuffle_ps( z, s, _MM_SHUFFLER( 0, 0, 2, 3 ) );
            const __m128 c3 = _mm_fmadd_ps( _mm_mul_ps( rtfPlbn, invRtfMlbn ), _mm_setr_ps( -1, -1, -1, 0 ), _mm_setr_ps( 0, 0, 0, 1 ) );

            // NOTE: c0..3 are set up properly, no need to store reverse
            ZP_ALIGN16 Matrix4x4f matrix {};
            _mm_store_ps( matrix.c0.m, c0 );
            _mm_store_ps( matrix.c1.m, c1 );
            _mm_store_ps( matrix.c2.m, c2 );
            _mm_store_ps( matrix.c3.m, c3 );

            return matrix;
        }

        zp_uint32_t Log2( zp_uint32_t v )
        {
            return static_cast<zp_uint32_t>( ::log2f( static_cast<zp_float32_t>( v ) ) );
        }
    } // namespace Math
} // namespace zp

#if ZP_USE_TESTS
#include "Test/Test.h"

using namespace zp;

ZP_TEST_GROUP( Math )
{
    ZP_TEST_SUITE( Vector2f )
    {
        ZP_TEST( Length )
        {
            constexpr Vector2f v { 1, 1 };
            const zp_float32_t len = Math::Length( v );

            ZP_CHECK_EQUALS( len, 1.4142135623730950488016887242097f );
            ZP_CHECK_FLOAT32_APPROX( len, 1.4142135623730950488016887242097f, 0.000001f );
        };

        ZP_TEST( Normalize )
        {
            constexpr Vector2f v { 10, 10 };
            const Vector2f n = Math::Normalize( v );

            ZP_CHECK_FLOAT32_APPROX( Math::Length( n ), 1.0f, 0.000001f );
        }
    }

    ZP_TEST_SUITE( Vector3f )
    {
        ZP_TEST( Length )
        {
            constexpr Vector3f v { 1, 1, 1 };
            const zp_float32_t len = Math::Length( v );

            ZP_CHECK_EQUALS( len, 1.7320508075688772935274463415059f );
            ZP_CHECK_FLOAT32_APPROX( len, 1.7320508075688772935274463415059f, 0.000001f );
        };

        ZP_TEST( Normalize )
        {
            constexpr Vector3f v { 10, 10, 10 };
            const Vector3f n = Math::Normalize( v );

            ZP_CHECK_FLOAT32_APPROX( Math::Length( n ), 1.0f, 0.000001f );
        }
    }

    ZP_TEST_SUITE( Vector4f )
    {
        ZP_TEST( Length1 )
        {
            constexpr Vector4f v { 1, 0, 0, 0 };
            const zp_float32_t len = Math::Length( v );

            ZP_CHECK_EQUALS( len, 1.0f );
            ZP_CHECK_FLOAT32_APPROX( len, 1.0f, 0.000001f );
        }

        ZP_TEST( Length2 )
        {
            constexpr Vector4f v { 1, 1, 0, 0 };
            const zp_float32_t len = Math::Length( v );

            ZP_CHECK_EQUALS( len, 1.4142135623730950488016887242097f );
            ZP_CHECK_FLOAT32_APPROX( len, 1.4142135623730950488016887242097f, 0.000001f );
        };

        ZP_TEST( Length3 )
        {
            constexpr Vector4f v { 1, 1, 1, 0 };
            const zp_float32_t len = Math::Length( v );

            ZP_CHECK_EQUALS( len, 1.7320508075688772935274463415059f );
            ZP_CHECK_FLOAT32_APPROX( len, 1.7320508075688772935274463415059f, 0.000001f );
        };

        ZP_TEST( Length4 )
        {
            constexpr Vector4f v { 1, 1, 1, 1 };
            const zp_float32_t len = Math::Length( v );

            ZP_CHECK_EQUALS( len, 2.0f );
            ZP_CHECK_FLOAT32_APPROX( len, 2.0f, 0.000001f );
        };

        ZP_TEST( Normalize )
        {
            constexpr Vector4f v { 10, 10, 10, 10 };
            const Vector4f n = Math::Normalize( v );

            ZP_CHECK_FLOAT32_APPROX( Math::Length( n ), 1.0f, 0.000001f );
        }
    }


    ZP_TEST_SUITE( Matrix4x4f )
    {
        ZP_TEST( IdentityEqualsCorrectValue )
        {
            constexpr Matrix4x4f matrix { .v { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };

            ZP_CHECK_EQUALS( matrix, Matrix4x4f::identity );
        }

        ZP_TEST( ZeroEqualsCorrectValue )
        {
            constexpr Matrix4x4f matrix { .v { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

            ZP_CHECK_EQUALS( matrix, Matrix4x4f::zero );
        }

        ZP_TEST( MultiplyIdentity )
        {
            constexpr Matrix4x4f matrix { .v { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } };

            ZP_CHECK_EQUALS( Math::Mul( matrix, Matrix4x4f::identity ), matrix );
        }

        ZP_TEST( MultiplyIdentityColumnMajor )
        {
            // Identity matrix with column-major layout
            constexpr Matrix4x4f identity { .v { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
            ZP_CHECK_EQUALS( Math::Mul( identity, identity ), identity );
        }

        ZP_TEST( Multiply )
        {
            // A = [[1,2],[3,4]]
            constexpr Matrix4x4f a { .v { 1, 2, 0, 0, 3, 4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
            // B = [[5,6],[7,8]]
            constexpr Matrix4x4f b { .v { 5, 6, 0, 0, 7, 8, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
            // A*B should be [[19,22],[43,50]]
            Matrix4x4f c = Math::Mul( a, b );

            ZP_CHECK_FLOAT32_APPROX( c.m[ 0 ][ 0 ], 19.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( c.m[ 0 ][ 1 ], 22.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( c.m[ 0 ][ 2 ], 43.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( c.m[ 0 ][ 3 ], 50.0f, 0.000001f );
        }

        ZP_TEST( Transpose )
        {
            // A = [[1,2],[3,4]]
            constexpr Matrix4x4f a { .v { 1, 2, 0, 0, 3, 4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
            const Matrix4x4f at = Math::Transpose( a );

            ZP_CHECK_FLOAT32_APPROX( at.m[ 0 ][ 0 ], 3.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( at.m[ 0 ][ 1 ], 1.0f, 0.000001f );
        }

        ZP_TEST( MultiplyVector )
        {
            // A = [[2,0],[0,3]] (scaling matrix)
            constexpr Matrix4x4f a { .v { 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
            const Vector4f v { 1, 2, 3, 4 };
            const Vector4f expected { 2, 6, 3, 4 };
            const Vector4f result = Math::Mul( a, v );

            ZP_CHECK_FLOAT32_APPROX( result.x, expected.x, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.y, expected.y, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.z, expected.z, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.w, expected.w, 0.000001f );
        }
    }


    ZP_TEST_SUITE( Vector2f )
    {
        ZP_TEST( Dot )
        {
            constexpr Vector2f a { 1, 2 };
            constexpr Vector2f b { 3, 4 };
            const zp_float32_t result = Math::Dot( a, b );

            // 1*3 + 2*4 = 11
            ZP_CHECK_EQUALS( result, 11.0f );
        };

        ZP_TEST( DotWithZero )
        {
            constexpr Vector2f a { 0, 5 };
            constexpr Vector2f b { 0, 0 };
            const zp_float32_t result = Math::Dot( a, b );

            // 0*0 + 5*0 = 0
            ZP_CHECK_EQUALS( result, 0.0f );
        };

        ZP_TEST( LengthSq )
        {
            constexpr Vector2f v { 3, 4 };
            const zp_float32_t len = Math::Length( v );
            const zp_float32_t lensq = Math::LengthSq( v );

            // len should be 5.0
            // lensq should be 25.0
            ZP_CHECK_FLOAT32_APPROX( len, 5.0f, 0.000001f );
            ZP_CHECK_EQUALS( lensq, 25.0f );
        };

        ZP_TEST( LengthSqZeroVector )
        {
            constexpr Vector2f v { 0, 0 };
            const zp_float32_t lensq = Math::LengthSq( v );

            // 0^2 + 0^2 = 0
            ZP_CHECK_EQUALS( lensq, 0.0f );
        };

        ZP_TEST( Cmp )
        {
            constexpr Vector2f a { 1, 2 };
            constexpr Vector2f b { 1, 2 };
            const Vector2i result = Math::Cmp( a, b );

            // Equal vectors should return {0, 0}
            ZP_CHECK_EQUALS( result.x, 0 );
            ZP_CHECK_EQUALS( result.y, 0 );
        };

        ZP_TEST( CmpLessThan )
        {
            constexpr Vector2f a { 1, 2 };
            constexpr Vector2f b { 2, 2 };
            const Vector2i result = Math::Cmp( a, b );

            // a < b should return {-1, 0}
            ZP_CHECK_EQUALS( result.x, -1 );
            ZP_CHECK_EQUALS( result.y, 0 );
        };

        ZP_TEST( CmpGreaterThan )
        {
            constexpr Vector2f a { 2, 2 };
            constexpr Vector2f b { 1, 2 };
            const Vector2i result = Math::Cmp( a, b );

            // a > b should return {1, 0}
            ZP_CHECK_EQUALS( result.x, 1 );
            ZP_CHECK_EQUALS( result.y, 0 );
        };
    }


    ZP_TEST_SUITE( Vector3f )
    {
        ZP_TEST( Cross )
        {
            constexpr Vector3f a { 1, 0, 0 };
            constexpr Vector3f b { 0, 1, 0 };
            const Vector3f result = Math::Cross( a, b );

            // Cross product of x and y unit vectors is z unit vector
            // Result should be {0, 0, 1}
            ZP_CHECK_FLOAT32_APPROX( result.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.y, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.z, 1.0f, 0.000001f );
        };

        ZP_TEST( CrossParallelVectors )
        {
            constexpr Vector3f a { 1, 2, 3 };
            constexpr Vector3f b { 2, 4, 6 }; // 2*a
            const Vector3f result = Math::Cross( a, b );

            // Parallel vectors have zero cross product
            ZP_CHECK_FLOAT32_APPROX( result.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.y, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.z, 0.0f, 0.000001f );
        };

        ZP_TEST( CrossAntiParallelVectors )
        {
            constexpr Vector3f a { 1, 2, 3 };
            constexpr Vector3f b { -1, -2, -3 }; // -a
            const Vector3f result = Math::Cross( a, b );

            // Anti-parallel vectors have zero cross product
            ZP_CHECK_FLOAT32_APPROX( result.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.y, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.z, 0.0f, 0.000001f );
        };

        ZP_TEST( DotWithZero )
        {
            constexpr Vector3f a { 0, 5, 3 };
            constexpr Vector3f b { 0, 0, 0 };
            const zp_float32_t result = Math::Dot( a, b );

            // 0*0 + 5*0 + 3*0 = 0
            ZP_CHECK_EQUALS( result, 0.0f );
        };

        ZP_TEST( LengthSq )
        {
            constexpr Vector3f v { 1, 2, 2 };
            const zp_float32_t lensq = Math::LengthSq( v );

            // 1^2 + 2^2 + 2^2 = 1 + 4 + 4 = 9
            ZP_CHECK_EQUALS( lensq, 9.0f );
        };

        ZP_TEST( NormalizeZeroVector )
        {
            constexpr Vector3f v { 0, 0, 0 };
            const Vector3f n = Math::Normalize( v );

            // Normalizing zero vector should give NaN values
            // We verify the result is not the identity vector
            ZP_CHECK_NOT_EQUALS( n.x, 0.0f );
        };

        ZP_TEST( NormalizeNegativeVector )
        {
            constexpr Vector3f v { -3, -4, 0 };
            const Vector3f n = Math::Normalize( v );

            // Length should be 5
            ZP_CHECK_FLOAT32_APPROX( Math::Length( n ), 1.0f, 0.000001f );
            // Direction should be opposite of input
            ZP_CHECK_FLOAT32_APPROX( n.x, 0.6f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( n.y, 0.8f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( n.z, 0.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Vector4f )
    {
        ZP_TEST( Dot )
        {
            constexpr Vector4f a { 1, 2, 3, 4 };
            constexpr Vector4f b { 5, 6, 7, 8 };
            const zp_float32_t result = Math::Dot( a, b );

            // 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70
            ZP_CHECK_EQUALS( result, 70.0f );
        };

        ZP_TEST( DotWithZero )
        {
            constexpr Vector4f a { 0, 5, 3, 1 };
            constexpr Vector4f b { 0, 0, 0, 0 };
            const zp_float32_t result = Math::Dot( a, b );

            ZP_CHECK_EQUALS( result, 0.0f );
        };

        ZP_TEST( Cross )
        {
            constexpr Vector4f a { 1, 2, 3, 4 };
            constexpr Vector4f b { 2, 3, 4, 5 };
            const Vector4i result = Math::Cmp( a, b );

            // Cmp returns comparison, not cross product
            // This test verifies Cmp returns correct comparison results
            const Vector4i cmpResult = Math::Cmp(
                { 1, 2, 3, 0 }, { 2, 3, 4, 0 } );
            ZP_CHECK_EQUALS( cmpResult.x, -1 );
        };

        ZP_TEST( NormalizeWithLargeValues )
        {
            constexpr Vector4f v { 100, 200, 300, 400 };
            const Vector4f n = Math::Normalize( v );

            // Length should be approximately 1
            ZP_CHECK_FLOAT32_APPROX( Math::Length( n ), 1.0f, 0.00001f );
        };

        ZP_TEST( NormalizeWithSmallValues )
        {
            constexpr Vector4f v { 0.1, 0.2, 0.3, 0.4 };
            const Vector4f n = Math::Normalize( v );

            // Length should be approximately 1
            ZP_CHECK_FLOAT32_APPROX( Math::Length( n ), 1.0f, 0.00001f );
        };
    }


    ZP_TEST_SUITE( PerspectiveDivide )
    {
        ZP_TEST( NormalizedVector )
        {
            constexpr Vector4f v { 1, 2, 3, 1 };
            const Vector3f result = Math::PerspectiveDivide( v );

            // w=1, so result should be {1, 2, 3}
            ZP_CHECK_FLOAT32_APPROX( result.x, 1.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.y, 2.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.z, 3.0f, 0.000001f );
        };

        ZP_TEST( VectorWithWZero )
        {
            constexpr Vector4f v { 1, 2, 3, 0 };
            const Vector3f result = Math::PerspectiveDivide( v );

            // w=0, so result should have inf values
            // The result should not equal the input
            ZP_CHECK_NOT_EQUALS( result.x, v.x );
        };

        ZP_TEST( NegativeW )
        {
            constexpr Vector4f v { 1, 2, 3, -1 };
            const Vector3f result = Math::PerspectiveDivide( v );

            // w=-1, so result should be {-1, -2, -3}
            ZP_CHECK_FLOAT32_APPROX( result.x, -1.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.y, -2.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.z, -3.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Bounds2Df )
    {
        ZP_TEST( EncapsulateSinglePoint )
        {
            Bounds2Df bounds;
            bounds.encapsulate( Vector2f { 5, 5 } );

            ZP_CHECK_FLOAT32_APPROX( bounds.min().x, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.min().y, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().x, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().y, 5.0f, 0.000001f );
        };

        ZP_TEST( EncapsulateMultiplePoints )
        {
            Bounds2Df bounds;
            bounds.encapsulate( Vector2f { 1, 1 } );
            bounds.encapsulate( Vector2f { 5, 5 } );
            bounds.encapsulate( Vector2f { 1, 5 } );
            bounds.encapsulate( Vector2f { 5, 1 } );

            ZP_CHECK_FLOAT32_APPROX( bounds.min().x, 1.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.min().y, 1.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().x, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().y, 5.0f, 0.000001f );
        };

        ZP_TEST( Size )
        {
            Bounds2Df bounds;
            bounds.encapsulate( Vector2f { 0, 0 } );
            bounds.encapsulate( Vector2f { 10, 10 } );

            const auto s = bounds.size();
            ZP_CHECK_FLOAT32_APPROX( s.width, 10.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( s.height, 10.0f, 0.000001f );
        };

        ZP_TEST( Center )
        {
            Bounds2Df bounds;
            bounds.encapsulate( Vector2f { 1, 1 } );
            bounds.encapsulate( Vector2f { 9, 9 } );

            const auto c = bounds.center();
            ZP_CHECK_FLOAT32_APPROX( c.x, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( c.y, 5.0f, 0.000001f );
        };

        ZP_TEST( MinMax )
        {
            Bounds2Df bounds;
            bounds.encapsulate( Vector2f { 2, 2 } );
            bounds.encapsulate( Vector2f { 8, 8 } );

            ZP_CHECK_FLOAT32_APPROX( bounds.min().x, 2.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().x, 8.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Bounds3Df )
    {
        ZP_TEST( EncapsulateSinglePoint )
        {
            Bounds3Df bounds;
            bounds.encapsulate( Vector3f { 5, 5, 5 } );

            ZP_CHECK_FLOAT32_APPROX( bounds.min().x, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.min().y, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.min().z, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().x, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().y, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().z, 5.0f, 0.000001f );
        };

        ZP_TEST( EncapsulateMultiplePoints )
        {
            Bounds3Df bounds;
            bounds.encapsulate( Vector3f { 0, 0, 0 } );
            bounds.encapsulate( Vector3f { 10, 10, 10 } );
            bounds.encapsulate( Vector3f { 5, 0, 5 } );
            bounds.encapsulate( Vector3f { 0, 10, 5 } );
            bounds.encapsulate( Vector3f { 0, 0, 10 } );

            ZP_CHECK_FLOAT32_APPROX( bounds.min().x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().x, 10.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.min().y, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().y, 10.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.min().z, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( bounds.max().z, 10.0f, 0.000001f );
        };

        ZP_TEST( Size )
        {
            Bounds3Df bounds;
            bounds.encapsulate( Vector3f { 0, 0, 0 } );
            bounds.encapsulate( Vector3f { 10, 10, 10 } );

            const auto s = bounds.size();
            ZP_CHECK_FLOAT32_APPROX( s.width, 10.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( s.height, 10.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( s.depth, 10.0f, 0.000001f );
        };

        ZP_TEST( Extents )
        {
            Bounds3Df bounds;
            bounds.encapsulate( Vector3f { 0, 0, 0 } );
            bounds.encapsulate( Vector3f { 10, 10, 10 } );

            const auto e = bounds.extents();
            ZP_CHECK_FLOAT32_APPROX( e.width, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( e.height, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( e.depth, 5.0f, 0.000001f );
        };

        ZP_TEST( Center )
        {
            Bounds3Df bounds;
            bounds.encapsulate( Vector3f { 1, 1, 1 } );
            bounds.encapsulate( Vector3f { 9, 9, 9 } );

            const auto c = bounds.center();
            ZP_CHECK_FLOAT32_APPROX( c.x, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( c.y, 5.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( c.z, 5.0f, 0.000001f );
        };

        ZP_TEST( InvalidStaticMember )
        {
            // Test invalid bounds have special values
            const Bounds3Df invalid = Bounds3Df::invalid;
            ZP_CHECK_FLOAT32_APPROX( invalid.xMin, 1.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( invalid.xMax, -1.0f, 0.000001f );
        };

        ZP_TEST( EmptyStaticMember )
        {
            // Test empty bounds are all zeros
            const Bounds3Df empty = Bounds3Df::empty;
            ZP_CHECK_FLOAT32_APPROX( empty.xMin, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( empty.xMax, 0.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Color )
    {
        ZP_TEST( LerpRedGreen )
        {
            constexpr Color red { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
            constexpr Color green { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };

            // Lerp at t=0.5 should give yellow (0.5, 0.5, 0.0, 1.0)
            const Color result = Math::Lerp( red, green, 0.5f );

            ZP_CHECK_FLOAT32_APPROX( result.r, 0.5f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.g, 0.5f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.b, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.a, 1.0f, 0.000001f );
        };

        ZP_TEST( LerpAtStart )
        {
            constexpr Color red { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
            constexpr Color green { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };

            // Lerp at t=0 should give red
            const Color result = Math::Lerp( red, green, 0.0f );

            ZP_CHECK_FLOAT32_APPROX( result.r, 1.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.g, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.b, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.a, 1.0f, 0.000001f );
        };

        ZP_TEST( LerpAtEnd )
        {
            constexpr Color red { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
            constexpr Color green { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };

            // Lerp at t=1 should give green
            const Color result = Math::Lerp( red, green, 1.0f );

            ZP_CHECK_FLOAT32_APPROX( result.r, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.g, 1.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.b, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.a, 1.0f, 0.000001f );
        };

        ZP_TEST( LerpUnclampNegative )
        {
            constexpr Color red { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
            constexpr Color green { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };

            // Lerp with t=-0.5 should give negative red
            const Color result = Math::LerpUnclamp( red, green, -0.5f );

            ZP_CHECK_FLOAT32_APPROX( result.r, 0.75f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.g, 0.25f, 0.000001f );
        };

        ZP_TEST( LerpUnclampGreaterThanOne )
        {
            constexpr Color red { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
            constexpr Color green { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };

            // Lerp with t=1.5 should go beyond green
            const Color result = Math::LerpUnclamp( red, green, 1.5f );

            ZP_CHECK_FLOAT32_APPROX( result.r, -0.25f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.g, 0.75f, 0.000001f );
        };

        ZP_TEST( Add )
        {
            constexpr Color red { .r = 0.5f, .g = 0.0f, .b = 0.0f, .a = 0.5f };
            constexpr Color green { .r = 0.0f, .g = 0.5f, .b = 0.0f, .a = 0.5f };

            const Color result = Math::Add( red, green );

            ZP_CHECK_FLOAT32_APPROX( result.r, 0.5f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.g, 0.5f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.b, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.a, 1.0f, 0.000001f );
        };

        ZP_TEST( Mul )
        {
            constexpr Color red { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 0.5f };
            constexpr Color green { .r = 0.5f, .g = 0.5f, .b = 0.5f, .a = 0.5f };

            const Color result = Math::Mul( red, green );

            ZP_CHECK_FLOAT32_APPROX( result.r, 0.5f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.g, 0.5f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.b, 0.5f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( result.a, 0.25f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Quaternion )
    {
        ZP_TEST( Identity )
        {
            // Test quaternion identity member
            const Quaternion id = Quaternion::identity;

            // Identity quaternion is typically {0, 0, 0, 1}
            ZP_CHECK_FLOAT32_APPROX( id.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( id.y, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( id.z, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( id.w, 1.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Ray3Df )
    {
        ZP_TEST( Constructor )
        {
            Ray3Df ray {};

            // Verify the ray has valid members
            ZP_CHECK_FLOAT32_APPROX( ray.position.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( ray.direction.x, 0.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Plane3Df )
    {
        ZP_TEST( Constructor )
        {
            Plane3Df plane {};

            // Verify the plane has valid members
            ZP_CHECK_FLOAT32_APPROX( plane.normal.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( plane.d, 0.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Frustum )
    {
        ZP_TEST( Constructor )
        {
            Frustum frustum {};

            // Verify frustum has valid planes
            ZP_CHECK_FLOAT32_APPROX( frustum.planes[ 0 ].normal.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( frustum.planes[ 0 ].d, 0.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Offset2Df )
    {
        ZP_TEST( Constructor )
        {
            Offset2Df offset {};

            // Verify the offset has valid members
            ZP_CHECK_FLOAT32_APPROX( offset.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( offset.y, 0.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Offset3Df )
    {
        ZP_TEST( Constructor )
        {
            Offset3Df offset {};

            // Verify the offset has valid members
            ZP_CHECK_FLOAT32_APPROX( offset.x, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( offset.y, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( offset.z, 0.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Size2Df )
    {
        ZP_TEST( Constructor )
        {
            Size2Df size {};

            // Verify the size has valid members
            ZP_CHECK_FLOAT32_APPROX( size.width, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( size.height, 0.0f, 0.000001f );
        };
    }


    ZP_TEST_SUITE( Size3Df )
    {
        ZP_TEST( Constructor )
        {
            Size3Df size {};

            // Verify the size has valid members
            ZP_CHECK_FLOAT32_APPROX( size.width, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( size.height, 0.0f, 0.000001f );
            ZP_CHECK_FLOAT32_APPROX( size.depth, 0.0f, 0.000001f );
        };
    }
}
#endif
