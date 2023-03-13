//
// Created by phosg on 2/14/2022.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Math.h"

#include <cmath>
#include <immintrin.h>

#define _MM_SHUFFLER( fp0, fp1, fp2, fp3 )   _MM_SHUFFLE(fp3,fp2,fp1,fp0)

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
        1, 1, 1,
        -1, -1, -1,
    };

    template<>
    const Bounds3Df Bounds3Df::empty = {
        0, 0, 0,
        0, 0, 0,
    };

    template<>
    const Bounds3Di Bounds3Di::invalid = {
        1, 1, 1,
        -1, -1, -1,
    };

    template<>
    const Bounds3Di Bounds3Di::empty = {
        0, 0, 0,
        0, 0, 0,
    };


    template<>
    const Vector2f Vector2f::zero = { 0, 0 };

    template<>
    const Vector2f Vector2f::one = { 1, 1 };

    template<>
    const Vector2i Vector2i::zero = { 0, 0 };

    template<>
    const Vector2i Vector2i::one = { 1, 1 };


    template<>
    const Vector3f Vector3f::zero = { 0, 0, 0 };

    template<>
    const Vector3f Vector3f::one = { 1, 1, 1 };

    template<>
    const Vector3i Vector3i::zero = { 0, 0, 0 };

    template<>
    const Vector3i Vector3i::one = { 1, 1, 1 };


    template<>
    const Vector4f Vector4f::zero = { 0, 0, 0, 0 };

    template<>
    const Vector4f Vector4f::one = { 1, 1, 1, 1 };

    template<>
    const Vector4i Vector4i::zero = { 0, 0, 0, 0 };

    template<>
    const Vector4i Vector4i::one = { 1, 1, 1, 1 };

    //
    //
    //

    const Quaternion Quaternion::identity { 0, 0, 0, 1 };

    //
    //
    //

    template<>
    const Matrix4x4f Matrix4x4f::identity {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    template<>
    const Matrix4x4f Matrix4x4f::zero {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
    };

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
        }

        Vector3f PerspectiveDivide( const Vector4f& v )
        {
            const __m128 invW = _mm_rcp_ps( _mm_set1_ps( v.w ) );
            const __m128 xyzw = _mm_load_ps( v.m );

            ZP_ALIGN16 zp_float32_t m[4];
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

        Color Mul( const Color& x, const Color& y )
        {
            const __m128 mx = _mm_loadu_ps( x.rgba );
            const __m128 my = _mm_loadu_ps( y.rgba );

            ZP_ALIGN16 Color r {};
            _mm_store_ps( r.rgba, _mm_mul_ps( mx, my ) );
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
            _mm_storer_ps( r.c0.m, col0 );
            _mm_storer_ps( r.c1.m, col1 );
            _mm_storer_ps( r.c2.m, col2 );
            _mm_storer_ps( r.c3.m, col3 );

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
            _mm_storer_ps( r.m[ 0 ], lc0 );
            _mm_storer_ps( r.m[ 1 ], lc1 );
            _mm_storer_ps( r.m[ 2 ], lc2 );
            _mm_storer_ps( r.m[ 3 ], lc3 );

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

            ZP_ALIGN16 zp_float32_t fmin[4];
            ZP_ALIGN16 zp_float32_t fmax[4];
            _mm_storer_ps( fmin, _mm_sub_ps( tcenter, textents ) );
            _mm_storer_ps( fmax, _mm_add_ps( tcenter, textents ) );

            Bounds3Df r {
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

            ZP_ALIGN16 zp_float32_t m[4];
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

            ZP_ALIGN16 zp_float32_t m[4];
            _mm_storer_ps( m, _mm_normalize_ps( v ) );

            Vector2f r { .x = m[ 0 ], .y = m[ 1 ] };
            return r;
        }

        Vector3f Normalize( const Vector3f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, lh.z, 0 );

            ZP_ALIGN16 zp_float32_t m[4];
            _mm_storer_ps( m, _mm_normalize_ps( v ) );

            Vector3f r { .x = m[ 0 ], .y = m[ 1 ], .z = m[ 2 ] };
            return r;
        }

        Vector4f Normalize( const Vector4f& lh )
        {
            const __m128 v = _mm_setr_ps( lh.x, lh.y, lh.z, lh.w );

            ZP_ALIGN16 Vector4f r;
            _mm_storer_ps( r.m, _mm_normalize_ps( v ) );

            return r;
        }

        Vector2i Cmp( const Vector2f& lh, const Vector2f& rh )
        {
            const __m128 vlh = _mm_setr_ps( lh.x, lh.y, 0, 0 );
            const __m128 vrh = _mm_setr_ps( rh.x, rh.y, 0, 0 );

            const __m128 veq = _mm_cmpeq_ps( vlh, vrh );
            const __m128 vlt = _mm_cmplt_ps( vlh, vrh );

            ZP_ALIGN16 zp_uint32_t eq[4];
            ZP_ALIGN16 zp_uint32_t lt[4];
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>(eq), veq );
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>(lt), vlt );

            return {
                .x = lt[ 0 ] ? -1 : eq[ 0 ] ? 0 : 1,
                .y = lt[ 1 ] ? -1 : eq[ 1 ] ? 0 : 1,
            };
        }

        Vector3i Cmp( const Vector3f& lh, const Vector3f& rh )
        {
            const __m128 vlh = _mm_setr_ps( lh.x, lh.y, lh.z, 0 );
            const __m128 vrh = _mm_setr_ps( rh.x, rh.y, rh.z, 0 );

            const __m128 veq = _mm_cmpeq_ps( vlh, vrh );
            const __m128 vlt = _mm_cmplt_ps( vlh, vrh );

            ZP_ALIGN16 zp_uint32_t eq[4];
            ZP_ALIGN16 zp_uint32_t lt[4];
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>(eq), veq );
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>(lt), vlt );

            return {
                .x = lt[ 0 ] ? -1 : eq[ 0 ] ? 0 : 1,
                .y = lt[ 1 ] ? -1 : eq[ 1 ] ? 0 : 1,
                .z = lt[ 2 ] ? -1 : eq[ 2 ] ? 0 : 1,
            };
        }

        Vector4i Cmp( const Vector4f& lh, const Vector4f& rh )
        {
            const __m128 vlh = _mm_setr_ps( lh.x, lh.y, lh.z, lh.w );
            const __m128 vrh = _mm_setr_ps( rh.x, rh.y, rh.z, rh.w );

            const __m128 veq = _mm_cmpeq_ps( vlh, vrh );
            const __m128 vlt = _mm_cmplt_ps( vlh, vrh );

            ZP_ALIGN16 zp_uint32_t eq[4];
            ZP_ALIGN16 zp_uint32_t lt[4];
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>(eq), veq );
            _mm_storer_ps( reinterpret_cast<zp_float32_t*>(lt), vlt );

            return {
                .x = lt[ 0 ] ? -1 : eq[ 0 ] ? 0 : 1,
                .y = lt[ 1 ] ? -1 : eq[ 1 ] ? 0 : 1,
                .z = lt[ 2 ] ? -1 : eq[ 2 ] ? 0 : 1,
                .w = lt[ 3 ] ? -1 : eq[ 3 ] ? 0 : 1,
            };
        }

        Matrix4x4f OrthoLH( const Rect2Df& orthoRect, zp_float32_t zNear, zp_float32_t zFar, zp_float32_t orthoScale )
        {
            const zp_float32_t r = orthoRect.right(); //orthoRect.offset.x + ( orthoRect.size.width * 0.5f );
            const zp_float32_t l = orthoRect.left(); //orthoRect.offset.x + ( orthoRect.size.width * -0.5f );
            const zp_float32_t t = orthoRect.top(); //orthoRect.offset.y + ( orthoRect.size.height * 0.5f );
            const zp_float32_t b = orthoRect.bottom(); //orthoRect.offset.y + ( orthoRect.size.height * -0.5f );

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
    }
}