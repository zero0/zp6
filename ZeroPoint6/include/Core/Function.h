//
// Created by phosg on 7/27/2024.
//

#ifndef ZP_FUNCTION_H
#define ZP_FUNCTION_H

namespace zp
{
    template<typename R, typename ... P>
    class Function
    {
    public:
        typedef R (* func_t)( P... );

        Function()
            : m_callable()
            , m_stub()
        {
        }

        Function( func_t func )
            : m_callable()
            , m_stub()
        {
            *this = zp_move( from_function( func ) );
        }

        Function& operator=( func_t func )
        {
            *this = zp_move( from_function( func ) );
            return *this;
        }

        template<class T, R (T::*TMethod)( P... )>
        static Function from_method( T* object_ptr )
        {
            Function d;
            d.m_callable = { .m_ptr = object_ptr };
            d.m_stub = &stub_method<T, TMethod>;

            return d;
        }

        template<class T, R (T::*TMethod)( P... ) const>
        static Function from_method( T* object_ptr )
        {
            Function d;
            d.m_callable = { .m_ptr = object_ptr };
            d.m_stub = &stub_method<T, TMethod>;

            return d;
        }

        static Function from_function( func_t func )
        {
            Function d;
            d.m_callable = { .m_func = func };
            d.m_stub = &stub_function;

            return d;
        }

        operator zp_bool_t() const
        {
            return m_stub != nullptr;
        }

        R operator()( P ... args )
        {
            return ( m_stub )( m_callable, args... );
        }

        R operator()( P ... args ) const
        {
            return ( m_stub )( m_callable, args... );
        }

    private:
        union Callable
        {
            void* m_ptr;
            func_t m_func;
        };

        typedef R (* stub_t)( const Callable&, P... );

        Callable m_callable;
        stub_t m_stub;

        static R stub_function( const Callable& callable, P... args )
        {
            return callable.m_func( args... );
        }

        template<class T, R (T::*TMethod)( P... )>
        static R stub_method( const Callable& callable, P... args )
        {
            T* t = static_cast<T*>( callable.m_ptr );
            return ( t->*TMethod )( args... );
        }
    };
}

#endif //ZP_FUNCTION_H
