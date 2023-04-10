#ifndef ZP_PLATFORM_H
#define ZP_PLATFORM_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"

namespace zp
{
    typedef unsigned long (* __stdcall ThreadFunc)( void* );

    typedef zp_int64_t (* ProcAddressFunc)();

    typedef void (* OnWindowGetMinMaxSize)( zp_handle_t windowHandle, zp_int32_t& minWidth, zp_int32_t& minHeight, zp_int32_t& maxWidth, zp_int32_t& maxHeight );

    typedef void (* OnWindowResize)( zp_handle_t windowHandle, zp_int32_t width, zp_int32_t height );

    typedef void (* OnWindowFocus)( zp_handle_t windowHandle, zp_bool_t isNowFocused );

    struct WindowKeyEvent
    {
        zp_uint32_t keyCode;
        zp_uint32_t repeatCount;
        ZP_BOOL32( isCtrlDown );
        ZP_BOOL32( isShiftDown );
        ZP_BOOL32( isAltDown );
        ZP_BOOL32( wasKeyDown );
        ZP_BOOL32( isKeyReleased );
    };

    typedef void (* OnWindowKeyEvent)( zp_handle_t windowHandle, const WindowKeyEvent& windowKeyEvent );

    struct WindowMouseEvent
    {
        zp_int32_t x;
        zp_int32_t y;
        zp_int32_t zDelta;
        ZP_BOOL32( isCtrlDown );
        ZP_BOOL32( isShiftDown );
    };

    typedef void (* OnWindowMouseEvent)( zp_handle_t windowHandle, const WindowMouseEvent& windowMouseEvent );

    struct WindowCallbacks
    {
        zp_int32_t minWidth, minHeight, maxWidth, maxHeight;
        OnWindowGetMinMaxSize onWindowGetMinMaxSize;
        OnWindowResize onWindowResize;
        OnWindowFocus onWindowFocus;
        OnWindowKeyEvent onWindowKeyEvent;
        OnWindowMouseEvent onWindowMouseEvent;
    };

    struct OpenWindowDesc
    {
        zp_handle_t instanceHandle;
        WindowCallbacks* callbacks;
        const char* title;
        zp_int32_t width;
        zp_int32_t height;
    };

    enum OpenFileMode
    {
        ZP_OPEN_FILE_MODE_READ,
        ZP_OPEN_FILE_MODE_WRITE,
        ZP_OPEN_FILE_MODE_READ_WRITE,
    };

    enum CreateFileMode
    {
        ZP_CREATE_FILE_MODE_OPEN,
        ZP_CREATE_FILE_MODE_OPEN_NEW,
        ZP_CREATE_FILE_MODE_CREATE,
        ZP_CREATE_FILE_MODE_CREATE_NEW,
        ZP_CREATE_FILE_MODE_TRUNCATE,
    };

    enum FileCachingMode
    {
        ZP_FILE_CACHING_MODE_DEFAULT = 0,
        ZP_FILE_CACHING_MODE_SEQUENTIAL = 1 << 0,
        ZP_FILE_CACHING_MODE_RANDOM_ACCESS = 1 << 1,
        ZP_FILE_CACHING_MODE_NO_BUFFERING = 1 << 2,
        ZP_FILE_CACHING_MODE_WRITE_THROUGH = 1 << 3,
    };

    enum MoveMethod
    {
        ZP_MOVE_METHOD_BEGIN,
        ZP_MOVE_METHOD_CURRENT,
        ZP_MOVE_METHOD_END,
    };

    enum MessageBoxType
    {
        ZP_MESSAGE_BOX_TYPE_INFO,
        ZP_MESSAGE_BOX_TYPE_WARNING,
        ZP_MESSAGE_BOX_TYPE_ERROR,
    };

    enum MessageBoxButton
    {
        ZP_MESSAGE_BOX_BUTTON_OK,
        ZP_MESSAGE_BOX_BUTTON_OK_CANCEL,
        ZP_MESSAGE_BOX_BUTTON_RETRY_CANCEL,
        ZP_MESSAGE_BOX_BUTTON_HELP,
        ZP_MESSAGE_BOX_BUTTON_YES_NO,
        ZP_MESSAGE_BOX_BUTTON_YES_NO_CANCEL,
        ZP_MESSAGE_BOX_BUTTON_ABORT_RETRY_IGNORE,
        ZP_MESSAGE_BOX_BUTTON_CANCEL_TRY_AGAIN_CONTINUE,
    };

    enum MessageBoxResult
    {
        ZP_MESSAGE_BOX_RESULT_OK,
        ZP_MESSAGE_BOX_RESULT_CANCEL,
        ZP_MESSAGE_BOX_RESULT_RETRY,
        ZP_MESSAGE_BOX_RESULT_HELP,
        ZP_MESSAGE_BOX_RESULT_YES,
        ZP_MESSAGE_BOX_RESULT_NO,
        ZP_MESSAGE_BOX_RESULT_CLOSE,
        ZP_MESSAGE_BOX_RESULT_ABORT,
        ZP_MESSAGE_BOX_RESULT_IGNORE,
        ZP_MESSAGE_BOX_RESULT_CONTINUE,
        ZP_MESSAGE_BOX_RESULT_TRY_AGAIN,
    };

    class Platform
    {
    ZP_NONCOPYABLE( Platform );

    public:
        Platform();

        ~Platform() = default;

        zp_handle_t OpenWindow( const OpenWindowDesc* desc );

        zp_bool_t DispatchWindowMessages( zp_handle_t windowHandle, zp_int32_t& exitCode );

        void CloseWindow( zp_handle_t windowHandle );

        void SetWindowTitle( zp_handle_t windowHandle, const char* title );

        void SetWindowSize( zp_handle_t windowHandle, zp_int32_t width, zp_int32_t height );

        void* AllocateSystemMemory( void* baseAddress, zp_size_t size );

        void FreeSystemMemory( void* ptr );

        [[nodiscard]] zp_size_t GetMemoryPageSize( zp_size_t size ) const;

        void* CommitMemoryPage( void** ptr, zp_size_t size );

        void DecommitMemoryPage( void* ptr, zp_size_t size );

        void GetCurrentDir( char* path, zp_size_t maxPathLength );

        template<zp_size_t Size>
        void GetCurrentDir( char (& path)[Size] )
        {
            GetCurrentDir( path, Size );
        }

        zp_handle_t OpenFileHandle( const char* filePath, OpenFileMode openFileMode, CreateFileMode createFileMode = ZP_CREATE_FILE_MODE_OPEN, FileCachingMode fileCachingMode = ZP_FILE_CACHING_MODE_DEFAULT );

        zp_handle_t OpenTempFileHandle( const char* tempFileNamePrefix = nullptr, const char* tempFileNameExtension = nullptr, FileCachingMode fileCachingMode = ZP_FILE_CACHING_MODE_DEFAULT );

        void SeekFile( zp_handle_t fileHandle, zp_ptrdiff_t distanceToMoveInBytes, MoveMethod moveMethod );

        zp_size_t GetFileSize( zp_handle_t fileHandle ) const;

        void CloseFileHandle( zp_handle_t fileHandle );

        zp_size_t ReadFile( zp_handle_t fileHandle, void* buffer, zp_size_t bytesToRead );

        zp_size_t WriteFile( zp_handle_t fileHandle, const void* data, zp_size_t size );

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

        [[nodiscard]] zp_handle_t GetCurrentThread() const;

        [[nodiscard]] zp_uint32_t GetCurrentThreadId() const;

        zp_uint32_t GetThreadId( zp_handle_t threadHandle ) const;

        void SetThreadName( zp_handle_t threadHandle, const char* threadName );

        void SetThreadPriority( zp_handle_t threadHandle, zp_int32_t priority );

        zp_int32_t GetThreadPriority( zp_handle_t threadHandle );

        void SetThreadAffinity( zp_handle_t threadHandle, zp_uint64_t affinityMask );

        void SetThreadIdealProcessor( zp_handle_t threadHandle, zp_uint32_t processorIndex );

        void CloseThread( zp_handle_t threadHandle );

        void JoinThreads( zp_handle_t* threadHandles, zp_size_t threadHandleCount );

        [[nodiscard]] zp_uint32_t GetProcessorCount() const;

        MessageBoxResult ShowMessageBox( zp_handle_t windowHandle, const char* title, const char* message, MessageBoxType messageBoxType, MessageBoxButton messageBoxButton );

    private:
        zp_uint64_t m_activeProcessorMask;
        zp_size_t m_systemPageSize;
        zp_uint32_t m_numProcessors;
    };

    Platform* GetPlatform();
}

#endif //ZP_PLATFORM_H
