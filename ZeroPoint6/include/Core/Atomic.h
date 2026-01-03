//
// Created by phosg on 1/27/2022.
//

#ifndef ZP_ATOMIC_H
#define ZP_ATOMIC_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"

#if ZP_PLATFORM_WINDOWS

#include <intrin.h>

#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedAdd)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchanger)

#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedAdd64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedCompareExchanger64)
#else
#error "Platform Not Implemented for Atomic.h"
#endif

namespace zp
{
    namespace Atomic
    {
        ZP_FORCEINLINE void MemoryBarrier()
        {
#if ZP_PLATFORM_WINDOWS
            _mm_mfence();
#endif
        }

        //
        // Int32
        //

        ZP_FORCEINLINE zp_int32_t And( zp_int32_t* destination, zp_int32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedAnd( reinterpret_cast<__LONG32 volatile *>(destination), value );
#endif
        }

        ZP_FORCEINLINE zp_int32_t Or( zp_int32_t* destination, zp_int32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedOr( reinterpret_cast<__LONG32 volatile *>(destination), value );
#endif
        }

        ZP_FORCEINLINE zp_int32_t Xor( zp_int32_t* destination, zp_int32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedXor( reinterpret_cast<__LONG32 volatile *>(destination), value );
#endif
        }

        ZP_FORCEINLINE zp_int32_t Increment( zp_int32_t* addend )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedIncrement( reinterpret_cast<__LONG32 volatile *>(addend) );
#endif
        }

        ZP_FORCEINLINE zp_int32_t Decrement( zp_int32_t* addend )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedDecrement( reinterpret_cast<__LONG32 volatile *>(addend) );
#endif
        }

        ZP_FORCEINLINE zp_int32_t Add( zp_int32_t* addend, zp_int32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedAdd( reinterpret_cast<__LONG32 volatile *>(addend), value );
#endif
        }

        ZP_FORCEINLINE zp_int32_t Exchange( zp_int32_t* target, zp_int32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedExchange( reinterpret_cast<__LONG32 volatile *>(target), value );
#endif
        }

        ZP_FORCEINLINE zp_int32_t ExchangeAdd( zp_int32_t* target, zp_int32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedExchangeAdd( reinterpret_cast<__LONG32 volatile *>(target), value );
#endif
        }

        ZP_FORCEINLINE zp_int32_t CompareExchange( zp_int32_t* destination, zp_int32_t exChange, zp_int32_t comperand )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedCompareExchange( reinterpret_cast<__LONG32 volatile *>(destination), exChange, comperand );
#endif
        }

        //
        // UInt32
        //

        ZP_FORCEINLINE zp_uint32_t And( zp_uint32_t* destination, zp_uint32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedAnd( reinterpret_cast<__LONG32 volatile *>(destination), value );
#endif
        }

        ZP_FORCEINLINE zp_uint32_t Or( zp_uint32_t* destination, zp_uint32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedOr( reinterpret_cast<__LONG32 volatile *>(destination), value );
#endif
        }

        ZP_FORCEINLINE zp_uint32_t Xor( zp_uint32_t* destination, zp_uint32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedXor( reinterpret_cast<__LONG32 volatile *>(destination), value );
#endif
        }

        ZP_FORCEINLINE zp_uint32_t Increment( zp_uint32_t* addend )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedIncrement( reinterpret_cast<__LONG32 volatile *>(addend) );
#endif
        }

        ZP_FORCEINLINE zp_uint32_t Decrement( zp_uint32_t* addend )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedDecrement( reinterpret_cast<__LONG32 volatile *>(addend) );
#endif
        }

        ZP_FORCEINLINE zp_uint32_t Add( zp_uint32_t* addend, zp_uint32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedAdd( reinterpret_cast<__LONG32 volatile *>(addend), value );
#endif
        }

        ZP_FORCEINLINE zp_uint32_t Exchange( zp_uint32_t* target, zp_uint32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedExchange( reinterpret_cast<__LONG32 volatile *>(target), value );
#endif
        }

        ZP_FORCEINLINE zp_uint32_t ExchangeAdd( zp_uint32_t* target, zp_uint32_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedExchangeAdd( reinterpret_cast<__LONG32 volatile *>(target), value );
#endif
        }

        ZP_FORCEINLINE zp_uint32_t CompareExchange( zp_uint32_t* destination, zp_uint32_t exChange, zp_uint32_t comperand )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedCompareExchange( reinterpret_cast<__LONG32 volatile *>(destination), exChange, comperand );
#endif
        }

        //
        // SizeT
        //

        ZP_FORCEINLINE zp_size_t AddSizeT( zp_size_t* addend, zp_size_t value )
        {
#if ZP_PLATFORM_WINDOWS
#if ZP_PLATFORM_ARCH64
            return _InterlockedAdd64( reinterpret_cast<__int64 volatile *>(addend), static_cast<__int64>(value) );
#else
            return _InterlockedAdd( (long*)addend );
#endif
#endif
        }

        ZP_FORCEINLINE zp_size_t IncrementSizeT( zp_size_t* addend )
        {
#if ZP_PLATFORM_WINDOWS
#if ZP_PLATFORM_ARCH64
            return _InterlockedIncrement64( reinterpret_cast<__int64 volatile *>(addend) );
#else
            return _InterlockedIncrement( (long*)addend );
#endif
#endif
        }

        ZP_FORCEINLINE zp_size_t DecrementSizeT( zp_size_t* addend )
        {
#if ZP_PLATFORM_WINDOWS
#if ZP_PLATFORM_ARCH64
            return _InterlockedDecrement64( reinterpret_cast<__int64 volatile *>(addend) );
#else
            return _InterlockedDecrement( (long*)addend );
#endif
#endif
        }

        ZP_FORCEINLINE zp_size_t ExchangeSizeT( zp_size_t* target, zp_size_t value )
        {
#if ZP_PLATFORM_WINDOWS
#if ZP_PLATFORM_ARCH64
            return _InterlockedExchange64( reinterpret_cast<__int64 volatile *>(target), static_cast<__int64>(value) );
#else
            return _InterlockedExchange( (long*)target, (long)value );
#endif
#endif
        }

        ZP_FORCEINLINE zp_size_t ExchangeAddSizeT( zp_size_t* target, zp_size_t value )
        {
#if ZP_PLATFORM_WINDOWS
#if ZP_PLATFORM_ARCH64
            return _InterlockedExchangeAdd64( reinterpret_cast<__int64 volatile *>(target), static_cast<__int64>(value) );
#else
            return _InterlockedExchangeAdd( (long*)target, (long)value );
#endif
#endif
        }

        ZP_FORCEINLINE zp_size_t CompareExchangeSizeT( zp_size_t* destination, zp_size_t exChange, zp_size_t comperand )
        {
#if ZP_PLATFORM_WINDOWS
#if ZP_PLATFORM_ARCH64
            return _InterlockedCompareExchange64( reinterpret_cast<__int64 volatile *>(destination), static_cast<__int64>(exChange), static_cast<__int64>(comperand) );
#else
            return _InterlockedCompareExchange((long*)destination, (long)exChange, (long)comperand );
#endif
#endif
        }
        //
        // Int64
        //

        ZP_FORCEINLINE zp_int64_t And( zp_int64_t* destination, zp_int64_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedAnd64( destination, value );
#endif
        }

        ZP_FORCEINLINE zp_int64_t Or( zp_int64_t* destination, zp_int64_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedOr64( destination, value );
#endif
        }

        ZP_FORCEINLINE zp_int64_t Xor( zp_int64_t* destination, zp_int64_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedXor64( destination, value );
#endif
        }

        ZP_FORCEINLINE zp_int64_t Increment( zp_int64_t* addend )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedIncrement64( addend );
#endif
        }

        ZP_FORCEINLINE zp_int64_t Decrement( zp_int64_t* addend )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedDecrement64( addend );
#endif
        }

        ZP_FORCEINLINE zp_int64_t Add( zp_int64_t* addend, zp_int64_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedAdd64( addend, value );
#endif
        }

        ZP_FORCEINLINE zp_int64_t Exchange( zp_int64_t* target, zp_int64_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedExchange64( target, value );
#endif
        }

        ZP_FORCEINLINE zp_int64_t ExchangeAdd( zp_int64_t* target, zp_int64_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedExchangeAdd64( target, value );
#endif
        }

        ZP_FORCEINLINE zp_int64_t CompareExchange( zp_int64_t* destination, zp_int64_t exChange, zp_int64_t comperand )
        {
#if ZP_PLATFORM_WINDOWS
            return _InterlockedCompareExchange64( destination, exChange, comperand );
#endif
        }

        //
        // UInt64
        //

        ZP_FORCEINLINE zp_uint64_t Exchange( zp_uint64_t* target, zp_uint64_t value )
        {
#if ZP_PLATFORM_WINDOWS
            return static_cast<zp_uint64_t>( _InterlockedExchange64( reinterpret_cast<__int64 volatile *>( target ), static_cast<__int64>( value ) ) );
#endif
        }
    };
}

#endif //ZP_ATOMIC_H
