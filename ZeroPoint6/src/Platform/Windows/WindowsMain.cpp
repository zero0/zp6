
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

//import Platform;
//import Core;

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

#include "Platform/Platform.h"

#include "Rendering/RenderingEngine.h"

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
            zp::ResizeRenderingEngine( 0, 0 );
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

int APIENTRY OldWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    HINSTANCE h = ::LoadLibrary( TEXT( "../ShadowGame/ZeroPoint_Game.dll" ) );

    //if (h)
    //{
    //    auto ptr = (zp::GetEngineEntryPoint)::GetProcAddress(h, TEXT(ZP_STR(GetEngineEntryPoint)));
    //    if (ptr)
    //    {
    //        const zp::EngineEntryPointAPI* api = ptr();
    //    }
    //}

    WNDCLASSEX wc;
    wc.cbSize = sizeof( WNDCLASSEX );
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.lpfnWndProc = WinProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject( DKGRAY_BRUSH ));
    wc.hIcon = LoadIcon( nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon( nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor( nullptr, IDC_ARROW);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = kZeroPointClassName;

    ATOM reg = ::RegisterClassEx( &wc );
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
    DWORD exStyle = 0;
#if ZP_DEBUG
    exStyle |= WS_EX_ACCEPTFILES;
#endif

    RECT r = { 0, 0, 800, 600 };
    ::AdjustWindowRectEx( &r, style, false, exStyle );

    int width = r.right - r.left;
    int height = r.bottom - r.top;

    LPCSTR title = "ZeroPoint6";
    HWND hWnd = ::CreateWindowEx(
        exStyle,
        kZeroPointClassName,
        title,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    //   zp::GetPlatform()->SetWindowHandle(hWnd);
//    ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(zp::GetPlatform()));

    ::ShowWindow( hWnd, SW_SHOW );
    ::UpdateWindow( hWnd );

    zp::InitializeRenderingEngine( hInstance, hWnd, width, height );
    zp_printf( "hello world" );
    int exitCode = 0;

    MSG msg;
    bool isRunning = true;
    while( isRunning )
    {
        if( ::PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ))
        {
            if( msg.message == WM_QUIT )
            {
                exitCode = static_cast<int>(msg.wParam);
                isRunning = false;
            }

            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );
        }

        if( isRunning )
        {
            zp::RenderFrame();
        }
    }

    zp::DestroyRenderingEngine();

    ::DestroyWindow( hWnd );
    ::UnregisterClass( kZeroPointClassName, hInstance );

    // zp::GetPlatform()->SetWindowHandle(ZP_NULL_HANDLE);

    if( h )
    {
        ::FreeLibrary( h );
    }

    return exitCode;
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    using namespace zp;

    MemoryAllocator<SystemMemoryStorage, SystemPageAllocatorPolicy> s_defaultAllocator(
        SystemMemoryStorage( nullptr, 10 MB ),
        SystemPageAllocatorPolicy()
    );

    MemoryAllocator<NullMemoryStorage, SimpleAllocatorPolicy> s_simpleAllocator(
        NullMemoryStorage( 0 ),
        SimpleAllocatorPolicy()
    );

    RegisterAllocator( MemoryLabels::Default, &s_simpleAllocator );
    RegisterAllocator( MemoryLabels::String, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Graphics, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::FileIO, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Buffer, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::User, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Data, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Temp, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::ThreadSafe, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Profiler, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Debug, &s_defaultAllocator );

    Engine* engine = ZP_NEW( Default, Engine );
    engine->initialize();

    engine->startEngine();

    engine->destroy();
    int exitCode = engine->getExitCode();

    ZP_FREE( Default, engine );

    return exitCode;
}
