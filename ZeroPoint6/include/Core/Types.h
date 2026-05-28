//
// Created by phosg on 11/5/2021.
//

#ifndef ZP_TYPES_H
#define ZP_TYPES_H

#include "Core/Defines.h"

using zp_bool_t = bool;

using zp_int8_t = signed char;
using zp_uint8_t = unsigned char;

using zp_int16_t = short;
using zp_uint16_t = unsigned short;

#define USE_LONG_AS_INT 0
#if USE_LONG_AS_INT
using zp_int32_t = long;
using zp_uint32_t = unsigned long;
#else
using zp_int32_t = int;
using zp_uint32_t = unsigned int;
#endif

using zp_int64_t = long long;
using zp_uint64_t = unsigned long long;

using zp_float16_t = zp_uint16_t;
using zp_float32_t = float;
using zp_float64_t = double;

using zp_handle_t = void*;
constexpr void* ZP_NULL_HANDLE = nullptr;

#define ZP_BOOL8( x )  zp_uint8_t x : 1
#define ZP_BOOL16( x ) zp_uint16_t x : 1
#define ZP_BOOL32( x ) zp_uint32_t x : 1
#define ZP_BOOL64( x ) zp_uint64_t x : 1

#ifdef ZP_PLATFORM_ARCH64
using zp_size_t = unsigned long long;
using zp_ptrdiff_t = long long;
using zp_ptr_t = unsigned long long;
#else
using zp_size_t = unsigned int;
using zp_ptrdiff_t = int;
using zp_ptr_t = unsigned int;
#endif

#if ZP_USE_UTF8_LITERALS
using zp_char8_t = char8_t;
using zp_char16_t = char16_t;
using zp_char32_t = char32_t;
using zp_wchar_t = wchar_t;
#else
using zp_char8_t = char;
using zp_char16_t = zp_uint16_t;
using zp_char32_t = zp_uint32_t;
using zp_wchar_t = zp_uint32_t;
#endif


using zp_nullptr_t = decltype( nullptr );
using zp_void_t = void;

//
//
//

namespace zp
{
    template<typename T, T v>
    struct compile_time_constant
    {
        using type = T;
        static constexpr T value = v;
    };

    template<zp_bool_t v>
    using compile_time_constant_bool = compile_time_constant<zp_bool_t, v>;

    using compile_time_constant_true = compile_time_constant_bool<true>;
    using compile_time_constant_false = compile_time_constant_bool<false>;

    // remove cv
    template<typename T>
    struct remove_cv
    {
        using type = T;
    };

    template<typename T>
    struct remove_cv<const T>
    {
        using type = T;
    };

    template<typename T>
    struct remove_cv<volatile T>
    {
        using type = T;
    };

    template<typename T>
    struct remove_cv<const volatile T>
    {
        using type = T;
    };

    template<typename T>
    using remove_cv_t = remove_cv<T>::type;

    // enable if
    template<zp_bool_t B, typename T = void>
    struct enable_if;

    template<typename T>
    struct enable_if<true, T>
    {
        using type = T;
    };

    template<zp_bool_t B, typename T = void>
    using enable_if_t = enable_if<B, T>::type;

    // size of
    template<typename T>
    struct size_of : compile_time_constant<zp_size_t, sizeof( T )>
    {
    };

    template<typename T>
    constexpr size_of<T>::type size_of_v = size_of<T>::value;

    // alignment of
    template<typename T>
    struct alignment_of : compile_time_constant<zp_size_t, alignof( T )>
    {
    };

    template<typename T>
    constexpr alignment_of<T>::type alignment_of_v = alignment_of<T>::value;

    // is same
    template<typename T, typename U>
    struct is_same : compile_time_constant_false
    {
    };

    template<typename T>
    struct is_same<T, T> : compile_time_constant_true
    {
    };

    template<typename T, typename U>
    constexpr is_same<T, U>::type is_same_v = is_same<T, U>::value;

    // is void
    template<typename T>
    struct is_void : is_same<T, void>
    {
    };

    template<typename T>
    constexpr is_void<T>::type is_void_v = is_void<T>::value;

    // is integer
    template<typename T>
    struct is_integer : compile_time_constant_bool<is_same_v<remove_cv_t<T>, zp_bool_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_int8_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_int16_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_int32_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_int64_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_uint8_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_uint16_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_uint32_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_uint64_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_char8_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_char16_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_char32_t> ||
                                                   is_same_v<remove_cv_t<T>, zp_wchar_t> ||
                                                   false>
    {
    };

    template<typename T>
    constexpr is_integer<T>::value is_integer_v = is_integer<T>::value;

    // is floating point
    template<typename T>
    struct is_floating_point : compile_time_constant_bool<
                                   is_same_v<remove_cv_t<T>, zp_float16_t> ||
                                   is_same_v<remove_cv_t<T>, zp_float32_t> ||
                                   is_same_v<remove_cv_t<T>, zp_float64_t> ||
                                   false>
    {
    };

    template<typename T>
    constexpr is_floating_point<T>::value is_floating_point_v = is_floating_point<T>::value;

    // is pointer
    template<typename T>
    struct is_pointer : compile_time_constant_false
    {
    };

    template<typename T>
    struct is_pointer<T*> : compile_time_constant_true
    {
    };

    template<typename T>
    struct is_pointer<T* const> : compile_time_constant_true
    {
    };

    template<typename T>
    struct is_pointer<T* volatile> : compile_time_constant_true
    {
    };

    template<typename T>
    struct is_pointer<T* const volatile> : compile_time_constant_true
    {
    };

    template<typename T>
    constexpr is_pointer<T>::type is_pointer_v = is_pointer<T>::value;

    // is reference
    template<typename T>
    struct is_reference : compile_time_constant_false
    {
    };

    template<typename T>
    struct is_reference<T&> : compile_time_constant_true
    {
    };

    template<typename T>
    struct is_reference<T&&> : compile_time_constant_true
    {
    };

    template<typename T>
    constexpr is_reference<T>::type is_reference_v = is_reference<T>::value;

    // is enum
    template<typename T>
    struct is_enum : compile_time_constant_bool<__is_enum( T )>
    {
    };

    template<typename T>
    constexpr is_enum<T>::type is_enum_v = is_enum<T>::value;

} // namespace zp

//
//
//

using zp_hash32_t = zp_uint32_t;

using zp_hash64_t = zp_uint64_t;

struct zp_hash128_t
{
    zp_uint64_t m32;
    zp_uint64_t m10;
};

ZP_FORCEINLINE zp_bool_t operator==( const zp_hash128_t& lh, const zp_hash128_t& rh )
{
    return lh.m32 == rh.m32 && lh.m10 == rh.m10;
}

ZP_FORCEINLINE zp_bool_t operator!=( const zp_hash128_t& lh, const zp_hash128_t& rh )
{
    return !( lh == rh );
}

ZP_FORCEINLINE zp_bool_t operator<( const zp_hash128_t& lh, const zp_hash128_t& rh )
{
    return lh.m32 != rh.m32 ? lh.m32 < rh.m32 : lh.m10 < rh.m10;
}

ZP_FORCEINLINE zp_bool_t operator>( const zp_hash128_t& lh, const zp_hash128_t& rh )
{
    return lh.m32 != rh.m32 ? lh.m32 > rh.m32 : lh.m10 > rh.m10;
}

ZP_FORCEINLINE zp_size_t operator%( const zp_hash128_t& lh, zp_size_t rh )
{
    return ( lh.m32 ^ lh.m10 ) % rh;
}

//
//
//

struct zp_guid128_t
{
    zp_uint64_t m32;
    zp_uint64_t m10;

    constexpr explicit operator zp_hash128_t() const
    {
        return { .m32 = m32, .m10 = m10 };
    }
};

ZP_FORCEINLINE zp_bool_t operator==( const zp_guid128_t& lh, const zp_guid128_t& rh )
{
    return lh.m32 == rh.m32 && lh.m10 == rh.m10;
}

ZP_FORCEINLINE zp_bool_t operator!=( const zp_guid128_t& lh, const zp_guid128_t& rh )
{
    return !( lh.m32 == rh.m32 && lh.m10 == rh.m10 );
}

ZP_FORCEINLINE zp_bool_t operator<( const zp_guid128_t& lh, const zp_guid128_t& rh )
{
    return lh.m32 != rh.m32 ? lh.m32 < rh.m32 : lh.m10 < rh.m10;
}

ZP_FORCEINLINE zp_bool_t operator>( const zp_guid128_t& lh, const zp_guid128_t& rh )
{
    return lh.m32 != rh.m32 ? lh.m32 > rh.m32 : lh.m10 > rh.m10;
}

//
//
//

using zp_time_t = zp_uint64_t;

#define ZP_TIME_INFINITE static_cast<zp_time_t>( ~0 )

//
//
//

namespace zp
{
    using MemoryLabel = zp_uint8_t;
}

//
//
//

namespace zp
{
    enum
    {
        kMaxStackTraceDepth = 30,
    };

    struct StackTrace
    {
        // FixedArray<void*, kMaxStackTraceDepth> stack;
        void* stack[ kMaxStackTraceDepth ];
        zp_size_t length;
        zp_hash64_t hash;
    };
} // namespace zp

#endif // ZP_TYPES_H
