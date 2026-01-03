//
// Created by phosg on 2/25/2025.
//

#include "Test/Test.h"

#include "Core/Log.h"
#include "Core/Macros.h"
#include "Core/Math.h"

#include "Platform/Platform.h"

using namespace zp;

namespace zp
{
    ITest::ITest()
        : m_next( nullptr )
        , m_prev( nullptr )
    {
        TestRunner::Add( this );
    }

    //
    //
    //

    namespace
    {
        struct TestRunnerContext
        {
            ITest* m_firstTest;
        };

        TestRunnerContext s_context {};
    }

    void TestRunner::Add( ITest* test )
    {
        if( s_context.m_firstTest == nullptr )
        {
            s_context.m_firstTest = test;
            s_context.m_firstTest->m_next = test;
            s_context.m_firstTest->m_prev = test;
        }
        else
        {
            test->m_next = s_context.m_firstTest;
            test->m_prev = s_context.m_firstTest->m_prev;
            test->m_prev->m_next = test;
            test->m_next->m_prev = test;
        }
    }

    void TestRunner::RunAll( TestResults& testResults )
    {
        struct SetupTeardownFinallyBlock
        {
            SetupTeardownFinallyBlock( ITest* test, TestResults& testResults )
                : test( test )
                , results( &testResults )
            {
                test->Setup( results );
            }

            ~SetupTeardownFinallyBlock()
            {
                try
                {
                    test->Teardown( results );
                }
                catch( ... )
                {
                }
            }

            ITest* test;
            ITestResults* results;
        };

        testResults.StartRun();

        const zp_time_t startTime = Platform::TimeNow();

        ITest* currentTest = s_context.m_firstTest;
        if( currentTest != nullptr )
        {
            do
            {
                const zp_time_t startTestTime = Platform::TimeNow();

                testResults.StartTest();

                try
                {
                    const SetupTeardownFinallyBlock setupTeardown( currentTest, testResults );

                    currentTest->Run( &testResults );
                }
                catch( ... )
                {
                };

                testResults.EndTest();

                const zp_time_t endTestTime = Platform::TimeNow();

                currentTest = currentTest->m_next;
            } while( currentTest != s_context.m_firstTest );
        }

        testResults.EndRun();
    }

    //
    //
    //

    TestResults::TestResults( MemoryLabel memoryLabel )
        : m_results( 8, memoryLabel )
        , m_numTests( 0 )
        , m_numPassed( 0 )
        , m_numFailed( 0 )
    {
    }

    void TestResults::Pass( const TestResultDesc& result )
    {
        ++m_numPassed;
        zp_printfln( ZP_CC_B( GREEN, DEFAULT ) "[PASS]" ZP_CC_RESET " %s %s:%d %s", result.testName, result.file, result.line, result.operation );
    }

    void TestResults::Fail( const TestResultDesc& result )
    {
        ++m_numFailed;
        if( result.reason )
        {
            zp_printfln( ZP_CC_B( RED, DEFAULT ) "[FAIL]" ZP_CC_RESET " %s %s:%d %s - %s", result.testName, result.file, result.line, result.operation, result.reason );
        }
        else
        {
            zp_printfln( ZP_CC_B( RED, DEFAULT ) "[FAIL]" ZP_CC_RESET " %s %s:%d %s", result.testName, result.file, result.line, result.operation );
        }
    }

    void TestResults::StartRun()
    {
        m_numTests = 0;
        m_numPassed = 0;
        m_numFailed = 0;
        m_startTime = Platform::TimeNow();
    }

    void TestResults::EndRun()
    {
        const zp_time_t endTime = Platform::TimeNow();

        MutableFixedString256 log;
        log.append( "[Test Run Complete]" ).appendLine();
        log.append( "  Passed: " ).appendFormat( "%d", m_numPassed ).appendLine();
        log.append( "  Failed: " ).appendFormat( "%d", m_numFailed );

        zp_printfln( log.c_str() );
    }

    void TestResults::StartTest()
    {
        ++m_numTests;
    }

    void TestResults::EndTest()
    {

    }
}
