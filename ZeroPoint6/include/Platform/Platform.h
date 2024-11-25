#ifndef ZP_PLATFORM_H
#define ZP_PLATFORM_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/String.h"

namespace zp
{
    typedef zp_uint32_t (* __stdcall ThreadFunc)( void* );

    typedef zp_int64_t (* ProcAddressFunc)();

    typedef void (* OnWindowGetMinMaxSizeFunc)( zp_handle_t windowHandle, zp_int32_t& minWidth, zp_int32_t& minHeight, zp_int32_t& maxWidth, zp_int32_t& maxHeight, void* userPtr );

    typedef void (* OnWindowResizeFunc)( zp_handle_t windowHandle, zp_int32_t width, zp_int32_t height, void* userPtr );

    typedef void (* OnWindowFocusFunc)( zp_handle_t windowHandle, zp_bool_t isNowFocused, void* userPtr );

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

    typedef void (* OnWindowKeyEvent)( zp_handle_t windowHandle, const WindowKeyEvent& windowKeyEvent, void* userPtr );

    struct WindowMouseEvent
    {
        zp_int32_t x;
        zp_int32_t y;
        zp_int32_t zDelta;
        ZP_BOOL32( isCtrlDown );
        ZP_BOOL32( isShiftDown );
    };

    typedef void (* OnWindowMouseEvent)( zp_handle_t windowHandle, const WindowMouseEvent& windowMouseEvent, void* userPtr );

    typedef void (* OnWindowClosed)( zp_handle_t windowHandle, void* userPtr );

    typedef void (* OnWindowHelp)( zp_handle_t windowHandle, void* userPtr );

    struct WindowCallbacks
    {
        zp_int32_t minWidth, minHeight, maxWidth, maxHeight;
        OnWindowGetMinMaxSizeFunc onWindowGetMinMaxSize;
        OnWindowResizeFunc onWindowResize;
        OnWindowFocusFunc onWindowFocus;
        OnWindowKeyEvent onWindowKeyEvent;
        OnWindowMouseEvent onWindowMouseEvent;
        OnWindowHelp onWindowHelpEvent;
        OnWindowClosed onWindowClosed;
        void* userPtr;
    };

    struct OpenWindowDesc
    {
        zp_handle_t instanceHandle;
        WindowCallbacks* callbacks;
        const char* title;
        zp_int32_t width;
        zp_int32_t height;
        zp_bool_t showWindow;
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
        CreateFileMode_Count,
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

    enum class AddressFamily
    {
        IPv4,
        IPv6,
        NetBIOS,
        Bluetooth,
    };

    enum class SocketType
    {
        Stream,
        Datagram,
        Raw,
    };

    enum class ConnectionProtocol
    {
        TCP,
        UDP,
    };

    enum class SocketDirection
    {
        Connect,
        Listen,
    };

    struct IPAddress
    {
        char addr[16];
        zp_uint16_t port;

        static IPAddress Localhost( zp_uint16_t port )
        {
            return {
                .addr = "127.0.0.1",
                .port = port
            };
        }
    };

    struct SocketDesc
    {
        const char* name;
        IPAddress address;
        AddressFamily addressFamily;
        SocketType socketType;
        ConnectionProtocol connectionProtocol;
        SocketDirection socketDirection;

    };

    struct WindowHandle
    {
        zp_handle_t handle;

        ZP_FORCEINLINE explicit operator zp_bool_t() const
        {
            return handle != nullptr;
        }
    };

    enum class WindowFullscreenType
    {
        Window,
        BorderlessWindow,
        Fullscreen,
    };

    // Window
    namespace Platform
    {
        WindowHandle OpenWindow( const OpenWindowDesc& desc );

        zp_bool_t DispatchWindowMessages( WindowHandle windowHandle, zp_int32_t& exitCode );

        zp_bool_t DispatchMessages( zp_int32_t& exitCode );

        void ShowWindow( WindowHandle windowHandle, zp_bool_t show );

        void CloseWindow( WindowHandle windowHandle );

        void SetWindowTitle( WindowHandle windowHandle, const String& title );

        void SetWindowSize( WindowHandle windowHandle, zp_int32_t width, zp_int32_t height, WindowFullscreenType windowFullscreenType = WindowFullscreenType::Window );
    }

    struct OpenSystemTrayDesc
    {
    };

    struct SystemTrayHandle
    {
        zp_handle_t handle;
    };

    namespace Platform
    {
        SystemTrayHandle OpenSystemTray( const OpenSystemTrayDesc& desc );

        void CloseSystemTray( SystemTrayHandle systemTrayHandle );
    }

    struct ConsoleHandle
    {
        zp_handle_t handle;

        ZP_FORCEINLINE explicit operator zp_bool_t() const
        {
            return handle != nullptr;
        }
    };

    // Console
    namespace Platform
    {
        ConsoleHandle OpenConsole();

        zp_bool_t CloseConsole( ConsoleHandle );

        zp_bool_t SetConsoleTitle( ConsoleHandle, const String& title );
    }

    // Memory
    namespace Platform
    {
        void* AllocateSystemMemory( void* baseAddress, zp_size_t size );

        void FreeSystemMemory( void* ptr );

        [[nodiscard]] zp_size_t GetMemoryPageSize( zp_size_t size );

        void* CommitMemoryPage( void** ptr, zp_size_t size );

        void DecommitMemoryPage( void* ptr, zp_size_t size );
    }

    struct FileHandle
    {
        zp_handle_t handle;

        ZP_FORCEINLINE explicit operator zp_bool_t() const
        {
            return handle != nullptr;
        }
    };

    // File & Path
    namespace Platform
    {
        zp_size_t GetCurrentDir( char* path, zp_size_t maxPathLength );

        String GetCurrentDirStr( char* path, zp_size_t maxPathLength );

        template<zp_size_t Size>
        zp_size_t GetCurrentDir( char (& path)[Size] )
        {
            return GetCurrentDir( path, Size );
        }

        template<zp_size_t Size>
        String GetCurrentDirStr( char (& path)[Size] )
        {
            return GetCurrentDirStr( path, Size );
        }

        enum class CreateDirResult
        {
            Success,
            ErrorDirectoryAlreadyExists = -1,
            ErrorPathNotFound = -2,
        };

        zp_bool_t CreateDirectory( const char* path );

        zp_bool_t DirectoryExists( const char* path );

        zp_bool_t FileExists( const char* filePath );

        zp_bool_t FileCopy( const char* srcFilePath, const char* dstFilePath, zp_bool_t force = false );

        zp_bool_t FileMove( const char* srcFilePath, const char* dstFilePath );

        FileHandle OpenFileHandle( const char* filePath, OpenFileMode openFileMode, CreateFileMode createFileMode = ZP_CREATE_FILE_MODE_OPEN, FileCachingMode fileCachingMode = ZP_FILE_CACHING_MODE_DEFAULT );

        FileHandle OpenTempFileHandle( const char* tempFileNamePrefix = nullptr, const char* tempFileNameExtension = nullptr, FileCachingMode fileCachingMode = ZP_FILE_CACHING_MODE_DEFAULT );

        void SeekFile( FileHandle fileHandle, zp_ptrdiff_t distanceToMoveInBytes, MoveMethod moveMethod );

        zp_size_t GetFileSize( FileHandle fileHandle );

        void CloseFileHandle( FileHandle fileHandle );

        zp_size_t ReadFile( FileHandle fileHandle, void* buffer, zp_size_t bytesToRead );

        zp_size_t WriteFile( FileHandle fileHandle, const void* data, zp_size_t size );

        extern zp_char8_t PathSep;
    }

    // Process
    namespace Platform
    {
        zp_handle_t LoadExternalLibrary( const char* libraryPath );

        void UnloadExternalLibrary( zp_handle_t libraryHandle );

        ProcAddressFunc GetProcAddress( zp_handle_t libraryHandle, const char* name );

        template<typename T>
        T GetProcAddress( zp_handle_t libraryHandle, const String& name )
        {
            return (T)GetProcAddress( libraryHandle, name.c_str() );
        }
    }

    struct ThreadHandle
    {
        zp_handle_t handle;

        ZP_FORCEINLINE explicit operator zp_bool_t() const
        {
            return handle != nullptr;
        }
    };

    // Threading
    namespace Platform
    {
        zp_handle_t AllocateThreadPool( zp_uint32_t minThreads, zp_uint32_t maxThreads );

        void FreeThreadPool( zp_handle_t threadPool );

        ThreadHandle CreateThread( ThreadFunc threadFunc, void* param, zp_size_t stackSize, zp_uint32_t* outThreadId );

        [[nodiscard]] ThreadHandle GetCurrentThread();

        [[nodiscard]] zp_uint32_t GetCurrentThreadId();

        void YieldCurrentThread();

        void SleepCurrentThread( zp_uint64_t milliseconds );

        zp_uint32_t GetThreadId( ThreadHandle threadHandle );

        void SetThreadName( ThreadHandle threadHandle, const String& threadName );

        void SetThreadPriority( ThreadHandle threadHandle, zp_int32_t priority );

        zp_int32_t GetThreadPriority( ThreadHandle threadHandle );

        void SetThreadAffinity( ThreadHandle threadHandle, zp_uint64_t affinityMask );

        void SetThreadIdealProcessor( ThreadHandle threadHandle, zp_uint32_t processorIndex );

        void CloseThread( ThreadHandle threadHandle );

        void JoinThreads( ThreadHandle* threadHandles, zp_size_t threadHandleCount );

        [[nodiscard]] zp_uint32_t GetProcessorCount();

        zp_int32_t ExecuteProcess( const char* process, const char* arguments );
    }

    struct Semaphore
    {
        zp_handle_t handle;

        ZP_FORCEINLINE explicit operator zp_bool_t() const
        {
            return handle != nullptr;
        }
    };

    // Semaphore
    namespace Platform
    {
        enum class AcquireSemaphoreResult
        {
            Signaled,
            NotSignaled,
            Abandoned,
            Failed,
        };

        Semaphore CreateSemaphore( zp_int32_t initialCount, zp_int32_t maxCount, const char* name = nullptr );

        AcquireSemaphoreResult AcquireSemaphore( Semaphore semaphore, zp_time_t millisecondTimeout = 0 );

        zp_int32_t ReleaseSemaphore( Semaphore semaphore, zp_int32_t releaseCount = 1 );

        zp_bool_t CloseSemaphore( Semaphore semaphore );
    }

    struct Mutex
    {
        zp_handle_t handle;

        ZP_FORCEINLINE explicit operator zp_bool_t() const
        {
            return handle != nullptr;
        }
    };

    enum class AcquireMutexResult
    {
        Acquired,
        Abandoned,
        Failed,
    };

    // Mutex
    namespace Platform
    {
        Mutex CreateMutex( zp_bool_t initialOwner, const char* name = nullptr );

        AcquireMutexResult AcquireMutex( Mutex mutex, zp_time_t millisecondTimeout = ZP_TIME_INFINITE );

        zp_bool_t ReleaseMutex( Mutex mutex );

        zp_bool_t CloseMutex( Mutex mutex );
    }

    namespace
    {
        const char* kDefaultDateTimeFormat = "%Y-%m-%d %H:%M:%S";
    }

    struct DateTime
    {
        zp_int32_t year;
        zp_int32_t month;
        zp_int32_t day;
        zp_int32_t hour;
        zp_int32_t minute;
        zp_int32_t second;
        zp_int32_t microseconds;
        zp_int32_t year_day;
        zp_int32_t week_day;
        zp_int32_t is_dst;
    };

    // Time
    namespace Platform
    {
        [[nodiscard]] zp_time_t TimeNow();

        [[nodiscard]] zp_time_t TimeFrequency();

        [[nodiscard]] zp_uint64_t TimeCycles();

        [[nodiscard]] DateTime DateTimeNowLocal();

        [[nodiscard]] DateTime DateTimeNowUTC();

        zp_size_t DateTimeToString( const DateTime& dateTime, char* str, zp_size_t length, const char* format = kDefaultDateTimeFormat ); //"%d-%m-%Y %H:%M:%S" );

        template<zp_size_t Size>
        zp_size_t DateTimeToString( const DateTime& dateTime, char (& str)[Size], const char* format = kDefaultDateTimeFormat )
        {
            return DateTimeToString( dateTime, str, Size, format );
        }
    }

    // Util
    namespace Platform
    {
        MessageBoxResult ShowMessageBox( zp_handle_t windowHandle, const char* title, const char* message, MessageBoxType messageBoxType, MessageBoxButton messageBoxButton );

        void DebugBreak();
    }

    constexpr zp_ptr_t ZP_INVALID_SOCKET = ~0;

    struct Socket
    {
        zp_ptr_t handle = ZP_INVALID_SOCKET;

        ZP_FORCEINLINE explicit operator zp_bool_t() const
        {
            return handle != ZP_INVALID_SOCKET;
        }
    };

    // Networking
    namespace Platform
    {
        zp_bool_t InitializeNetworking();

        void ShutdownNetworking();

        Socket OpenSocket( const SocketDesc& desc );

        Socket AcceptSocket( Socket socket );

        zp_size_t ReceiveSocket( Socket socket, void* dst, zp_size_t dstSize );

        zp_size_t SendSocket( Socket socket, const void* src, zp_size_t srcSize );

        void CloseSocket( Socket socket );
    };

#pragma region Path

    class FilePath
    {
    public:
        enum
        {
            kMaxFilePath = 512
        };

        FilePath() = default;

        ~FilePath() = default;

        FilePath& operator=( const FilePath& other )
        {
            if( this != &other )
            {
                m_path = other.m_path;
            }
            return *this;
        }

        FilePath& operator=( FilePath&& other ) noexcept
        {
            m_path = zp_move( other.m_path );
            return *this;
        }

        [[nodiscard]] zp_bool_t exists() const
        {
            const zp_bool_t found = isFile() || isDirectory();
            return found;
        }

        [[nodiscard]] zp_bool_t isFile() const
        {
            return Platform::FileExists( m_path.c_str() );
        }

        [[nodiscard]] zp_bool_t isDirectory() const
        {
            return Platform::DirectoryExists( m_path.c_str() );
        }

        FilePath& operator/( const char* other )
        {
            m_path.append( Platform::PathSep );
            m_path.append( other );
            return *this;
        }

        FilePath& operator/( const String& other )
        {
            m_path.append( Platform::PathSep );
            m_path.append( other );
            return *this;
        }

        FilePath& operator/( const FilePath& other )
        {
            m_path.append( Platform::PathSep );
            m_path.append( other.m_path );
            return *this;
        }

        FilePath& operator+( const char* other )
        {
            m_path.append( other );
            return *this;
        }

        FilePath& operator+( const String& other )
        {
            m_path.append( other );
            return *this;
        }

        FilePath& operator+( const FilePath& other )
        {
            m_path.append( other.m_path );
            return *this;
        }

        [[nodiscard]] ZP_FORCEINLINE const char* c_str() const
        {
            return m_path.c_str();
        }

        [[nodiscard]] ZP_FORCEINLINE char* str()
        {
            return m_path.mutable_str();
        }

    private:
        MutableFixedString<kMaxFilePath> m_path;
    };

#pragma endregion

}

#endif //ZP_PLATFORM_H
