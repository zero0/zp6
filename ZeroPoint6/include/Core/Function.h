//
// Created by phosg on 7/27/2024.
//

#ifndef ZP_FUNCTION_H
#define ZP_FUNCTION_H

#include "Core/Types.h"
#include "Core/Memory.h"

namespace zp
{
    template<typename TSignature>
    class Function
    {
    };

    template<typename R, typename ... P>
    class Function<R( P... )>
    {
    public:
        using func_t = R ( * )( P... );

        Function()
            : m_data()
            , m_stub()
        {
        }

        Function( zp_nullptr_t )
            : m_data()
            , m_stub()
        {
        }

        Function( const Function& func )
            : m_data()
            , m_stub( func.m_stub )
        {
        }

        Function( Function&& func ) noexcept
        {
            m_stub = func.m_stub;

            func.m_stub = {};
        }

        template<typename TFunc>
        Function( TFunc&& func )
            : m_data()
            , m_stub()
        {
            *this = from_lambda( func );
        }

        Function& operator=( zp_nullptr_t )
        {
            m_stub = {};

            return *this;
        }

        Function& operator=( const Function& func )
        {
            m_data = func.m_data;
            m_stub = func.m_stub;

            return *this;
        }

        Function& operator=( Function&& func ) noexcept
        {
            m_data = func.m_data;
            m_stub = func.m_stub;

            func.m_stub = {};

            return *this;
        }

        Function& operator=( func_t func )
        {
            callable()->m_functionPtr = func;
            m_stub = &stub_function;

            return *this;
        }

        template<typename TFunc>
        Function& operator=( TFunc&& func )
        {
            return *this;
        }

        template<class T, R (T::*TMethod)( P... )>
        static Function from_method( T* object_ptr )
        {
            Function method {};
            method.callable()->m_objPtr = object_ptr;
            method.m_stub = &stub_method<T, TMethod>;

            return method;
        }

        template<class T, R (T::*TMethod)( P... ) const>
        static Function from_method( const T* object_ptr )
        {
            Function method {};
            method.callable()->m_constObjPtr = object_ptr;
            method.m_stub = &stub_const_method<T, TMethod>;

            return method;
        }

        static Function from_function( func_t func )
        {
            Function function {};
            function.callable()->m_functionPtr = func;
            function.m_stub = &stub_function;

            return function;
        }

        template<typename TLambda>
        static Function from_lambda( TLambda lambda )
        {
            Function function {};
            TLambda* ptr = function.as<TLambda>();
            *ptr = ( lambda );
            function.m_stub = &stub_lambda<TLambda>;

            return function;
        }

        explicit operator zp_bool_t() const
        {
            return m_stub != nullptr;
        }

        auto operator()( P ... args ) -> R
        {
            return m_stub( callable(), args... );
        }

        auto operator()( P ... args ) const -> R
        {
            return m_stub( callable(), args... );
        }

        auto operator==( const Function& func ) const -> zp_bool_t
        {
            const Callable* thisCallable = callable();
            const Callable* otherCallable = func.callable();

            return m_stub == func.m_stub && thisCallable->m_functionPtr == otherCallable->m_functionPtr;
        }

        auto operator==( zp_nullptr_t ) const -> zp_bool_t
        {
            return m_stub == nullptr;
        }

        auto operator!=( zp_nullptr_t ) const -> zp_bool_t
        {
            return m_stub != nullptr;
        }

    private:
        class _UndefinedClass;

        union Callable
        {
            void* m_objPtr;
            const void* m_constObjPtr;
            void (_UndefinedClass::*m_memberPtr)();
            func_t m_functionPtr;
        };

        using StubFunc = R ( * )( const Callable*, P... );

        ZP_FORCEINLINE Callable* callable()
        {
            return reinterpret_cast<Callable*>( m_data.data() );
        }

        ZP_FORCEINLINE const Callable* callable() const
        {
            return reinterpret_cast<const Callable*>( m_data.data() );
        }

        template<typename TLambda>
        TLambda* as()
        {
            return reinterpret_cast<TLambda*>( m_data.data() );
        }

        FixedArray<zp_uint8_t, sizeof( Callable )> m_data;
        StubFunc m_stub;

        static R stub_function( const Callable* callable, P... args )
        {
            return callable->m_functionPtr( args... );
        }

        template<class T, R (T::*TMethod)( P... )>
        static R stub_method( const Callable* callable, P... args )
        {
            T* obj = static_cast<T*>( callable->m_objPtr );
            return ( obj->*TMethod )( args... );
        }

        template<class T, R (T::*TMethod)( P... )>
        static R stub_const_method( const Callable* callable, P... args )
        {
            const T* obj = static_cast<const T*>( callable->m_constObjPtr );
            return ( obj->*TMethod )( args... );
        }

        template<class T>
        static R stub_lambda( const Callable* callable, P... args )
        {
            using TLambda = zp_remove_reference_t<T>;
            TLambda* func = reinterpret_cast<TLambda*>( callable->m_functionPtr );
            return ( *func )( args... );
        }
    };

    template<typename>
    struct _function_guide_helper
    {
    };

    template<typename R, typename TObj, typename ...TArgs>
    struct _function_guide_helper<R( TObj::* )( TArgs... )>
    {
        using type = R( TArgs... );
    };

    template<typename TFunc, typename TOp>
    using _function_guide_t = typename _function_guide_helper<TOp>::type;

    // deduction guides
    template<typename R, typename ...Args>
    Function( R( * )( Args... ) )
    ->
    Function<R( Args... )>;

    template<typename TFunc, typename TSignature = _function_guide_t<TFunc, decltype( &TFunc::operator() )>>
    Function( TFunc )
    ->
    Function<TSignature>;
}

#endif //ZP_FUNCTION_H
