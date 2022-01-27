//
// Created by phosg on 11/10/2021.
//

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Platform/Platform.h"

namespace zp
{
    namespace
    {
        Platform s_WindowsPlatform;
    }

    Platform* GetPlatform()
    {
        return &s_WindowsPlatform;
    }

    void Platform::SetWindowTitle( zp_handle_t windowHandle, const char* title )
    {
        ::SetWindowText( static_cast<HWND>( windowHandle), title );
    }

    void Platform::SetWindowSize( zp_handle_t windowHandle, const zp_uint32_t width, const zp_uint32_t height )
    {
        HWND hWnd = static_cast<HWND>( windowHandle);
        LONG style = ::GetWindowLong( hWnd, GWL_STYLE);
        LONG exStyle = ::GetWindowLong( hWnd, GWL_EXSTYLE);

        RECT r = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        ::AdjustWindowRectEx( &r, style, false, exStyle );

        ::SetWindowPos( hWnd, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER );
    }

    void* Platform::AllocateSystemMemory( void* baseAddress, const zp_size_t size )
    {
        SYSTEM_INFO systemInfo {};
        ::GetSystemInfo( &systemInfo );

        const zp_size_t systemPageSize = systemInfo.dwPageSize;

        const zp_size_t requestedSize = size > systemPageSize ? size : systemPageSize;
        const zp_size_t allocationInPageSize = (requestedSize & (systemPageSize - 1)) + requestedSize;

        void* ptr = ::VirtualAlloc( baseAddress, allocationInPageSize, MEM_RESERVE, PAGE_NOACCESS );

        return ptr;
    }

    void Platform::FreeSystemMemory( void* ptr )
    {
        ::VirtualFree( ptr, 0, MEM_RELEASE );
    }

    zp_size_t Platform::GetMemoryPageSize( const zp_size_t size ) const
    {
        SYSTEM_INFO systemInfo {};
        ::GetSystemInfo( &systemInfo );

        const zp_size_t systemPageSize = systemInfo.dwPageSize;

        const zp_size_t requestedSize = size > systemPageSize ? size : systemPageSize;
        const zp_size_t allocationInPageSize = (requestedSize & (systemPageSize - 1)) + requestedSize;
        return allocationInPageSize;
    }

    void* Platform::CommitMemoryPage( void** ptr, const zp_size_t size )
    {
        void* page = ::VirtualAlloc( *ptr, size, MEM_COMMIT, PAGE_READWRITE );

        SYSTEM_INFO systemInfo {};
        ::GetSystemInfo( &systemInfo );

        const zp_size_t systemPageSize = systemInfo.dwPageSize;

        const zp_size_t requestedSize = size > systemPageSize ? size : systemPageSize;
        const zp_size_t allocationInPageSize = (requestedSize & (systemPageSize - 1)) + requestedSize;
        *ptr = (char*)*ptr + allocationInPageSize;
        return page;
    }

    void Platform::DecommitMemoryPage( void* ptr, const zp_size_t size )
    {
        ::VirtualFree( ptr, size, MEM_DECOMMIT );
    }

    zp_handle_t Platform::OpenFileHandle( const char* filePath )
    {
        zp_handle_t fileHandle = ::CreateFile(
            filePath,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY,
            nullptr );
        return fileHandle;
    }

    void Platform::SeekFile( zp_handle_t fileHandle, const zp_ptrdiff_t distanceToMoveInBytes, const int moveMethod )
    {
        LARGE_INTEGER distance = {};
        distance.QuadPart = distanceToMoveInBytes;

        const DWORD moveMethodMapping[] = {
            FILE_BEGIN,
            FILE_CURRENT,
            FILE_END
        };

        ::SetFilePointerEx( fileHandle, distance, nullptr, moveMethodMapping[ moveMethod ] );
    }

    zp_size_t Platform::GetFileSize( zp_handle_t fileHandle ) const
    {
        LARGE_INTEGER fileSize = {};
        BOOL ok = ::GetFileSizeEx( fileHandle, &fileSize );
        return ok ? static_cast<zp_size_t>(fileSize.QuadPart) : 0;
    }

    void Platform::CloseFileHandle( zp_handle_t fileHandle )
    {
        ::CloseHandle( fileHandle );
    }

    zp_size_t Platform::ReadFile( zp_handle_t fileHandle, void* buffer, const zp_size_t bytesToRead )
    {
        DWORD bytesRead = 0;
        BOOL ok = ::ReadFile( fileHandle, buffer, bytesToRead, &bytesRead, nullptr );

        return ok ? bytesRead : 0;
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
            ::FreeLibrary( static_cast<HMODULE>(libraryHandle));
        }
    }

    zp_handle_t Platform::AllocateThreadPool( zp_uint32_t minThreads, zp_uint32_t maxThreads )
    {
        void* mem = ::HeapAlloc( ::GetProcessHeap(), HEAP_NO_SERIALIZE, sizeof( TP_CALLBACK_ENVIRON ));

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
            0,
            &id );
        ZP_ASSERT( threadHandle );

        if( threadId )
        {
            *threadId = id;
        }

        return threadHandle;
    }

    void Platform::CloseThread( zp_handle_t threadHandle )
    {
        ::CloseHandle( threadHandle );
    }

    void Platform::JoinThreads( zp_handle_t* threadHandles, zp_size_t threadHandleCount )
    {
        ::WaitForMultipleObjects( threadHandleCount, threadHandles, true, INFINITE );
    }
}