#ifndef ZP_PLATFORM_H
#define ZP_PLATFORM_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"

namespace zp
{
    typedef unsigned long (* __stdcall ThreadFunc)( void* );

    typedef zp_int64_t (* ProcAddressFunc)();

    typedef void (* OnWindowResize)( zp_handle_t windowHandle, zp_int32_t width, zp_int32_t height );
    typedef void (* OnWindowFocus)( zp_handle_t windowHandle, zp_bool_t isNowFocused );

    struct WindowCallbacks
    {
        zp_int32_t minWidth, minHeight, maxWidth, maxHeight;
        OnWindowResize onWindowResize;
        OnWindowFocus onWindowFocus;
    };

    struct OpenWindowDesc
    {
        zp_handle_t instanceHandle;
        WindowCallbacks* callbacks;
        const char* title;
        zp_int32_t width;
        zp_int32_t height;
    };

    class Platform
    {
    ZP_NONCOPYABLE( Platform );

    public:
        Platform() = default;

        ~Platform() = default;

        zp_handle_t OpenWindow( OpenWindowDesc* desc );

        zp_bool_t DispatchWindowMessages( zp_handle_t windowHandle, zp_int32_t* exitCode );

        void CloseWindow( zp_handle_t windowHandle );

        void SetWindowTitle( zp_handle_t windowHandle, const char* title );

        void SetWindowSize( zp_handle_t windowHandle, zp_int32_t width, zp_int32_t height );

        void* AllocateSystemMemory( void* baseAddress, zp_size_t size );

        void FreeSystemMemory( void* ptr );

        zp_size_t GetMemoryPageSize( const zp_size_t size ) const;

        void* CommitMemoryPage( void** ptr, zp_size_t size );

        void DecommitMemoryPage( void* ptr, zp_size_t size );

        zp_handle_t OpenFileHandle( const char* filePath );

        void SeekFile( zp_handle_t fileHandle, zp_ptrdiff_t distanceToMoveInBytes, int moveMethod );

        zp_size_t GetFileSize( zp_handle_t fileHandle ) const;

        void CloseFileHandle( zp_handle_t fileHandle );

        zp_size_t ReadFile( zp_handle_t fileHandle, void* buffer, zp_size_t bytesToRead );

        zp_handle_t LoadExternalLibrary( const char* libraryPath );

        void UnloadExternalLibrary( zp_handle_t libraryHandle );

        ProcAddressFunc GetProcAddress( zp_handle_t libraryHandle, const char* name );

        template<typename T>
        T GetProcAddress( zp_handle_t libraryHandle, const char* name )
        {
            return (T)GetProcAddress( libraryHandle, name );
        }

        zp_handle_t AllocateThreadPool( zp_uint32_t minThreads, zp_uint32_t maxThreads );

        void FreeThreadPool( zp_handle_t threadPool );

        zp_handle_t CreateThread( ThreadFunc threadFunc, void* param, zp_size_t stackSize, zp_uint32_t* threadId );

        zp_handle_t GetCurrentThread() const;

        zp_uint32_t GetCurrentThreadId() const;

        zp_uint32_t GetThreadId( zp_handle_t threadHandle ) const;

        void SetThreadName( zp_handle_t threadHandle, const char* threadName );

        void SetThreadPriority( zp_handle_t threadHandle, zp_int32_t priority );

        zp_int32_t GetThreadPriority( zp_handle_t threadHandle );

        void SetThreadAffinity( zp_handle_t threadHandle, zp_uint64_t affinityMask );

        void SetThreadIdealProcessor( zp_handle_t threadHandle, zp_uint32_t processorIndex );

        void CloseThread( zp_handle_t threadHandle );

        void JoinThreads( zp_handle_t* threadHandles, zp_size_t threadHandleCount );

        zp_uint32_t GetProcessorCount();
    };

    Platform* GetPlatform();
}

#endif //ZP_PLATFORM_H
