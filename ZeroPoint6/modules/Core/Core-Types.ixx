module;

#include <cstdint>

export module Core:Types;

export
{
    typedef signed char zp_int8_t;
    typedef unsigned char zp_uint8_t;

    typedef signed short zp_int16_t;
    typedef unsigned short zp_uint16_t;

    typedef signed int zp_int32_t;
    typedef unsigned int zp_uint32_t;

    typedef signed long long zp_int64_t;
    typedef unsigned long long zp_uint64_t;

    typedef float zp_float32_t;
    typedef double zp_float64_t;

    typedef void* zp_handle_t;
    #define ZP_NULL_HANDLE  static_cast<zp_handle_t>(nullptr)
};
