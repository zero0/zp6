//
// Created by phosg on 7/27/2024.
//

#ifndef ZP_FUNCTION_H
#define ZP_FUNCTION_H

namespace zp
{
    template<typename R, typename ... P>
    class Delegate
    {
    public:
        typedef R (* func_t)( P... );

        Delegate()
            : m_ptr()
            , m_stub()
        {
        }

        template<class T, R (T::*TMethod)( P... )>
        static Delegate from_method( T* object_ptr )
        {
            Delegate d;
            d.m_ptr = { .m_ptr = object_ptr };
            d.m_stub = &stub_method<T, TMethod>;

            return d;
        }

        template<class T, R (T::*TMethod)( P... ) const>
        static Delegate from_method( T* object_ptr )
        {
            Delegate d;
            d.m_ptr = { .m_ptr = object_ptr };
            d.m_stub = &stub_method<T, TMethod>;

            return d;
        }

        static Delegate from_function( func_t func )
        {
            Delegate d;
            d.m_ptr = { .m_func = func };
            d.m_stub = &stub_function;

            return d;
        }

        operator zp_bool_t() const
        {
            return m_stub != nullptr;
        }

        R operator()( P ... args )
        {
            return ( m_stub )( m_ptr, args... );
        }

        R operator()( P ... args ) const
        {
            return ( m_stub )( m_ptr, args... );
        }

    private:
        union Callable
        {
            void* m_ptr;
            func_t m_func;
        };

        typedef R (* stub_t)( const Callable&, P... );

        Callable m_ptr;
        stub_t m_stub;

        static R stub_function( const Callable& ptr, P... args )
        {
            return ptr.m_func( args... );
        }

        template<class T, R (T::*TMethod)( P... )>
        static R stub_method( const Callable& ptr, P... args )
        {
            T* t = static_cast<T*>( ptr.m_ptr );
            return ( t->*TMethod )( args... );
        }
    };

#if 0
    template<typename R, typename ... P>
    class Function
    {
    public:

        typedef R (* func_t)( P ... );

        Function() = default;

        ~Function() = default;

        explicit Function( func_t func )
            : m_func( func )
        {
        }

        Function& operator=( func_t func )
        {
            m_func = func;
            return *this;
        }

        R operator()( P ... args )
        {
            if( m_func )
            {
                return m_func( args... );
            }
            else
            {
                return R();
            }
        }

    private:
        func_t m_func;
    };


    template<auto A>
    class Method;

    template<typename T, typename R, typename ... P, R (T::*M)( P... )>
    class Method<M>
    {
    public:
        typedef T* ptr_t;

        explicit Method( ptr_t ptr )
            : m_ptr( ptr )
        {
        }

        R operator()( P ... args )
        {
            if( m_ptr )
            {
                return ( m_ptr->*M )( args... );
            }
            else
            {
                return R();
            }
        }

    private:
        Method()
            : m_ptr( nullptr )
        {
        }

        ptr_t m_ptr;
    };

    template<typename T, typename R, typename ... P, R (T::*M)( P... ) const>
    class Method<M>
    {
    public:
        typedef T* ptr_t;

        explicit Method( ptr_t ptr )
            : m_ptr( ptr )
        {
        }

        R operator()( P ... args ) const
        {
            if( m_ptr )
            {
                return ( m_ptr->*M )( args... );
            }
            else
            {
                return R();
            }
        }


    private:
        Method()
            : m_ptr( nullptr )
        {
        }

        ptr_t m_ptr;
    };


    struct IThing
    {
        virtual void Meth( int a )
        {
        }
    };

    struct SSSS : public IThing
    {
        float Met( char a, int b )
        {
            return fgf;
        }

        void Meth( int a ) override
        {

        }

        [[nodiscard]] float TestConst( char* a, int b ) const
        {
            return fgf;
        }

        int TestMeth( int a ) const
        {
            return fgf;
        }

        float fgf;
    };

    int TestFunc( int a )
    {
        return a;
    }

    void TestAct( int a )
    {
    }

    template<typename T>
    struct Container
    {
        T m_val[2];
    };

    void afaf()
    {
        Container<Function<int, int>> fffffff {};
        fffffff.m_val[ 0 ] = TestFunc;

        Function<int, int> aa = Function( TestFunc );
        aa( 0 );

        Function ff( TestFunc );
        auto r = ff( 0 );

        Function vv( TestAct );
        vv( 0 );


        Function rr = Function( TestFunc );
        auto b = rr( 1 );

        SSSS ss, gg;
        Method mmm = Method<&SSSS::Met>( &gg );
        auto rm = mmm( 0, 1 );

        auto ccc = Method<&SSSS::TestConst>( &ss );
        auto cr = ccc( nullptr, 2 );

        Container<Delegate<void, int>> fafefa;
        fafefa.m_val[ 0 ] = Delegate < void, int > ::from_function( TestAct );
        fafefa.m_val[ 1 ] = Delegate < void, int > ::from_method<SSSS, &SSSS::Meth>( &ss );

        fafefa.m_val[ 0 ]( 0 );
    }
#endif
}

#endif //ZP_FUNCTION_H
