//
// Created by phosg on 2/25/2025.
//

#ifndef ZP_TEST_H
#define ZP_TEST_H

#include "Core/Macros.h"
#include "Core/Types.h"

//
#define ZP_TEST_GROUP( group ) namespace TestGroup##group

//
#define ZP_TEST_SUITE( suite ) namespace TestSuite##suite

//
#define ZP_TEST( name )                         \
class Test##name : public zp::Test              \
{                                               \
public:                                         \
    Test##name() : Test() {}                    \
    void Run( zp::ITestResult* const __result ) final; \
    void Setup( zp::ITestResult* const __result ) final; \
    void Teardown( zp::ITestResult* const __result ) final; \
};                                              \
Test##name g_Test##name;                        \
void Test##name::Run( zp::ITestResult* const __result )

//
#define ZP_TEST_ARGS( name, args )              \
class Test##name final : public zp::Test        \
{                                               \
public:                                         \
    Test##name() : Test() {}                    \
    void Run( zp::ITestResult* const __ result ) final; \
    void Setup( zp::ITestResult* const __result ) final; \
    void Teardown( zp::ITestResult* const __result ) final; \
private:                                        \
    struct args m_args;                         \
};                                              \
Test##name g_Test##name;                        \
void Test##name::Run( zp::ITestResult* const __result )

//
#define ZP_TEST_SETUP( name )                   \
void Test##name::Setup( zp::ITestResult* const __result )

//
#define ZP_TEST_TEARDOWN( name )                \
void Test##name::Teardown( zp::ITestResult* const __result )

//
//
//

#define ZP_CHECK_EQUALS_OP_MSG( lh, rh, op, msg )                               \
do                                                                              \
{                                                                               \
    if( op( (lh), (rh) ) )                                                      \
    {                                                                           \
        __result->Pass();                                                         \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        __result->Fail( #op "( " #lh ", " #rh ")", msg, __FILE__, __LINE__ ); \
    }                                                                           \
} while( false )

#define ZP_CHECK_EQUALS_OP( lh, rh, op )    ZP_CHECK_EQUALS_OP_MSG( lh, rh, op, "" )

#define ZP_CHECK_EQUALS_MSG( lh, rh, msg )                           \
do                                                                   \
{                                                                    \
    if( (lh) == (rh) )                                               \
    {                                                                \
        __result->Pass();                                              \
    }                                                                \
    else                                                             \
    {                                                                \
        __result->Fail( #lh " == " #rh, msg, __FILE__, __LINE__ ); \
    }                                                                \
} while( false )

#define ZP_CHECK_EQUALS( lh, rh )           ZP_CHECK_EQUALS_MSG( lh, rh, "" )

#define ZP_CHECK_NOT_EQUALS_MSG( lh, rh, msg )                       \
do                                                                   \
{                                                                    \
    if( (lh) != (rh) )                                               \
    {                                                                \
        __result->Pass();                                              \
    }                                                                \
    else                                                             \
    {                                                                \
        __result->Fail( #lh " != " #rh, msg, __FILE__, __LINE__ ); \
    }                                                                \
} while( false )

#define ZP_CHECK_NOT_EQUALS(  lh, rh )      ZP_CHECK_NOT_EQUALS_MSG( lh, rh, "" )

#define ZP_CHECK_APPROX_MSG( lh, rh, ep, msg )                       \
do                                                                   \
{                                                                    \
    const zp_float32_t flh = static_cast<zp_float32_t>( lh );        \
    const zp_float32_t frh = static_cast<zp_float32_t>( rh );        \
    const zp_float32_t fep = static_cast<zp_float32_t>( ep );        \
    const zp_float32_t fd = flh - frh;                               \
    if( fd >= -fep && fd <= fep )                                    \
    {                                                                \
        __result->Pass();                                              \
    }                                                                \
    else                                                             \
    {                                                                \
        __result->Fail( #lh " ~ " #rh " (" #ep ")", msg, __FILE__, __LINE__ ); \
    }                                                                \
} while( false )

#define ZP_CHECK_APPROX( lh, rh, ep )       ZP_CHECK_APPROX_MSG( lh, rh, ep, "" )

//
//
//

namespace zp
{
    class Test;

    ZP_PURE_INTERFACE ITestResult
    {
    public:
        virtual void Pass() = 0;

        virtual void Fail( const char* operation, const char* reason, const char* file, zp_uint32_t line ) = 0;
    };

    class TestFilter
    {};

    class TestResults : public ITestResult
    {
    public:
        void Pass() final;

        void Fail( const char* operation, const char* reason, const char* file, zp_uint32_t line ) final;

        void StartRun();

        void EndRun();

        void StartTest();

        void EndTest();
    };

    class TestRunner final
    {
        ZP_NONCOPYABLE(TestRunner);
        ZP_NONMOVABLE(TestRunner);
        ZP_STATIC_CLASS(TestRunner);

    public:
        static void Add( Test* test );

        static void RunAll( TestResults& testResults );
    };

    class Test
    {
    public:
        Test();

        virtual void Run( ITestResult* result ) = 0;

        virtual void Setup( ITestResult* result )
        {
        };

        virtual void Teardown( ITestResult* result )
        {
        };

    private:
        Test* m_next;
        Test* m_prev;

        friend class TestRunner;
    };
}

#endif //ZP_TEST_H
