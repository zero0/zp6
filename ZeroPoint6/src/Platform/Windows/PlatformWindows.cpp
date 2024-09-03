//
// Created by phosg on 11/10/2021.
//

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT _WIN32_WINNT_WIN10

#include <windows.h>
#include <uxtheme.h>
#include <winuser.h>
#include <processthreadsapi.h>
#include <excpt.h>
#include <winsock2.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <time.h>
#include <sys/time.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dwmapi.dll")
#pragma comment(lib, "uxtheme.lib")

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Math.h"
#include "Core/Common.h"
#include "Core/Threading.h"
#include "Core/Version.h"
#include "Core/Allocator.h"
#include "Platform/Platform.h"

#define TestFlag( a, f, t, r )      a |= ((f) & (t)) ? (r) : 0

using namespace zp;

ZP_STATIC_ASSERT( sizeof( CriticalSection ) >= sizeof( CRITICAL_SECTION ) );

CriticalSection::CriticalSection()
    : m_memory()
{
    LPCRITICAL_SECTION ptr = reinterpret_cast<LPCRITICAL_SECTION>( m_memory );
    ::InitializeCriticalSection( ptr );
}

CriticalSection::~CriticalSection()
{
    LPCRITICAL_SECTION ptr = reinterpret_cast<LPCRITICAL_SECTION>( m_memory );
    ::DeleteCriticalSection( ptr );
}

CriticalSection::CriticalSection( const CriticalSection& other )
    : m_memory()
{
    LPCRITICAL_SECTION ptr = reinterpret_cast<LPCRITICAL_SECTION>( m_memory );
    ::InitializeCriticalSection( ptr );
}

CriticalSection::CriticalSection( CriticalSection&& other ) noexcept
    : m_memory()
{
    LPCRITICAL_SECTION ptr = reinterpret_cast<LPCRITICAL_SECTION>( m_memory );
    LPCRITICAL_SECTION otherPtr = reinterpret_cast<LPCRITICAL_SECTION>( other.m_memory );

    *ptr = *otherPtr;

    zp_zero_memory_array( other.m_memory );
}

void CriticalSection::enter()
{
    LPCRITICAL_SECTION ptr = reinterpret_cast<LPCRITICAL_SECTION>( m_memory );
    ::EnterCriticalSection( ptr );
}

void CriticalSection::leave()
{
    LPCRITICAL_SECTION ptr = reinterpret_cast<LPCRITICAL_SECTION>( m_memory );
    ::LeaveCriticalSection( ptr );
}

//
//
//
#define USE_CUSTOM_TOOLBAR_HEADER   0
namespace
{
    const char* kZeroPointClassName = "ZeroPoint::WindowClass";
    const char* kZeroPointSystemTrayClassName = "ZeroPoint::SystemTrayClass";

    enum
    {
        kMaxWindows = 4
    };

    zp_size_t s_windowCount {};
    HWND s_windows[kMaxWindows] {};

    LRESULT CALLBACK WinProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
    {
        auto windowCallbacks = reinterpret_cast<WindowCallbacks*>(::GetWindowLongPtr( hWnd, GWLP_USERDATA ));

        switch( uMessage )
        {
            case WM_CREATE:
            {
                // track created window
                ZP_ASSERT( s_windowCount < kMaxWindows );
                s_windows[ s_windowCount++ ] = hWnd;
#if USE_CUSTOM_TOOLBAR_HEADER
                RECT rect;
                ::GetWindowRect(hWnd, &rect);

                ::SetWindowPos(hWnd, nullptr,
                    rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
#endif
            }
                break;

            case WM_CLOSE:
            {
                if( windowCallbacks && windowCallbacks->onWindowClosed )
                {
                    windowCallbacks->onWindowClosed( hWnd, windowCallbacks->userPtr );
                }

                ::DestroyWindow( hWnd );
            }
                break;

            case WM_DESTROY:
            {
                // remove tracked windows
                for( zp_size_t i = 0; i < s_windowCount; ++i )
                {
                    if( s_windows[ i ] == hWnd )
                    {
                        --s_windowCount;
                        s_windows[ i ] = s_windows[ s_windowCount ];
                        s_windows[ s_windowCount ] = nullptr;
                        --i;
                    }
                }

                // if there are no more windows, quit app
                if( s_windowCount == 0 )
                {
                    ::PostQuitMessage( 0 );
                }
            }
                break;

#if USE_CUSTOM_TOOLBAR_HEADER
                case WM_NCCALCSIZE:
                {
                    if( !wParam )
                    {
                        return ::DefWindowProc( hWnd, uMessage, wParam, lParam );
                    }

                    zp_uint32_t dpi = ::GetDpiForWindow(hWnd);

                    zp_int32_t frameX = ::GetSystemMetricsForDpi(SM_CXFRAME, dpi);
                    zp_int32_t frameY = ::GetSystemMetricsForDpi(SM_CYFRAME, dpi);
                    zp_int32_t padding = ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

                    auto lpNCCalcSizeParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                    lpNCCalcSizeParams->rgrc[0].right -= frameX + padding;
                    lpNCCalcSizeParams->rgrc[0].left += frameX + padding;
                    lpNCCalcSizeParams->rgrc[0].bottom -= frameY + padding;
                }
                    break;
#endif
            case WM_GETMINMAXINFO:
            {
                if( windowCallbacks )
                {
                    zp_int32_t minWidth;
                    zp_int32_t minHeight;
                    zp_int32_t maxWidth;
                    zp_int32_t maxHeight;
                    if( windowCallbacks->onWindowGetMinMaxSize )
                    {
                        windowCallbacks->onWindowGetMinMaxSize( hWnd, minWidth, minHeight, maxWidth, maxHeight, windowCallbacks->userPtr );
                    }
                    else
                    {
                        minWidth = windowCallbacks->minWidth;
                        minHeight = windowCallbacks->minHeight;
                        maxWidth = windowCallbacks->maxWidth;
                        maxHeight = windowCallbacks->maxHeight;
                    }

                    auto lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
                    lpMMI->ptMinTrackSize = { .x = minWidth, .y = minHeight };
                    lpMMI->ptMaxSize = { .x = maxWidth, .y = maxHeight };
                    lpMMI->ptMaxTrackSize = { .x = maxWidth, .y = maxHeight };
                }
            }
                break;

            case WM_WINDOWPOSCHANGED:
            {
                if( windowCallbacks && windowCallbacks->onWindowResize )
                {
                    auto pos = reinterpret_cast<LPWINDOWPOS >( lParam );
                    if( ( ~pos->flags & SWP_NOSIZE ) == SWP_NOSIZE )
                    {
                        const DWORD style = ::GetWindowLong( hWnd, GWL_STYLE );
                        const DWORD exStyle = ::GetWindowLong( hWnd, GWL_EXSTYLE );

                        RECT rc;
                        ::SetRectEmpty( &rc );
                        ::AdjustWindowRectEx( &rc, style, false, exStyle );

                        const zp_int32_t width = pos->cx - ( rc.right - rc.left );
                        const zp_int32_t height = pos->cy - ( rc.bottom - rc.top );

                        windowCallbacks->onWindowResize( hWnd, width, height, windowCallbacks->userPtr );
                    }
                }
            }
                break;

            case WM_KILLFOCUS:
            {
                if( windowCallbacks && windowCallbacks->onWindowFocus )
                {
                    windowCallbacks->onWindowFocus( hWnd, false, windowCallbacks->userPtr );
                }
            }
                break;

            case WM_SETFOCUS:
            {
                if( windowCallbacks && windowCallbacks->onWindowFocus )
                {
                    windowCallbacks->onWindowFocus( hWnd, true, windowCallbacks->userPtr );
                }
            }
                break;

            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            {
                if( windowCallbacks && windowCallbacks->onWindowKeyEvent )
                {
                    WORD vkCode = LOWORD( wParam );                                 // virtual-key code

                    WORD keyFlags = HIWORD( lParam );

                    WORD scanCode = LOBYTE( keyFlags );                             // scan code
                    BOOL isExtendedKey = ( keyFlags & KF_EXTENDED ) == KF_EXTENDED; // extended-key flag, 1 if scancode has 0xE0 prefix

                    if( isExtendedKey )
                    {
                        scanCode = MAKEWORD( scanCode, 0xE0 );
                    }

                    zp_bool_t wasKeyDown = ( keyFlags & KF_REPEAT ) == KF_REPEAT;        // previous key-state flag, 1 on autorepeat
                    WORD repeatCount = LOWORD( lParam );                            // repeat count, > 0 if several keydown messages was combined into one message

                    zp_bool_t isKeyReleased = ( keyFlags & KF_UP ) == KF_UP;             // transition-state flag, 1 on keyup
                    zp_bool_t isAltKeyDown = ( keyFlags & KF_ALTDOWN ) == KF_ALTDOWN;
                    zp_bool_t isCtrlKeyDown = GetKeyState( VK_CONTROL ) & 0x8000;
                    zp_bool_t isShiftKeyDown = GetKeyState( VK_SHIFT ) & 0x8000;

                    // if we want to distinguish these keys:
                    switch( vkCode )
                    {
                        case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
                        case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
                        case VK_MENU:    // converts to VK_LMENU or VK_RMENU
                            vkCode = LOWORD( MapVirtualKey( scanCode, MAPVK_VSC_TO_VK_EX ) );
                            break;
                        default:
                            break;
                    }

                    const WindowKeyEvent event {
                        .keyCode = vkCode,
                        .repeatCount = repeatCount,
                        .isCtrlDown = isCtrlKeyDown,
                        .isShiftDown = isShiftKeyDown,
                        .isAltDown = isAltKeyDown,
                        .wasKeyDown = wasKeyDown,
                        .isKeyReleased = isKeyReleased,
                    };
                    windowCallbacks->onWindowKeyEvent( hWnd, event, windowCallbacks->userPtr );
                }
            }
                break;

            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            {
                if( windowCallbacks && windowCallbacks->onWindowMouseEvent )
                {
                    const POINTS p = MAKEPOINTS( lParam );
                    const SHORT zDelta = GET_WHEEL_DELTA_WPARAM( wParam );

                    const WORD fwKeys = GET_KEYSTATE_WPARAM( wParam );
                    const zp_bool_t isCtrlDown = ( MK_CONTROL & fwKeys ) == MK_CONTROL;
                    const zp_bool_t isShiftDown = ( MK_SHIFT & fwKeys ) == MK_SHIFT;

                    const WindowMouseEvent event {
                        .x = p.x,
                        .y = p.y,
                        .zDelta = zDelta,
                        .isCtrlDown = isCtrlDown,
                        .isShiftDown = isShiftDown,
                    };
                    windowCallbacks->onWindowMouseEvent( hWnd, event, windowCallbacks->userPtr );
                }
            }
                break;

            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDBLCLK:
            {
                POINTS p = MAKEPOINTS( lParam );

            }
                break;

            case WM_HELP:
            {
                if( windowCallbacks && windowCallbacks->onWindowHelpEvent )
                {
                    windowCallbacks->onWindowHelpEvent( hWnd, windowCallbacks->userPtr );
                }
            }
                break;

            case WM_COMMAND:
            {
                switch( LOWORD( wParam ) )
                {

                }
            }
                break;

            default:
                return ::DefWindowProc( hWnd, uMessage, wParam, lParam );
        }

        return 0;
    }

    void PrintLastErrorMsg( const char* msg )
    {
        const DWORD errorMessageID = ::GetLastError();
        if( errorMessageID )
        {
            LPSTR messageBuffer = nullptr;

            ::FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                errorMessageID,
                MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                reinterpret_cast<LPSTR>(&messageBuffer),
                0,
                nullptr );

            zp_error_printfln( "%s: [%d] %s", msg, errorMessageID, messageBuffer );

            ::LocalFree( messageBuffer );
        }
    }

    void PrintLastWSAErrorMsg( const char* msg )
    {
        const int errorMessageID = ::WSAGetLastError();
        if( errorMessageID )
        {
            LPSTR messageBuffer = nullptr;

            ::FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                errorMessageID,
                MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                reinterpret_cast<LPSTR>(&messageBuffer),
                0,
                nullptr );

            zp_error_printfln( "%s: [%d] %s", msg, errorMessageID, messageBuffer );

            ::LocalFree( messageBuffer );
        }
    }
}

namespace zp
{
    zp_char8_t Platform::PathSep = '\\';

    namespace
    {
        ATOM g_windowClassReg = 0;
    }

    WindowHandle Platform::OpenWindow( const OpenWindowDesc& desc )
    {
        HINSTANCE hInstance = desc.instanceHandle ? static_cast<HINSTANCE>( desc.instanceHandle ) : ::GetModuleHandle( nullptr );

        if( g_windowClassReg == 0 )
        {
            const WNDCLASSEX wc {
                .cbSize = sizeof( WNDCLASSEX ),
                .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                .lpfnWndProc = WinProc,
                .cbClsExtra = 0,
                .cbWndExtra = 0,
                .hInstance = hInstance,
                .hIcon = ::LoadIcon( hInstance, MAKEINTRESOURCE( 101 ) ),
                .hCursor = ::LoadCursor( hInstance, IDC_ARROW ),
                .hbrBackground = static_cast<HBRUSH>( ::GetStockObject( DKGRAY_BRUSH ) ),
                .lpszMenuName = nullptr,
                .lpszClassName = kZeroPointClassName,
                .hIconSm = static_cast<HICON>( ::LoadImage( hInstance, MAKEINTRESOURCE( 101 ), IMAGE_ICON, ::GetSystemMetrics( SM_CXSMICON ), ::GetSystemMetrics( SM_CYSMICON ), LR_DEFAULTCOLOR ) ),
            };

            g_windowClassReg = ::RegisterClassEx( &wc );
        }

        const DWORD style = WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
        DWORD exStyle = WS_EX_APPWINDOW;
#if ZP_DEBUG
        exStyle |= WS_EX_ACCEPTFILES;
#endif

        RECT r {
            .left = 0,
            .top = 0,
            .right =  desc.width,
            .bottom = desc.height
        };
        ::AdjustWindowRectEx( &r, style, false, exStyle );

        const int width = r.right - r.left;
        const int height = r.bottom - r.top;

        HWND hWnd = ::CreateWindowEx(
            exStyle,
            kZeroPointClassName,
            desc.title,
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
        ZP_ASSERT( hWnd );

        ::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)desc.callbacks );
/*
        // set dark mode by default
        const BOOL useDarkMode = true;
        if( FAILED( ::DwmSetWindowAttribute( hWnd, 20, &useDarkMode, sizeof( BOOL ) ) ) )
        {
            ::DwmSetWindowAttribute( hWnd, 19, &useDarkMode, sizeof( BOOL ) );
        }
*/
        ::ShowWindow( hWnd, desc.showWindow ? SW_SHOW : SW_HIDE );
        ::UpdateWindow( hWnd );

        return { .handle = hWnd };
    }

    zp_bool_t Platform::DispatchWindowMessages( WindowHandle windowHandle, zp_int32_t& exitCode )
    {
        zp_bool_t isClosed = false;

        HWND hWnd = static_cast<HWND>( windowHandle.handle );

        MSG msg {};
        while( ::PeekMessage( &msg, hWnd, 0, 0, PM_REMOVE ) )
        {
            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );

            if( !isClosed && msg.message == WM_CLOSE )
            {
                exitCode = static_cast<zp_int32_t>( msg.wParam );
                isClosed = true;
            }
        }

        return isClosed;
    }

    zp_bool_t Platform::DispatchMessages( zp_int32_t& exitCode )
    {
        zp_bool_t isRunning = true;

        MSG msg {};
        while( ::PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );

            if( isRunning && msg.message == WM_QUIT )
            {
                exitCode = static_cast<zp_int32_t>( msg.wParam );
                isRunning = false;
            }
        }

        return isRunning;
    }

    void Platform::ShowWindow( WindowHandle windowHandle, zp_bool_t show )
    {
        HWND hWnd = static_cast<HWND>( windowHandle.handle );
        if( ::IsWindow( hWnd ) )
        {
            ::ShowWindow( hWnd, show ? SW_SHOW : SW_HIDE );
            ::UpdateWindow( hWnd );
        }
    }

    void Platform::CloseWindow( WindowHandle windowHandle )
    {
        HWND hWnd = static_cast<HWND>( windowHandle.handle );
        if( ::IsWindow( hWnd ) )
        {
            ::CloseWindow( hWnd );
        }
    }

    void Platform::SetWindowTitle( WindowHandle windowHandle, const String& title )
    {
        HWND hWnd = static_cast<HWND>( windowHandle.handle );
        if( ::IsWindow( hWnd ) )
        {
            ::SetWindowText( hWnd, title.c_str() );
        }
    }

    void Platform::SetWindowSize( WindowHandle windowHandle, const zp_int32_t width, const zp_int32_t height )
    {
        HWND hWnd = static_cast<HWND>( windowHandle.handle );
        if( ::IsWindow( hWnd ) )
        {
            const LONG style = ::GetWindowLong( hWnd, GWL_STYLE );
            const LONG exStyle = ::GetWindowLong( hWnd, GWL_EXSTYLE );

            RECT r { .left = 0, .top = 0, .right = width, .bottom = height };
            ::AdjustWindowRectEx( &r, style, false, exStyle );

            ::SetWindowPos( hWnd, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER );
        }
    }

    LRESULT CALLBACK SystemTrayProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
    {
        switch( uMessage )
        {
            case WM_CLOSE:
                break;

            case WM_DESTROY:
                break;

            case 0xBEEF:
                break;

            default:
                return DefWindowProc( hWnd, uMessage, wParam, lParam );
        }

        return 0;
    }

    namespace
    {
        ATOM g_systemTrayClassReg = 0;
    }

    SystemTrayHandle Platform::OpenSystemTray( const OpenSystemTrayDesc& desc )
    {
        HINSTANCE hInstance = {};

        if( g_systemTrayClassReg == 0 )
        {
            const WNDCLASSEX wc {
                .cbSize = sizeof( WNDCLASSEX ),
                .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                .lpfnWndProc = SystemTrayProc,
                .cbClsExtra = 0,
                .cbWndExtra = 0,
                .hInstance = hInstance,
                .hIcon = ::LoadIcon( hInstance, MAKEINTRESOURCE( 101 ) ),
                .hCursor = ::LoadCursor( hInstance, IDC_ARROW ),
                .hbrBackground = static_cast<HBRUSH>( ::GetStockObject( DKGRAY_BRUSH ) ),
                .lpszMenuName = nullptr,
                .lpszClassName = kZeroPointSystemTrayClassName,
                .hIconSm = static_cast<HICON>( ::LoadImage( hInstance, MAKEINTRESOURCE( 101 ), IMAGE_ICON, ::GetSystemMetrics( SM_CXSMICON ), ::GetSystemMetrics( SM_CYSMICON ), LR_DEFAULTCOLOR ) ),
            };

            g_systemTrayClassReg = ::RegisterClassEx( &wc );
        }

        void* mem = ::HeapAlloc( ::GetProcessHeap(), HEAP_NO_SERIALIZE, sizeof( NOTIFYICONDATA ) );
        PNOTIFYICONDATA notifyIconData = reinterpret_cast<PNOTIFYICONDATA>( mem );

        HWND hWnd = ::CreateWindowEx(
            0,
            kZeroPointSystemTrayClassName,
            "SystemTray",
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );
        ZP_ASSERT( hWnd );

        ::ShowWindow( hWnd, SW_HIDE );

        *notifyIconData = {
            .cbSize = sizeof( NOTIFYICONDATA ),
            .hWnd = hWnd,
            .uID = *(UINT*)notifyIconData,
            .uFlags = NIF_ICON | NIF_TIP,
            .uCallbackMessage = 0xBEEF,
            .hIcon = static_cast<HICON>( LoadImage( hInstance, MAKEINTRESOURCE( 101 ), IMAGE_ICON, ::GetSystemMetrics( SM_CXSMICON ), ::GetSystemMetrics( SM_CYSMICON ), LR_DEFAULTCOLOR ) ),
            .uVersion = NOTIFYICON_VERSION_4,
        };

        WINBOOL ok = ::Shell_NotifyIcon( NIM_ADD, notifyIconData );
        if( !ok )
        {
            ::HeapFree( ::GetProcessHeap(), HEAP_NO_SERIALIZE, notifyIconData );

            notifyIconData = nullptr;
        }

        return { .handle = notifyIconData };
    }

    void Platform::CloseSystemTray( SystemTrayHandle systemTrayHandle )
    {
        if( systemTrayHandle.handle )
        {
            PNOTIFYICONDATA notifyIconData = reinterpret_cast<PNOTIFYICONDATA>( systemTrayHandle.handle );

            ::Shell_NotifyIcon( NIM_DELETE, notifyIconData );

            ::HeapFree( ::GetProcessHeap(), HEAP_NO_SERIALIZE, notifyIconData );
        }
    }

    ConsoleHandle Platform::OpenConsole()
    {
        const WINBOOL ok = ::AllocConsole();
        return { .handle = ok ? zp_handle_t( ~0 ) : nullptr };
    }

    zp_bool_t Platform::CloseConsole( ConsoleHandle )
    {
        const WINBOOL ok = ::FreeConsole();
        return ok;
    }

    zp_bool_t Platform::SetConsoleTitle( ConsoleHandle, const String& title )
    {
        const WINBOOL ok = ::SetConsoleTitle( title.c_str() );
        return ok;
    }

    void* Platform::AllocateSystemMemory( void* baseAddress, const zp_size_t size )
    {
        const zp_size_t allocationInPageSize = GetMemoryPageSize( size );

        void* ptr = ::VirtualAlloc( baseAddress, allocationInPageSize, MEM_RESERVE, PAGE_NOACCESS );
        return ptr;
    }

    void Platform::FreeSystemMemory( void* ptr )
    {
        ::VirtualFree( ptr, 0, MEM_RELEASE );
    }

    zp_size_t Platform::GetMemoryPageSize( const zp_size_t size )
    {
        SYSTEM_INFO systemInfo;
        ::GetSystemInfo( &systemInfo );

        const zp_size_t systemPageSize = systemInfo.dwPageSize;

        const zp_size_t requestedSize = size > systemPageSize ? size : systemPageSize;
        const zp_size_t allocationInPageSize = ( requestedSize & ( systemPageSize - 1 ) ) + requestedSize;
        return allocationInPageSize;
    }

    void* Platform::CommitMemoryPage( void** ptr, const zp_size_t size )
    {
        void* page = ::VirtualAlloc( *ptr, size, MEM_COMMIT, PAGE_READWRITE );

        const zp_size_t allocationInPageSize = GetMemoryPageSize( size );
        *ptr = static_cast<zp_uint8_t*>( *ptr ) + allocationInPageSize;
        return page;
    }

    void Platform::DecommitMemoryPage( void* ptr, const zp_size_t size )
    {
        ::VirtualFree( ptr, size, MEM_DECOMMIT );
    }

    zp_size_t Platform::GetCurrentDir( char* path, zp_size_t maxPathLength )
    {
        zp_size_t length = ::GetCurrentDirectory( 0, nullptr );
        ZP_ASSERT( length < maxPathLength );

        ::GetCurrentDirectory( maxPathLength, path );
        return length;
    }

    String Platform::GetCurrentDirStr( char* path, zp_size_t maxPathLength )
    {
        return String::As( path, GetCurrentDir( path, maxPathLength ) );
    }

    zp_bool_t Platform::CreateDirectory( const char* path )
    {
        BOOL ok = ::CreateDirectory( path, nullptr );

        if( !ok )
        {
            switch( ::GetLastError() )
            {
                case ERROR_ALREADY_EXISTS:
                    ok = true;
                    break;
            }
        }

        return ok;
    }

    zp_bool_t Platform::DirectoryExists( const char* path )
    {
        const DWORD attr = ::GetFileAttributes( path );
        const zp_bool_t exists = ( attr != INVALID_FILE_ATTRIBUTES ) && ( attr & FILE_ATTRIBUTE_DIRECTORY );
        return exists;
    }

    zp_bool_t Platform::FileExists( const char* filePath )
    {
        const DWORD attr = ::GetFileAttributes( filePath );
        const zp_bool_t exists = ( attr != INVALID_FILE_ATTRIBUTES ) && !( attr & FILE_ATTRIBUTE_DIRECTORY );
        return exists;
    }

    zp_bool_t Platform::FileCopy( const char* srcFilePath, const char* dstFilePath, zp_bool_t force )
    {
        const BOOL ok = ::CopyFile( srcFilePath, dstFilePath, !force );
        return ok;
    }

    zp_bool_t Platform::FileMove( const char* srcFilePath, const char* dstFilePath )
    {
        const BOOL ok = ::MoveFile( srcFilePath, dstFilePath );
        return ok;
    }

    FileHandle Platform::OpenFileHandle( const char* filePath, OpenFileMode openFileMode, CreateFileMode createFileMode, FileCachingMode fileCachingMode )
    {
        DWORD access = 0;
        DWORD shareMode = 0;
        DWORD createDesc = 0;
        DWORD attributes = 0;

        switch( openFileMode )
        {
            case ZP_OPEN_FILE_MODE_READ:
                access = FILE_GENERIC_READ;
                shareMode = FILE_SHARE_READ;
                createDesc = OPEN_EXISTING;
                attributes = FILE_ATTRIBUTE_READONLY;
                break;

            case ZP_OPEN_FILE_MODE_WRITE:
                access = FILE_GENERIC_WRITE;
                shareMode = FILE_SHARE_WRITE;
                createDesc = CREATE_ALWAYS;
                attributes = FILE_ATTRIBUTE_NORMAL;
                break;

            case ZP_OPEN_FILE_MODE_READ_WRITE:
                access = FILE_ALL_ACCESS;
                shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
                createDesc = OPEN_EXISTING;
                attributes = FILE_ATTRIBUTE_NORMAL;
                break;

            default:
                ZP_INVALID_CODE_PATH();
                break;
        }

        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_SEQUENTIAL, FILE_FLAG_SEQUENTIAL_SCAN );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_RANDOM_ACCESS, FILE_FLAG_RANDOM_ACCESS );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_NO_BUFFERING, FILE_FLAG_NO_BUFFERING );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_WRITE_THROUGH, FILE_FLAG_WRITE_THROUGH );

        zp_handle_t fileHandle = ::CreateFile(
            filePath,
            access,
            shareMode,
            nullptr,
            createDesc,
            attributes,
            nullptr );
        return { .handle = fileHandle };
    }

    FileHandle Platform::OpenTempFileHandle( const char* tempFileNamePrefix, const char* tempFileNameExtension, FileCachingMode fileCachingMode )
    {
        char tempPath[MAX_PATH];
        const zp_size_t len = ::GetTempPath( ZP_ARRAY_SIZE( tempPath ), tempPath );

        FilePath tempRootPath;
        tempRootPath / tempPath / "ZeroPoint";

        //char tempRootPath[MAX_PATH];
        //zp_snprintf( tempRootPath, "%s%c%s%c", tempPath, '\\', "ZeroPoint " ZP_VERSION, '\\' );

        ::CreateDirectory( tempRootPath.c_str(), nullptr );

        char tempFileName[MAX_PATH];
        const zp_time_t unique = Platform::TimeNow();
        zp_snprintf( tempFileName, "%s-%x.%s", tempFileNamePrefix ? tempFileNamePrefix : "tmp", unique, tempFileNameExtension ? tempFileNameExtension : "tmp" );

        FilePath finalFileNamePath;
        finalFileNamePath / tempRootPath / tempFileName;

        DWORD attributes = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;

        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_SEQUENTIAL, FILE_FLAG_SEQUENTIAL_SCAN );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_RANDOM_ACCESS, FILE_FLAG_RANDOM_ACCESS );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_NO_BUFFERING, FILE_FLAG_NO_BUFFERING );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_WRITE_THROUGH, FILE_FLAG_WRITE_THROUGH );

        zp_handle_t fileHandle = ::CreateFile(
            finalFileNamePath.c_str(),
            FILE_ALL_ACCESS,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            CREATE_ALWAYS,
            attributes,
            nullptr );
        return { .handle = fileHandle };
    }

    void Platform::SeekFile( FileHandle fileHandle, const zp_ptrdiff_t distanceToMoveInBytes, const MoveMethod moveMethod )
    {
        LARGE_INTEGER distance {
            .QuadPart = distanceToMoveInBytes,
        };

        constexpr DWORD moveMethodMapping[] {
            FILE_BEGIN,
            FILE_CURRENT,
            FILE_END
        };

        ::SetFilePointerEx( fileHandle.handle, distance, nullptr, moveMethodMapping[ moveMethod ] );
    }

    zp_size_t Platform::GetFileSize( FileHandle fileHandle )
    {
        LARGE_INTEGER fileSize;
        const BOOL ok = ::GetFileSizeEx( fileHandle.handle, &fileSize );
        return ok ? static_cast<zp_size_t>(fileSize.QuadPart) : 0;
    }

    void Platform::CloseFileHandle( FileHandle fileHandle )
    {
        ::CloseHandle( fileHandle.handle );
    }

    zp_size_t Platform::ReadFile( FileHandle fileHandle, void* buffer, const zp_size_t bytesToRead )
    {
        DWORD bytesRead = 0;
        const BOOL ok = ::ReadFile( fileHandle.handle, buffer, bytesToRead, &bytesRead, nullptr );
        return ok ? bytesRead : 0;
    }

    zp_size_t Platform::WriteFile( FileHandle fileHandle, const void* data, const zp_size_t size )
    {
        DWORD bytesWritten = 0;
        const BOOL ok = ::WriteFile( fileHandle.handle, data, size, &bytesWritten, nullptr );

        return ok ? bytesWritten : 0;
    }

    zp_handle_t Platform::LoadExternalLibrary( const char* libraryPath )
    {
        HMODULE h = ::LoadLibrary( libraryPath );
        return h;
    }

    void Platform::UnloadExternalLibrary( zp_handle_t libraryHandle )
    {
        if( libraryHandle )
        {
            ::FreeLibrary( static_cast<HMODULE>(libraryHandle) );
        }
    }

    ProcAddressFunc Platform::GetProcAddress( zp_handle_t libraryHandle, const char* name )
    {
        FARPROC procAddress = ::GetProcAddress( static_cast<HMODULE >(libraryHandle), name );
        return static_cast<ProcAddressFunc>( procAddress );
    }

    zp_handle_t Platform::AllocateThreadPool( zp_uint32_t minThreads, zp_uint32_t maxThreads )
    {
        void* mem = ::HeapAlloc( ::GetProcessHeap(), HEAP_NO_SERIALIZE, sizeof( TP_CALLBACK_ENVIRON ) );

        auto callbackEnviron = static_cast<PTP_CALLBACK_ENVIRON>( mem );
        ::InitializeThreadpoolEnvironment( callbackEnviron );

        PTP_POOL pool = ::CreateThreadpool( nullptr );
        PTP_CLEANUP_GROUP cleanupGroup = ::CreateThreadpoolCleanupGroup();

        ::SetThreadpoolCallbackPool( callbackEnviron, pool );
        ::SetThreadpoolCallbackCleanupGroup( callbackEnviron, cleanupGroup, nullptr );

        ::SetThreadpoolThreadMinimum( pool, minThreads );
        ::SetThreadpoolThreadMaximum( pool, maxThreads );

        return callbackEnviron;
    }

    void Platform::FreeThreadPool( zp_handle_t threadPool )
    {
        auto callbackEnviron = static_cast<PTP_CALLBACK_ENVIRON>( threadPool );
        ::CloseThreadpoolCleanupGroupMembers( callbackEnviron->CleanupGroup, FALSE, nullptr );
        ::CloseThreadpoolCleanupGroup( callbackEnviron->CleanupGroup );
        ::CloseThreadpool( callbackEnviron->Pool );

        ::HeapFree( ::GetProcessHeap(), HEAP_NO_SERIALIZE, threadPool );
    }

    ThreadHandle Platform::CreateThread( ThreadFunc threadFunc, void* param, const zp_size_t stackSize, zp_uint32_t* threadId )
    {
        DWORD id;
        HANDLE threadHandle = ::CreateThread(
            nullptr,
            stackSize,
            (LPTHREAD_START_ROUTINE)threadFunc,
            param,
            THREAD_SET_INFORMATION,
            &id );
        ZP_ASSERT( threadHandle );

        if( threadId )
        {
            *threadId = id;
        }

        return { .handle = threadHandle };
    }

    ThreadHandle Platform::GetCurrentThread()
    {
        HANDLE threadHandle = ::GetCurrentThread();
        return { .handle = static_cast<zp_handle_t>( threadHandle ) };
    }

    zp_uint32_t Platform::GetCurrentThreadId()
    {
        const DWORD threadId = ::GetCurrentThreadId();
        return static_cast<zp_uint32_t>( threadId );
    }

    void Platform::YieldCurrentThread()
    {
        ::YieldProcessor();
    }

    void Platform::SleepCurrentThread( zp_uint64_t milliseconds )
    {
        ::Sleep( milliseconds );
    }

    zp_uint32_t Platform::GetThreadId( ThreadHandle threadHandle )
    {
        const DWORD threadId = ::GetThreadId( static_cast<HANDLE>( threadHandle.handle ) );
        return static_cast<zp_uint32_t>( threadId );
    }

    void Platform::SetThreadName( ThreadHandle threadHandle, const String& threadName )
    {
        //::SetThreadDescription();

        // Code from: https://docs.microsoft.com/en-us/archive/blogs/stevejs/naming-threads-in-win32-and-net
#pragma pack(push, 8)
        typedef struct tagTHREADNAME_INFO
        {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        } THREADNAME_INFO;
#pragma pack(pop)

        THREADNAME_INFO info {
            .dwType = 0x1000,
            .szName = threadName.c_str(),
            .dwThreadID = ::GetThreadId( static_cast<HANDLE>( threadHandle.handle ) ),
            .dwFlags = 0,
        };

#pragma warning(push)
#pragma warning(disable: 6320 6322)
        try
        {
            const DWORD MS_VC_EXCEPTION = 0x406D1388;
            RaiseException( MS_VC_EXCEPTION, 0, sizeof( THREADNAME_INFO ) / sizeof( const ULONG_PTR* ), (const ULONG_PTR*)&info );
        }
        catch( ... )
        {
        }
#pragma warning(pop)
    }

    void Platform::SetThreadPriority( ThreadHandle threadHandle, zp_int32_t priority )
    {
        ::SetThreadPriority( static_cast<HANDLE>( threadHandle.handle ), priority );
    }

    zp_int32_t Platform::GetThreadPriority( ThreadHandle threadHandle )
    {
        const zp_int32_t priority = ::GetThreadPriority( static_cast<HANDLE>( threadHandle.handle ) );
        return priority;
    }

    void Platform::SetThreadAffinity( ThreadHandle threadHandle, zp_uint64_t affinityMask )
    {
        ::SetThreadAffinityMask( static_cast<HANDLE>( threadHandle.handle ), affinityMask );
    }

    void Platform::SetThreadIdealProcessor( ThreadHandle threadHandle, zp_uint32_t processorIndex )
    {
        SYSTEM_INFO systemInfo;
        ::GetSystemInfo( &systemInfo );

        processorIndex = zp_clamp<zp_uint32_t>( processorIndex, 0, systemInfo.dwNumberOfProcessors );

        const zp_uint64_t requestedMask = 1 << processorIndex;
        if( systemInfo.dwActiveProcessorMask & requestedMask )
        {
            PROCESSOR_NUMBER processorNumber {
                .Number = static_cast<BYTE>( 0xFFu & processorIndex )
            };

            ::SetThreadIdealProcessorEx( static_cast<HANDLE>( threadHandle.handle ), &processorNumber, nullptr );
        }
    }

    void Platform::CloseThread( ThreadHandle threadHandle )
    {
        ::CloseHandle( threadHandle.handle );
    }

    void Platform::JoinThreads( ThreadHandle* threadHandles, zp_size_t threadHandleCount )
    {
        HANDLE handles[threadHandleCount];
        for( zp_size_t i = 0; i < threadHandleCount; ++i )
        {
            handles[ i ] = threadHandles[ i ].handle;
        }

        ::WaitForMultipleObjects( threadHandleCount, handles, true, INFINITE );
    }

    zp_uint32_t Platform::GetProcessorCount()
    {
        SYSTEM_INFO systemInfo;
        ::GetSystemInfo( &systemInfo );
        return systemInfo.dwNumberOfProcessors;
    }

    zp_time_t Platform::TimeNow()
    {
        LARGE_INTEGER val;
        ::QueryPerformanceCounter( &val );
        return val.QuadPart;
    }

    zp_time_t Platform::TimeFrequency()
    {
        LARGE_INTEGER val;
        ::QueryPerformanceFrequency( &val );
        return val.QuadPart;
    }

    zp_uint64_t Platform::TimeCycles()
    {
        const zp_uint64_t cycles = ::__rdtsc();
        return cycles;
    }

    namespace
    {
        thread_local tm t_tm;
    }

    DateTime Platform::DateTimeNowLocal()
    {
        timeval tv {};
        ::gettimeofday( &tv, nullptr );

        const time_t t = tv.tv_sec;
        ::localtime_s( &t_tm, &t );

        return {
            .year = static_cast<zp_uint16_t>( t_tm.tm_year + 1900 ),
            .month = static_cast<zp_uint8_t>( t_tm.tm_mon ),
            .day = static_cast<zp_uint8_t>( t_tm.tm_mday ),
            .hour = static_cast<zp_uint8_t>( t_tm.tm_hour ),
            .minute = static_cast<zp_uint8_t>( t_tm.tm_min ),
            .second = static_cast<zp_uint8_t>( t_tm.tm_sec ),
            .microseconds = static_cast<zp_uint32_t>( tv.tv_usec ),
            .year_day = static_cast<zp_uint16_t>( t_tm.tm_yday ),
            .week_day = static_cast<zp_uint8_t>( t_tm.tm_wday ),
            .is_dst = !!t_tm.tm_isdst
        };
    }

    DateTime Platform::DateTimeNowUTC()
    {
        timeval tv {};
        ::gettimeofday( &tv, nullptr );

        const time_t t = tv.tv_sec;
        ::gmtime_s( &t_tm, &t );

        return {
            .year = static_cast<zp_uint16_t>( t_tm.tm_year + 1900 ),
            .month = static_cast<zp_uint8_t>( t_tm.tm_mon ),
            .day = static_cast<zp_uint8_t>( t_tm.tm_mday ),
            .hour = static_cast<zp_uint8_t>( t_tm.tm_hour ),
            .minute = static_cast<zp_uint8_t>( t_tm.tm_min ),
            .second = static_cast<zp_uint8_t>( t_tm.tm_sec ),
            .microseconds = static_cast<zp_uint32_t>( tv.tv_usec ),
            .year_day = static_cast<zp_uint16_t>( t_tm.tm_yday ),
            .week_day = static_cast<zp_uint8_t>( t_tm.tm_wday ),
            .is_dst = !!t_tm.tm_isdst
        };
    }

    zp_size_t Platform::DateTimeToString( const DateTime& dateTime, char* str, zp_size_t length, const char* format )
    {
        const tm tm {
            .tm_sec = dateTime.second,
            .tm_min = dateTime.minute,
            .tm_hour = dateTime.hour,
            .tm_mday = dateTime.day,
            .tm_mon = dateTime.month,
            .tm_year = dateTime.year - 1900,
            .tm_wday = dateTime.week_day,
            .tm_yday = dateTime.year_day,
            .tm_isdst = dateTime.is_dst,
        };


        zp_size_t end;

        // handle special case of %S. to append milliseconds to the seconds
        if( const char* milli = zp_strstr( format, "%S." ) )
        {
            milli += zp_strlen( "%S." );

            MutableFixedString64 formatBuffer;
            formatBuffer.append( format, milli - format );
            formatBuffer.appendFormat( "%3d", dateTime.microseconds / 1000 );
            formatBuffer.append( milli );

            end = ::strftime( str, length, formatBuffer.c_str(), &tm );
        }
        else
        {
            end = ::strftime( str, length, format, &tm );
        }

        return end;
    }

    MessageBoxResult Platform::ShowMessageBox( zp_handle_t windowHandle, const char* title, const char* message, MessageBoxType messageBoxType, MessageBoxButton messageBoxButton )
    {
        constexpr UINT buttonMap[] {
            MB_OK,
            MB_OKCANCEL,
            MB_RETRYCANCEL,
            MB_HELP,
            MB_YESNO,
            MB_YESNOCANCEL,
            MB_ABORTRETRYIGNORE,
            MB_CANCELTRYCONTINUE,
        };
        constexpr UINT typeMap[] {
            MB_ICONINFORMATION,
            MB_ICONWARNING,
            MB_ICONERROR,
        };

        HWND hWnd = static_cast<HWND>( windowHandle );
        const UINT type = typeMap[ messageBoxType ] | buttonMap[ messageBoxButton ];

        const int id = ::MessageBoxEx( hWnd, message, title, type, 0 );

        constexpr MessageBoxResult resultMap[] {
            ZP_MESSAGE_BOX_RESULT_ABORT, // not used
            ZP_MESSAGE_BOX_RESULT_OK,
            ZP_MESSAGE_BOX_RESULT_CANCEL,
            ZP_MESSAGE_BOX_RESULT_ABORT,
            ZP_MESSAGE_BOX_RESULT_RETRY,
            ZP_MESSAGE_BOX_RESULT_IGNORE,
            ZP_MESSAGE_BOX_RESULT_YES,
            ZP_MESSAGE_BOX_RESULT_NO,
            ZP_MESSAGE_BOX_RESULT_CLOSE,
            ZP_MESSAGE_BOX_RESULT_HELP,
            ZP_MESSAGE_BOX_RESULT_TRY_AGAIN,
            ZP_MESSAGE_BOX_RESULT_CONTINUE,
        };

        const MessageBoxResult result = resultMap[ id ];
        return result;
    }

    void Platform::DebugBreak()
    {
#if ZP_DEBUG
        ::DebugBreak();
#endif
    }

    zp_int32_t Platform::ExecuteProcess( const char* process, const char* arguments )
    {
        STARTUPINFO startupinfo {
            .cb = sizeof( STARTUPINFO ),
        };

        DWORD returnCode = -1;

        MutableFixedString<1 KB> commandLine;
        commandLine.append( process );
        commandLine.append( ' ' );
        commandLine.append( arguments );

        PROCESS_INFORMATION processInformation {};
        const zp_bool_t ok = ::CreateProcess( nullptr, commandLine.mutable_str(), nullptr, nullptr, false, 0, nullptr, nullptr, &startupinfo, &processInformation );
        if( ok )
        {
            ::WaitForSingleObject( processInformation.hProcess, INFINITE );

            ::GetExitCodeProcess( processInformation.hProcess, &returnCode );

            ::CloseHandle( processInformation.hProcess );
            ::CloseHandle( processInformation.hThread );
        }
        else
        {
            PrintLastErrorMsg( "Failed to create process" );
        }

        return static_cast<zp_int32_t>( returnCode );
    }

    Semaphore Platform::CreateSemaphore( zp_int32_t initialCount, zp_int32_t maxCount, const char* name )
    {
        ZP_ASSERT( initialCount <= maxCount );

        HANDLE hSemaphore = ::CreateSemaphore( nullptr, initialCount, maxCount, name );
        if( hSemaphore == nullptr )
        {
            PrintLastErrorMsg( "Failed to create Semaphore" );
        }

        return { .handle = hSemaphore };
    }

    Platform::AcquireSemaphoreResult Platform::AcquireSemaphore( Semaphore semaphore, zp_time_t millisecondTimeout )
    {
        HANDLE hSemaphore = static_cast<HANDLE>( semaphore.handle );

        const DWORD result = ::WaitForSingleObject( hSemaphore, millisecondTimeout );
        switch( result )
        {
            case WAIT_OBJECT_0:
                return AcquireSemaphoreResult::Signaled;

            case WAIT_TIMEOUT:
                return AcquireSemaphoreResult::NotSignaled;

            case WAIT_ABANDONED:
                return AcquireSemaphoreResult::Abandoned;

            case WAIT_FAILED:
                PrintLastErrorMsg( "Failed to acquire Semaphore" );
                return AcquireSemaphoreResult::Failed;

            default:
                ZP_INVALID_CODE_PATH();
                return AcquireSemaphoreResult::Failed;
        }
    }

    zp_int32_t Platform::ReleaseSemaphore( Semaphore semaphore, zp_int32_t releaseCount )
    {
        HANDLE hSemaphore = static_cast<HANDLE>( semaphore.handle );

        LONG prevReleaseCount;
        const WINBOOL ok = ::ReleaseSemaphore( hSemaphore, releaseCount, &prevReleaseCount );
        if( !ok )
        {
            PrintLastErrorMsg( "Failed to release Semaphore" );
        }

        return prevReleaseCount;
    }

    zp_bool_t Platform::CloseSemaphore( Semaphore semaphore )
    {
        HANDLE hSemaphore = static_cast<HANDLE>( semaphore.handle );

        const WINBOOL ok = ::CloseHandle( hSemaphore );
        if( !ok )
        {
            PrintLastErrorMsg( "" );
        }

        return ok;
    }

    Mutex Platform::CreateMutex( zp_bool_t initialOwner, const char* name )
    {
        HANDLE hMutex = ::CreateMutex( nullptr, initialOwner, name );
        if( hMutex == nullptr )
        {
            PrintLastErrorMsg( "Failed to create Mutex" );
        }

        return { .handle = hMutex };
    }

    AcquireMutexResult Platform::AcquireMutex( Mutex mutex, zp_time_t millisecondTimeout )
    {
        HANDLE hSemaphore = static_cast<HANDLE>( mutex.handle );

        const DWORD result = ::WaitForSingleObject( hSemaphore, millisecondTimeout );
        switch( result )
        {
            case WAIT_OBJECT_0:
                return AcquireMutexResult::Acquired;

            case WAIT_ABANDONED:
                return AcquireMutexResult::Abandoned;

            case WAIT_FAILED:
                PrintLastErrorMsg( "Failed to acquire Mutex" );
                return AcquireMutexResult::Failed;

            default:
                ZP_INVALID_CODE_PATH();
                return AcquireMutexResult::Failed;
        }
    }

    zp_bool_t Platform::ReleaseMutex( Mutex mutex )
    {
        HANDLE hMutex = static_cast<HANDLE>( mutex.handle );

        const WINBOOL ok = ::ReleaseMutex( hMutex );
        if( !ok )
        {
            PrintLastErrorMsg( "Failed to release Mutex" );
        }

        return ok;
    }

    zp_bool_t Platform::CloseMutex( Mutex mutex )
    {
        HANDLE hMutex = static_cast<HANDLE>( mutex.handle );

        const WINBOOL ok = ::CloseHandle( hMutex );
        if( !ok )
        {
            PrintLastErrorMsg( "Failed to close Mutex" );
        }

        return ok;
    }

    zp_bool_t Platform::InitializeNetworking()
    {
        WSADATA wsaData {};
        int r = WSAStartup( WINSOCK_VERSION, &wsaData );
        if( r )
        {
            PrintLastWSAErrorMsg( "Failed to start WSA" );
            return false;
        }

        return true;
    }

    void ShutdownNetworking()
    {
        WSACleanup();
    }

    Socket Platform::OpenSocket( const SocketDesc& desc )
    {
        int addressFamily = AF_INET;
        int socketType = SOCK_STREAM;
        int protocol = IPPROTO_TCP;
        DWORD flags = 0;

        //SOCKET openSocket = ::WSASocket( addressFamily, socketType, protocol, nullptr, 0, flags );
        SOCKET openSocket = ::socket( addressFamily, socketType, protocol );
        if( openSocket == INVALID_SOCKET )
        {
            PrintLastWSAErrorMsg( "Failed to open socket" );
        }
        else
        {
            sockaddr_in addr {
                .sin_family = static_cast<short>( addressFamily ),
                .sin_port = htons( desc.address.port ),
            };
            addr.sin_addr.s_addr = inet_addr( desc.address.addr );

            int r;

            switch( desc.socketDirection )
            {
                case SocketDirection::Connect:
                {
                    r = ::connect( openSocket, reinterpret_cast<SOCKADDR*>(&addr), sizeof( addr ) );
                    if( r == SOCKET_ERROR )
                    {
                        PrintLastWSAErrorMsg( "Failed to connect socket" );
                    }
                }
                    break;

                case SocketDirection::Listen:
                {
                    //int r = ::WSAConnect( openSocket, &addr, sizeof(sockaddr), nullptr, nullptr, nullptr, nullptr );
                    r = ::bind( openSocket, reinterpret_cast<SOCKADDR*>(&addr), sizeof( addr ) );
                    if( r == SOCKET_ERROR )
                    {
                        PrintLastWSAErrorMsg( "Failed to bind socket" );

                        ::closesocket( openSocket );
                        openSocket = INVALID_SOCKET;
                    }
                    else
                    {
                        r = ::listen( openSocket, 0 );
                        if( r == SOCKET_ERROR )
                        {
                            PrintLastWSAErrorMsg( "Failed to listen socket" );
                            openSocket = INVALID_SOCKET;
                        }
                    }
                }
                    break;

                default:
                    ZP_INVALID_CODE_PATH_MSG( "Unknown " ZP_NAMEOF( SocketDirection ) );
                    break;
            }

        }

        return { .handle = openSocket };
    }

    Socket Platform::AcceptSocket( Socket socket )
    {
        ZP_ASSERT( socket.handle != INVALID_SOCKET );

        SOCKET acceptedSocket = ::accept( socket.handle, nullptr, nullptr );
        if( acceptedSocket == INVALID_SOCKET )
        {
            PrintLastWSAErrorMsg( "Failed to accept socket" );

            acceptedSocket = INVALID_SOCKET;
        }

        return { .handle = acceptedSocket };
    }

    zp_size_t Platform::ReceiveSocket( Socket socket, void* dst, zp_size_t dstSize )
    {
        ZP_ASSERT( socket.handle != INVALID_SOCKET );
        ZP_ASSERT( dstSize < zp_limit<int>::max() );

        const int flags = 0;
        int r = ::recv( socket.handle, static_cast<char*>( dst ), static_cast<int>( dstSize ), flags );
        if( r == SOCKET_ERROR )
        {
            PrintLastWSAErrorMsg( "Failed to receive socket" );

            r = 0;
        }

        return r;
    }

    zp_size_t Platform::SendSocket( Socket socket, const void* src, zp_size_t srcSize )
    {
        ZP_ASSERT( socket.handle != INVALID_SOCKET );
        ZP_ASSERT( srcSize < zp_limit<int>::max() );

        const int flags = 0;
        int r = ::send( socket.handle, static_cast<const char*>( src ), static_cast<int>( srcSize ), flags );
        if( r == SOCKET_ERROR )
        {
            PrintLastWSAErrorMsg( "Failed to send socket" );

            r = 0;
        }

        return r;
    }

    void Platform::CloseSocket( Socket socket )
    {
        ZP_ASSERT( socket.handle != INVALID_SOCKET );

        int r = ::closesocket( socket.handle );
        if( r == SOCKET_ERROR )
        {
            PrintLastWSAErrorMsg( "Failed to close socket" );
        }
    }
}
