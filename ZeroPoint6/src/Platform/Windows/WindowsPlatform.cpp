//
// Created by phosg on 11/10/2021.
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <processthreadsapi.h>
#include <excpt.h>

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Math.h"
#include "Core/Common.h"
#include "Platform/Platform.h"

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
                    ::PostQuitMessage( 0 );
                }
                    break;

                case WM_DESTROY:
                {
                }
                    break;

                case WM_GETMINMAXINFO:
                {
                    if( windowCallbacks )
                    {
                        auto lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
                        lpMMI->ptMinTrackSize = { windowCallbacks->minWidth, windowCallbacks->minHeight };
                        lpMMI->ptMaxSize = { windowCallbacks->maxWidth, windowCallbacks->maxHeight };
                        lpMMI->ptMaxTrackSize = { windowCallbacks->maxWidth, windowCallbacks->maxHeight };
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
    }

    Platform* GetPlatform()
    {
        return &s_WindowsPlatform;
    }

    zp_handle_t Platform::OpenWindow( OpenWindowDesc* desc )
    {
        HINSTANCE hInstance = desc->instanceHandle ? static_cast<HINSTANCE>(desc->instanceHandle) : ::GetModuleHandle( nullptr );

        WNDCLASSEX wc;
        wc.cbSize = sizeof( WNDCLASSEX );
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.lpfnWndProc = WinProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject( DKGRAY_BRUSH ));
        wc.hIcon = LoadIcon( hInstance, IDI_APPLICATION );
        wc.hIconSm = LoadIcon( hInstance, IDI_APPLICATION );
        wc.hCursor = LoadCursor( hInstance, IDC_ARROW );
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = kZeroPointClassName;

        ATOM reg = ::RegisterClassEx( &wc );
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
        DWORD exStyle = 0;
#if ZP_DEBUG
        exStyle |= WS_EX_ACCEPTFILES;
#endif

        RECT r { 0, 0, desc->width, desc->height };
        ::AdjustWindowRectEx( &r, style, false, exStyle );

        int width = r.right - r.left;
        int height = r.bottom - r.top;

        HWND hWnd = ::CreateWindowEx(
            exStyle,
            kZeroPointClassName,
            desc->title,
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

        ::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)desc->callbacks );

        ::ShowWindow( hWnd, SW_SHOW );
        ::UpdateWindow( hWnd );

        return hWnd;
    }

    zp_bool_t Platform::DispatchWindowMessages( zp_handle_t windowHandle, zp_int32_t* exitCode )
    {
        zp_bool_t isRunning = true;

        MSG msg;
        while( ::PeekMessage( &msg, static_cast<HWND >(windowHandle), 0, 0, PM_REMOVE ) )
        {
            if( msg.message == WM_QUIT )
            {
                if( exitCode )
                {
                    *exitCode = static_cast<int>(msg.wParam);
                }
                isRunning = false;
                break;
            }

            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );
        }

        return isRunning;
    }

    void Platform::CloseWindow( zp_handle_t windowHandle )
    {
        HWND hWnd = static_cast<HWND>( windowHandle);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr( hWnd, GWLP_HINSTANCE );

        ::DestroyWindow( hWnd );
        ::UnregisterClass( "", hInstance );
    }

    void Platform::SetWindowTitle( zp_handle_t windowHandle, const char* title )
    {
        ::SetWindowText( static_cast<HWND>( windowHandle), title );
    }

    void Platform::SetWindowSize( zp_handle_t windowHandle, const zp_int32_t width, const zp_int32_t height )
    {
        HWND hWnd = static_cast<HWND>( windowHandle);
        LONG style = ::GetWindowLong( hWnd, GWL_STYLE );
        LONG exStyle = ::GetWindowLong( hWnd, GWL_EXSTYLE );

        RECT r { 0, 0, width, height };
        ::AdjustWindowRectEx( &r, style, false, exStyle );

        ::SetWindowPos( hWnd, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER );
    }

    void* Platform::AllocateSystemMemory( void* baseAddress, const zp_size_t size )
    {
        SYSTEM_INFO systemInfo {};
        ::GetSystemInfo( &systemInfo );

        const zp_size_t systemPageSize = systemInfo.dwPageSize;

        const zp_size_t requestedSize = size > systemPageSize ? size : systemPageSize;
        const zp_size_t allocationInPageSize = ( requestedSize & ( systemPageSize - 1 ) ) + requestedSize;

        void* ptr = ::VirtualAlloc( baseAddress, allocationInPageSize, MEM_RESERVE, PAGE_NOACCESS );

        return ptr;
    }

    void Platform::FreeSystemMemory( void* ptr )
    {
        ::VirtualFree( ptr, 0, MEM_RELEASE );
    }

    zp_size_t Platform::GetMemoryPageSize( const zp_size_t size ) const
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

        SYSTEM_INFO systemInfo;
        ::GetSystemInfo( &systemInfo );

        const zp_size_t systemPageSize = systemInfo.dwPageSize;

        const zp_size_t requestedSize = size > systemPageSize ? size : systemPageSize;
        const zp_size_t allocationInPageSize = ( requestedSize & ( systemPageSize - 1 ) ) + requestedSize;
        *ptr = (char*)*ptr + allocationInPageSize;
        return page;
    }

    void Platform::DecommitMemoryPage( void* ptr, const zp_size_t size )
    {
        ::VirtualFree( ptr, size, MEM_DECOMMIT );
    }

    zp_handle_t Platform::OpenFileHandle( const char* filePath, OpenFileMode openFileMode, FileCachingMode fileCachingMode )
    {
        DWORD access = 0;
        DWORD shareMode = 0;
        DWORD attributes = 0;

        switch( openFileMode )
        {
            case ZP_OPEN_FILE_MODE_READ:
                access = FILE_GENERIC_READ;
                shareMode = FILE_SHARE_READ;
                attributes = FILE_ATTRIBUTE_READONLY;
                break;

            case ZP_OPEN_FILE_MODE_WRITE:
                access = FILE_GENERIC_WRITE;
                shareMode = FILE_SHARE_WRITE;
                attributes = FILE_ATTRIBUTE_NORMAL;
                break;

            case ZP_OPEN_FILE_MODE_READ_WRITE:
                access = FILE_ALL_ACCESS;
                shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
                attributes = FILE_ATTRIBUTE_NORMAL;
                break;

            default:
                ZP_INVALID_CODE_PATH();
                break;
        }

        if( fileCachingMode & ZP_FILE_CACHING_MODE_SEQUENTIAL )
        {
            attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
        }

        if( fileCachingMode & ZP_FILE_CACHING_MODE_RANDOM_ACCESS )
        {
            attributes |= FILE_FLAG_RANDOM_ACCESS;
        }

        if( fileCachingMode & ZP_FILE_CACHING_MODE_NO_BUFFERING )
        {
            attributes |= FILE_FLAG_NO_BUFFERING;
        }

        if( fileCachingMode & ZP_FILE_CACHING_MODE_WRITE_THROUGH )
        {
            attributes |= FILE_FLAG_WRITE_THROUGH;
        }

        zp_handle_t fileHandle = ::CreateFile(
            filePath,
            access,
            shareMode,
            nullptr,
            OPEN_EXISTING,
            attributes,
            nullptr );
        return fileHandle;
    }

    zp_handle_t Platform::OpenTempFileHandle( FileCachingMode fileCachingMode )
    {
        char pathBuffer[MAX_PATH];
        char fileNameBuffer[MAX_PATH];

        const UINT unique = 0x000000000000FFFF & zp_time_now();
        const DWORD pathLength = ::GetTempPath( ZP_ARRAY_SIZE( pathBuffer ), pathBuffer );
        ::GetTempFileName( pathBuffer, "tmp", unique, fileNameBuffer );

        DWORD attributes = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;

        if( fileCachingMode & ZP_FILE_CACHING_MODE_SEQUENTIAL )
        {
            attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
        }

        if( fileCachingMode & ZP_FILE_CACHING_MODE_RANDOM_ACCESS )
        {
            attributes |= FILE_FLAG_RANDOM_ACCESS;
        }

        if( fileCachingMode & ZP_FILE_CACHING_MODE_NO_BUFFERING )
        {
            attributes |= FILE_FLAG_NO_BUFFERING;
        }

        if( fileCachingMode & ZP_FILE_CACHING_MODE_WRITE_THROUGH )
        {
            attributes |= FILE_FLAG_WRITE_THROUGH;
        }

        zp_handle_t fileHandle = ::CreateFile(
            fileNameBuffer,
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
        LARGE_INTEGER distance = {};
        distance.QuadPart = distanceToMoveInBytes;

        constexpr DWORD moveMethodMapping[] = {
            FILE_BEGIN,
            FILE_CURRENT,
            FILE_END
        };

        ::SetFilePointerEx( fileHandle, distance, nullptr, moveMethodMapping[ moveMethod ] );
    }

    zp_size_t Platform::GetFileSize( zp_handle_t fileHandle ) const
    {
        LARGE_INTEGER fileSize {};
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
        ZP_ASSERT( h );

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

        auto callbackEnviron = static_cast<PTP_CALLBACK_ENVIRON>( mem);
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
        auto callbackEnviron = static_cast<PTP_CALLBACK_ENVIRON>( threadPool);
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

    void Platform::SetThreadName( zp_handle_t threadHandle, const char* threadName )
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

        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = ::GetThreadId( static_cast<HANDLE>( threadHandle ) );
        info.dwFlags = 0;

#pragma warning(push)
#pragma warning(disable: 6320 6322)
        try
        {
            const DWORD MS_VC_EXCEPTION = 0x406D1388;
            RaiseException( MS_VC_EXCEPTION, 0, sizeof( info ) / sizeof( ULONG_PTR ), (ULONG_PTR*)&info );
        }
        catch( ... )
        {
        }
#pragma warning(pop)
    }

    void Platform::SetThreadPriority( zp_handle_t threadHandle, zp_int32_t priority )
    {
        ::SetThreadPriority( static_cast<HANDLE>( threadHandle), priority );
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
        SYSTEM_INFO systemInfo;
        ::GetSystemInfo( &systemInfo );

        processorIndex = zp_clamp<zp_uint32_t>( processorIndex, 0, systemInfo.dwNumberOfProcessors );

        const zp_uint64_t requestedMask = 1 << processorIndex;
        if( systemInfo.dwActiveProcessorMask & requestedMask )
        {
            PROCESSOR_NUMBER processorNumber {};
            processorNumber.Number = processorIndex;

            ::SetThreadIdealProcessorEx( static_cast<HANDLE>( threadHandle), &processorNumber, nullptr );
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

    zp_uint32_t Platform::GetProcessorCount()
    {
        SYSTEM_INFO info;
        ::GetSystemInfo( &info );
        return info.dwNumberOfProcessors;
    }

    zp_int32_t Platform::ShowMessageBox( zp_handle_t windowHandle, const char* title, const char* message )
    {
        HWND hWnd = static_cast<HWND>( windowHandle );
        UINT type = MB_ICONINFORMATION | MB_ABORTRETRYIGNORE | MB_DEFBUTTON1;

        int id = ::MessageBox( hWnd, message, title, type );

        zp_int32_t result = -1;
        switch( id )
        {
            case IDABORT:
                result = 0;
                break;
            case IDRETRY:
                result = 1;
                break;
            case IDIGNORE:
                result = 2;
                break;
        }

        return result;
    }
}