//
// Created by phosg on 2/25/2025.
//

#include "Test/Test.h"
#include "Core/Macros.h"
#include "Core/Math.h"

using namespace zp;

ZP_TEST_GROUP( Math )
{
    ZP_TEST_SUITE( Vector4f )
    {
        ZP_TEST( Length )
        {
            const Vector4f a { 1, 1, 1, 1 };
            const zp_float32_t len = Math::Length( a );

            ZP_CHECK_EQUALS( len, 1 );
            ZP_CHECK_APPROX( len, 1, 0.000001f );
        };
    };
};

namespace zp
{
    Test::Test()
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
            Test* m_firstTest;
        };

        TestRunnerContext s_context {};
    }

    void TestRunner::Add( Test* test )
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
            SetupTeardownFinallyBlock( Test* test, TestResults& testResults )
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

            Test* test;
            ITestResult* results;
        };

        testResults.StartRun();

        for( Test* test = s_context.m_firstTest; test != s_context.m_firstTest; test = test->m_next )
        {
            testResults.StartTest();

            try
            {
                const SetupTeardownFinallyBlock setupTeardown( test, testResults );

                test->Run( &testResults );
            }
            catch( ... )
            {
            };

            testResults.EndTest();
        }

        testResults.EndRun();
    }

    //
    //
    //

    void TestResults::Pass()
    {

    }

    void TestResults::Fail( const char* operation, const char* reason, const char* file, zp_uint32_t line )
    {

    }

    void TestResults::StartRun()
    {

    }

    void TestResults::EndRun()
    {

    }

    void TestResults::StartTest()
    {

    }

    void TestResults::EndTest()
    {

    }
}
