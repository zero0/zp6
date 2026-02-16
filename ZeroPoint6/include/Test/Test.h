//
// Created by phosg on 2/25/2025.
//

#ifndef ZP_TEST_H
#define ZP_TEST_H

#include "Core/Macros.h"
#include "Core/String.h"
#include "Core/Types.h"
#include "Core/Vector.h"

//
#define ZP_TEST_GROUP( group ) namespace TestGroup##group

//
#define ZP_TEST_SUITE( suite ) namespace TestSuite##suite

//
#define ZP_TEST_WITH_SETUP( name )              \
class Test##name : public zp::ITest             \
{                                               \
public:                                         \
    Test##name() : ITest() {}                    \
    void Run( zp::ITestResults* const __result ) final; \
    void Setup( zp::ITestResults* const __result ) final; \
    void Teardown( zp::ITestResults* const __result ) final; \
};                                              \
Test##name g_Test##name;                        \
void Test##name::Run( zp::ITestResults* const __result )

//
#define ZP_TEST( name )                         \
class Test##name : public zp::ITest              \
{                                               \
public:                                         \
    Test##name() : ITest() {}                    \
    void Run( zp::ITestResults* const __result ) final; \
    void Setup( zp::ITestResults* const __result ) final {}; \
    void Teardown( zp::ITestResults* const __result ) final {}; \
};                                              \
Test##name g_Test##name;                        \
void Test##name::Run( zp::ITestResults* const __result )

//
#define ZP_TEST_WITH_ARGS( name, args )              \
class Test##name final : public zp::Test        \
{                                               \
public:                                         \
    Test##name() : ITest() {}                    \
    void Run( zp::ITestResults* const __ result ) final; \
    void Setup( zp::ITestResults* const __result ) final {}; \
    void Teardown( zp::ITestResults* const __result ) final {}; \
private:                                        \
    struct args m_args;                         \
};                                              \
Test##name g_Test##name;                        \
void Test##name::Run( zp::ITestResults* const __result )

//
#define ZP_TEST_WITH_SETUP_ARGS( name, args )              \
class Test##name final : public zp::Test        \
{                                               \
public:                                         \
    Test##name() : ITest() {}                    \
    void Run( zp::ITestResults* const __ result ) final; \
    void Setup( zp::ITestResults* const __result ) final; \
    void Teardown( zp::ITestResults* const __result ) final; \
private:                                        \
    struct args m_args;                         \
};                                              \
Test##name g_Test##name;                        \
void Test##name::Run( zp::ITestResults* const __result )

//
#define ZP_TEST_SETUP( name )                   \
void Test##name::Setup( zp::ITestResults* const __result )

//
#define ZP_TEST_TEARDOWN( name )                \
void Test##name::Teardown( zp::ITestResults* const __result )

//
//
//

// @formatter:off
#define TEST_NAME  zp_type_name<decltype(this)>()

#define TEST_RESULT_TEMPLATE( t, tn, op, r, f, l )  \
do                                                  \
{                                                   \
    const TestResultDesc __desc {                   \
        .testName = (tn),                           \
        .operation = (op),                          \
        .reason = (r),                              \
        .file = (f),                                \
        .line = (l),                                \
    };                                              \
    if( (t) )                                       \
    {                                               \
        __result->Pass( __desc );                   \
    }                                               \
    else                                            \
    {                                               \
        __result->Fail( __desc );                   \
    }                                               \
} while( false )

#define ZP_CHECK_EQUALS_OP_REASON( lh, rh, op, reason )     TEST_RESULT_TEMPLATE( op( (lh), (rh) ), TEST_NAME, #op "( " #lh ", " #rh ")", reason, __FILE__, __LINE__ )
#define ZP_CHECK_EQUALS_OP( lh, rh, op )                    TEST_RESULT_TEMPLATE( op( (lh), (rh) ), TEST_NAME, #op "( " #lh ", " #rh ")", nullptr, __FILE__, __LINE__ )

#define ZP_CHECK_EQUALS_REASON( lh, rh, reason )            TEST_RESULT_TEMPLATE( (lh) == (rh), TEST_NAME, #lh " == " #rh, reason, __FILE__, __LINE__ )
#define ZP_CHECK_EQUALS( lh, rh)                            TEST_RESULT_TEMPLATE( (lh) == (rh), TEST_NAME, #lh " == " #rh, nullptr, __FILE__, __LINE__ )

#define ZP_CHECK_NOT_EQUALS_REASON( lh, rh, reason )        TEST_RESULT_TEMPLATE( (lh) != (rh), TEST_NAME, #lh " != " #rh, reason, __FILE__, __LINE__ )
#define ZP_CHECK_NOT_EQUALS( lh, rh )                       TEST_RESULT_TEMPLATE( (lh) != (rh), TEST_NAME, #lh " != " #rh, nullptr, __FILE__, __LINE__ )

#define ZP_CHECK_FLOAT32_APPROX_REASON( lh, rh, ep, reason )         \
do                                                                   \
{                                                                    \
    const zp_float32_t flh = static_cast<zp_float32_t>( lh );        \
    const zp_float32_t frh = static_cast<zp_float32_t>( rh );        \
    const zp_float32_t fep = static_cast<zp_float32_t>( ep );        \
    const zp_float32_t fdiff = flh - frh;                            \
    TEST_RESULT_TEMPLATE( fdiff >= -fep && fdiff <= fep, TEST_NAME, #lh " ~= " #rh " (" #ep ")", reason, __FILE__, __LINE__ );\
} while( false )
#define ZP_CHECK_FLOAT32_APPROX( lh, rh, ep )               ZP_CHECK_FLOAT32_APPROX_REASON( lh, rh, ep, nullptr )
// @formatter:on



//
//
//

namespace zp
{
    class ITest;

    struct TestResultDesc
    {
        const char* testName;
        const char* operation;
        const char* reason;
        const char* file;
        const zp_uint32_t line;
    };

    ZP_PURE_INTERFACE ITestResults
    {
    public:
        virtual ~ITestResults() = default;

        virtual void Pass( const TestResultDesc& result ) = 0;

        virtual void Fail( const TestResultDesc& result ) = 0;
    };

    class TestFilter
    {
    };

    enum class TestResultStatus
    {
        Unknown,
        Passed,
        Failed,
        Error,
    };

    struct TestResult
    {
        MutableFixedString1024 msg;
        ITest* test;
        zp_time_t duration;
        TestResultStatus status;
    };

    class TestResults : public ITestResults
    {
    public:
        explicit TestResults( MemoryLabel memoryLabel );

        void Pass( const TestResultDesc& result ) final;

        void Fail( const TestResultDesc& result ) final;

        void StartRun();

        void EndRun();

        void StartTest();

        void EndTest();

    private:
        Vector<TestResult> m_results;
        zp_size_t m_numTests;
        zp_size_t m_numPassed;
        zp_size_t m_numFailed;
        zp_time_t m_startTime;
    };

    namespace TestRunner
    {
        void Add( ITest* test );

        void RunAll( TestResults& testResults );
    };

    class ITest
    {
    public:
        ITest();
        virtual ~ITest() = default;

        virtual void Run( ITestResults* result ) = 0;

        virtual void Setup( ITestResults* result ) = 0;

        virtual void Teardown( ITestResults* result ) = 0;

       IntrusiveListNode<ITest> node;
    };
}

#endif //ZP_TEST_H
