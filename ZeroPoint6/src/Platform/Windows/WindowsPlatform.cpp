//
// Created by phosg on 11/10/2021.
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <processthreadsapi.h>
#include <excpt.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Math.h"
#include "Core/Common.h"
#include "Core/Version.h"
#include "Core/Allocator.h"
#include "Platform/Platform.h"

#define TestFlag( a, f, t, r )      a |= ((f) & (t)) ? (r) : 0

namespace zp
{
    namespace
    {
        Platform s_WindowsPlatform;

        const char* kZeroPointClassName = "ZeroPoint::WindowClass";

        LRESULT CALLBACK WinProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
        {
            auto windowCallbacks = reinterpret_cast<WindowCallbacks*>(::GetWindowLongPtr( hWnd, GWLP_USERDATA ));

            switch( uMessage )
            {
                case WM_CLOSE:
                {
                    ::DestroyWindow( hWnd );
                }
                    break;

                case WM_DESTROY:
                {
                    //::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR) nullptr );
                    ::PostQuitMessage( 0 );
                }
                    break;

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
                            windowCallbacks->onWindowGetMinMaxSize( hWnd, minWidth, minHeight, maxWidth, maxHeight );
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
                            DWORD style = ::GetWindowLong( hWnd, GWL_STYLE );
                            DWORD exStyle = ::GetWindowLong( hWnd, GWL_EXSTYLE );

                            RECT rc;
                            ::SetRectEmpty( &rc );
                            ::AdjustWindowRectEx( &rc, style, false, exStyle );

                            zp_int32_t width = pos->cx - ( rc.right - rc.left );
                            zp_int32_t height = pos->cy - ( rc.bottom - rc.top );

                            windowCallbacks->onWindowResize( hWnd, width, height );
                        }
                    }
                }
                    break;

                case WM_KILLFOCUS:
                {
                    if( windowCallbacks && windowCallbacks->onWindowFocus )
                    {
                        windowCallbacks->onWindowFocus( hWnd, false );
                    }
                }
                    break;

                case WM_SETFOCUS:
                {
                    if( windowCallbacks && windowCallbacks->onWindowFocus )
                    {
                        windowCallbacks->onWindowFocus( hWnd, true );
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
                        }

                        WindowKeyEvent event {
                            .keyCode = vkCode,
                            .repeatCount = repeatCount,
                            .isCtrlDown = isCtrlKeyDown,
                            .isShiftDown = isShiftKeyDown,
                            .isAltDown = isAltKeyDown,
                            .wasKeyDown = wasKeyDown,
                            .isKeyReleased = isKeyReleased,
                        };
                        windowCallbacks->onWindowKeyEvent( hWnd, event );
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

                        WindowMouseEvent event {
                            .x = p.x,
                            .y = p.y,
                            .zDelta = zDelta,
                            .isCtrlDown = isCtrlDown,
                            .isShiftDown = isShiftDown,
                        };
                        windowCallbacks->onWindowMouseEvent( hWnd, event );
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
                    ::PostQuitMessage( 3 );
                }
                    break;

                default:
                    return ::DefWindowProc( hWnd, uMessage, wParam, lParam );
            }

            return 0;
        }
    }

    const zp_char8_t Platform::PathSep = '\\';

    Platform* GetPlatform()
    {
        return &s_WindowsPlatform;
    }

    Platform::Platform()
    {
        SYSTEM_INFO systemInfo;
        ::GetSystemInfo( &systemInfo );

        m_activeProcessorMask = systemInfo.dwActiveProcessorMask;
        m_systemPageSize = systemInfo.dwPageSize;
        m_numProcessors = systemInfo.dwNumberOfProcessors;
    }

    zp_handle_t Platform::OpenWindow( const OpenWindowDesc& desc )
    {
        HINSTANCE hInstance = desc.instanceHandle ? static_cast<HINSTANCE>(desc.instanceHandle) : ::GetModuleHandle( nullptr );

        const WNDCLASSEX wc {
            .cbSize = sizeof( WNDCLASSEX ),
            .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            .lpfnWndProc = WinProc,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = hInstance,
            .hIcon = LoadIcon( hInstance, IDI_APPLICATION ),
            .hCursor = LoadCursor( hInstance, IDC_ARROW ),
            .hbrBackground = static_cast<HBRUSH>(GetStockObject( DKGRAY_BRUSH )),
            .lpszMenuName = nullptr,
            .lpszClassName = kZeroPointClassName,
            .hIconSm = LoadIcon( hInstance, IDI_APPLICATION ),
        };

        const ATOM reg = ::RegisterClassEx( &wc );
        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
        DWORD exStyle = 0;
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

        ::ShowWindow( hWnd, desc.showWindow ? SW_SHOW : SW_HIDE );
        ::UpdateWindow( hWnd );

        return hWnd;
    }

    zp_bool_t Platform::DispatchWindowMessages( zp_handle_t windowHandle, zp_int32_t& exitCode )
    {
        zp_bool_t isRunning = true;

        HWND hWnd = static_cast<HWND>( windowHandle );

        MSG msg {};
        while( ::PeekMessage( &msg, hWnd, 0, 0, PM_REMOVE ) )
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

    void Platform::CloseWindow( zp_handle_t windowHandle )
    {
        HWND hWnd = static_cast<HWND>( windowHandle );
        if( ::IsWindow( hWnd ) )
        {
            ::CloseWindow( hWnd );
        }
    }

    void Platform::SetWindowTitle( zp_handle_t windowHandle, const String& title )
    {
        HWND hWnd = static_cast<HWND>( windowHandle);
        if( ::IsWindow( hWnd ) )
        {
            ::SetWindowText( hWnd, title.c_str() );
        }
    }

    void Platform::SetWindowSize( zp_handle_t windowHandle, const zp_int32_t width, const zp_int32_t height )
    {
        HWND hWnd = static_cast<HWND>( windowHandle );
        if( ::IsWindow( hWnd ) )
        {
            const LONG style = ::GetWindowLong( hWnd, GWL_STYLE );
            const LONG exStyle = ::GetWindowLong( hWnd, GWL_EXSTYLE );

            RECT r { .left = 0, .top = 0, .right = width, .bottom = height };
            ::AdjustWindowRectEx( &r, style, false, exStyle );

            ::SetWindowPos( hWnd, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER );
        }
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

    zp_size_t Platform::GetMemoryPageSize( const zp_size_t size ) const
    {
        const zp_size_t systemPageSize = m_systemPageSize;

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

    void Platform::GetCurrentDir( char* path, zp_size_t maxPathLength )
    {
        ::GetCurrentDirectory( maxPathLength, path );
    }

    zp_bool_t Platform::FileExists( const char* filePath ) const
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

    zp_handle_t Platform::OpenFileHandle( const char* filePath, OpenFileMode openFileMode, CreateFileMode createFileMode, FileCachingMode fileCachingMode )
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
        return fileHandle;
    }

    zp_handle_t Platform::OpenTempFileHandle( const char* tempFileNamePrefix, const char* tempFileNameExtension, FileCachingMode fileCachingMode )
    {
        char tempPath[MAX_PATH];
        zp_size_t len = ::GetTempPath( ZP_ARRAY_SIZE( tempPath ), tempPath );

        MutableFixedString<MAX_PATH> tempRootPath;
        tempRootPath.format( "%s%c%s%c", tempPath, PathSep, "ZeroPoint", PathSep );

        //char tempRootPath[MAX_PATH];
        //zp_snprintf( tempRootPath, "%s%c%s%c", tempPath, '\\', "ZeroPoint " ZP_VERSION, '\\' );

        ::CreateDirectory( tempRootPath.c_str(), nullptr );

        char tempFileName[MAX_PATH];
        const zp_time_t unique = zp_time_now();
        zp_snprintf( tempFileName, "%s-%x.%s", tempFileNamePrefix ? tempFileNamePrefix : "tmp", unique, tempFileNameExtension ? tempFileNameExtension : "tmp" );

        char finalFileNamePath[MAX_PATH];
        zp_snprintf( finalFileNamePath, "%s%c%s", tempRootPath, PathSep, tempFileName );

        DWORD attributes = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;

        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_SEQUENTIAL, FILE_FLAG_SEQUENTIAL_SCAN );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_RANDOM_ACCESS, FILE_FLAG_RANDOM_ACCESS );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_NO_BUFFERING, FILE_FLAG_NO_BUFFERING );
        TestFlag( attributes, fileCachingMode, ZP_FILE_CACHING_MODE_WRITE_THROUGH, FILE_FLAG_WRITE_THROUGH );

        zp_handle_t fileHandle = ::CreateFile(
            finalFileNamePath,
            FILE_ALL_ACCESS,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            CREATE_ALWAYS,
            attributes,
            nullptr );
        return fileHandle;
    }

    void Platform::SeekFile( zp_handle_t fileHandle, const zp_ptrdiff_t distanceToMoveInBytes, const MoveMethod moveMethod )
    {
        LARGE_INTEGER distance {
            .QuadPart = distanceToMoveInBytes,
        };

        constexpr DWORD moveMethodMapping[] {
            FILE_BEGIN,
            FILE_CURRENT,
            FILE_END
        };

        ::SetFilePointerEx( fileHandle, distance, nullptr, moveMethodMapping[ moveMethod ] );
    }

    zp_size_t Platform::GetFileSize( zp_handle_t fileHandle ) const
    {
        LARGE_INTEGER fileSize;
        const BOOL ok = ::GetFileSizeEx( fileHandle, &fileSize );
        return ok ? static_cast<zp_size_t>(fileSize.QuadPart) : 0;
    }

    void Platform::CloseFileHandle( zp_handle_t fileHandle )
    {
        ::CloseHandle( fileHandle );
    }

    zp_size_t Platform::ReadFile( zp_handle_t fileHandle, void* buffer, const zp_size_t bytesToRead )
    {
        DWORD bytesRead = 0;
        const BOOL ok = ::ReadFile( fileHandle, buffer, bytesToRead, &bytesRead, nullptr );
        return ok ? bytesRead : 0;
    }

    zp_size_t Platform::WriteFile( zp_handle_t fileHandle, const void* data, const zp_size_t size )
    {
        DWORD bytesWritten = 0;
        const BOOL ok = ::WriteFile( fileHandle, data, size, &bytesWritten, nullptr );

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

    zp_handle_t Platform::CreateThread( ThreadFunc threadFunc, void* param, const zp_size_t stackSize, zp_uint32_t* threadId )
    {
        DWORD id;
        HANDLE threadHandle = ::CreateThread(
            nullptr,
            stackSize,
            threadFunc,
            param,
            THREAD_SET_INFORMATION,
            &id );
        ZP_ASSERT( threadHandle );

        if( threadId )
        {
            *threadId = id;
        }

        return threadHandle;
    }

    zp_handle_t Platform::GetCurrentThread() const
    {
        HANDLE threadHandle = ::GetCurrentThread();
        return static_cast<zp_handle_t>( threadHandle );
    }

    zp_uint32_t Platform::GetCurrentThreadId() const
    {
        const DWORD threadId = ::GetCurrentThreadId();
        return static_cast<zp_uint32_t>( threadId );
    }

    zp_uint32_t Platform::GetThreadId( zp_handle_t threadHandle ) const
    {
        const DWORD threadId = ::GetThreadId( static_cast<HANDLE>( threadHandle ) );
        return static_cast<zp_uint32_t>( threadId );
    }

    void Platform::SetThreadName( zp_handle_t threadHandle, const String& threadName )
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
            .dwThreadID = ::GetThreadId( static_cast<HANDLE>( threadHandle ) ),
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

    void Platform::SetThreadPriority( zp_handle_t threadHandle, zp_int32_t priority )
    {
        ::SetThreadPriority( static_cast<HANDLE>( threadHandle ), priority );
    }

    zp_int32_t Platform::GetThreadPriority( zp_handle_t threadHandle )
    {
        const zp_int32_t priority = ::GetThreadPriority( static_cast<HANDLE>( threadHandle) );
        return priority;
    }

    void Platform::SetThreadAffinity( zp_handle_t threadHandle, zp_uint64_t affinityMask )
    {
        ::SetThreadAffinityMask( static_cast<HANDLE>( threadHandle), affinityMask );
    }

    void Platform::SetThreadIdealProcessor( zp_handle_t threadHandle, zp_uint32_t processorIndex )
    {
        processorIndex = zp_clamp<zp_uint32_t>( processorIndex, 0, m_numProcessors );

        const zp_uint64_t requestedMask = 1 << processorIndex;
        if( m_activeProcessorMask & requestedMask )
        {
            PROCESSOR_NUMBER processorNumber {
                .Number = static_cast<BYTE>( 0xFFu & processorIndex )
            };

            ::SetThreadIdealProcessorEx( static_cast<HANDLE>( threadHandle ), &processorNumber, nullptr );
        }
    }

    void Platform::CloseThread( zp_handle_t threadHandle )
    {
        ::CloseHandle( threadHandle );
    }

    void Platform::JoinThreads( zp_handle_t* threadHandles, zp_size_t threadHandleCount )
    {
        ::WaitForMultipleObjects( threadHandleCount, threadHandles, true, INFINITE );
    }

    zp_uint32_t Platform::GetProcessorCount() const
    {
        return m_numProcessors;
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

        int id = ::MessageBoxEx( hWnd, message, title, type, 0 );

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

    zp_int32_t Platform::ExecuteProcess( const char* process, const char* arguments )
    {
        STARTUPINFO startupinfo {
            .cb = sizeof( STARTUPINFO ),
        };

        DWORD returnCode = -1;

        char commandLine[1 KB];
        zp_snprintf( commandLine, "%s %s", process, arguments );

        PROCESS_INFORMATION processInformation {};
        const zp_bool_t ok = ::CreateProcess( nullptr, commandLine, nullptr, nullptr, false, 0, nullptr, nullptr, &startupinfo, &processInformation );
        if( ok )
        {
            ::WaitForSingleObject( processInformation.hProcess, INFINITE );

            ::GetExitCodeProcess( processInformation.hProcess, &returnCode );

            ::CloseHandle( processInformation.hProcess );
            ::CloseHandle( processInformation.hThread );
        }
        else
        {
            DWORD error = ::GetLastError();
            ZP_ASSERT_MSG( false, "Failed to create process" );
        }

        return static_cast<zp_int32_t>( returnCode );
    }
}
