
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

//import Platform;
//import Core;

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

#include "Platform/Platform.h"

#include "Rendering/RenderSystem.h"

#include "Engine/MemoryLabels.h"
#include "Engine/Engine.h"

static LPCSTR kZeroPointClassName = "ZeroPoint::WindowClass";

static LRESULT CALLBACK WinProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    switch( uMessage )
    {
        case WM_CLOSE:
        {
            ::PostQuitMessage( 0 );
        }
            break;

        case WM_GETMINMAXINFO:
        {
            auto lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);

            POINT p = { 300, 300 };
            lpMMI->ptMinTrackSize = p;
        }
            break;

        case WM_SIZE:
        {
            //zp::ResizeRenderingEngine( 0, 0 );
        }
            break;

        case WM_KILLFOCUS:
        {
        }
            break;

        case WM_SETFOCUS:
        {
        }
            break;

        case WM_KEYDOWN:
        {
        }
            break;

        case WM_KEYUP:
        {
        }
            break;

        case WM_MOUSEMOVE:
        {
            POINTS p = MAKEPOINTS( lParam );
        }
            break;

        case WM_MOUSEWHEEL:
        {
        }
            break;

        case WM_LBUTTONDOWN:
        {
        }
            break;

        case WM_LBUTTONUP:
        {
        }
            break;

        case WM_RBUTTONDOWN:
        {
        }
            break;

        case WM_RBUTTONUP:
        {
        }
            break;

        case WM_MBUTTONDOWN:
        {
        }
            break;

        case WM_MBUTTONUP:
        {
        }
            break;

        default:
            return ::DefWindowProc( hWnd, uMessage, wParam, lParam );
    }

    return 0;
}

struct MemoryConfig
{
    zp_size_t defaultAllocatorPageSize;
    zp_size_t tempAllocatorPageSize;
    zp_size_t profilerPageSize;
    zp_size_t debugPageSize;
};

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    using namespace zp;

    MemoryConfig memoryConfig {
        .defaultAllocatorPageSize = 16 MB,
        .tempAllocatorPageSize = 2 MB,
        .profilerPageSize = 16 MB,
        .debugPageSize = 1 MB,
    };

#if ZP_DEBUG
    void* const baseAddress = reinterpret_cast<void*>(0x10000000);
#else
    void* const baseAddress = nullptr;
#endif

    const zp_size_t totalMemorySize = 128 MB;
    void* systemMemory = GetPlatform()->AllocateSystemMemory( baseAddress, totalMemorySize );

    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_defaultAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, 0 ), memoryConfig.defaultAllocatorPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );

    zp_size_t endMemorySize = totalMemorySize;

    const zp_size_t tempMemorySize = 16 MB;
    endMemorySize -= tempMemorySize;
    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_tempAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), memoryConfig.tempAllocatorPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );

#if ZP_USE_PROFILER
    const zp_size_t profilerMemorySize = 8 MB;
    endMemorySize -= profilerMemorySize;
    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_profilingAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), memoryConfig.profilerPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );
#else
    MemoryAllocator<NullMemoryStorage, NullAllocationPolicy, NullMemoryLock> s_profilingAllocator(
        NullMemoryStorage,
        NullAllocationPolicy,
        NullMemoryLock
    );
#endif

#if ZP_DEBUG
    const zp_size_t debugMemorySize = 8 MB;
    endMemorySize -= debugMemorySize;
    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_debugAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), memoryConfig.debugPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );
#else
    MemoryAllocator<NullMemoryStorage, NullAllocationPolicy, NullMemoryLock> s_debugAllocator(
        NullMemoryStorage,
        NullAllocationPolicy,
        NullMemoryLock
    );
#endif

    RegisterAllocator( MemoryLabels::Default, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::String, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Graphics, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::FileIO, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Buffer, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::User, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Data, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Temp, &s_tempAllocator );
    RegisterAllocator( MemoryLabels::ThreadSafe, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Profiling, &s_profilingAllocator );
    RegisterAllocator( MemoryLabels::Debug, &s_debugAllocator );

    auto engine = ZP_NEW( Default, Engine );
    {
        engine->initialize();

        engine->startEngine();

        do
        {
            engine->process();

            //zp_yield_current_thread();
        } while( engine->isRunning() );

        engine->stopEngine();

        engine->destroy();
    }

    const zp_int32_t exitCode = engine->getExitCode();

    ZP_FREE( Default, engine );

    return exitCode;
}
