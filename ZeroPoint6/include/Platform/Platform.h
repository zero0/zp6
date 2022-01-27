#ifndef ZP_PLATFORM_H
#define ZP_PLATFORM_H

#include "Core/Types.h"
#include "Core/Macros.h"

namespace zp
{
    typedef unsigned long (* __stdcall ThreadFunc)(void*);

    class Platform
    {
    ZP_NONCOPYABLE(Platform);

    public:
        Platform() = default;
        ~Platform() = default;

        zp_handle_t GetWindowHandle() const
        {
            return m_windowHandle;
        }

        void SetWindowHandle(zp_handle_t windowHandle)
        {
            m_windowHandle = windowHandle;
        }

        void SetWindowTitle(zp_handle_t windowHandle, const char* title);

        void SetWindowSize(zp_handle_t windowHandle, zp_uint32_t width, zp_uint32_t height);

        void* AllocateSystemMemory(void* baseAddress, zp_size_t size);

        void FreeSystemMemory(void* ptr);

        zp_size_t GetMemoryPageSize(const zp_size_t size) const;

        void* CommitMemoryPage(void** ptr, zp_size_t size);

        void DecommitMemoryPage(void* ptr, zp_size_t size);

        zp_handle_t OpenFileHandle(const char* filePath);

        void SeekFile(zp_handle_t fileHandle, zp_ptrdiff_t distanceToMoveInBytes, int moveMethod);

        zp_size_t GetFileSize(zp_handle_t fileHandle) const;

        void CloseFileHandle(zp_handle_t fileHandle);

        zp_size_t ReadFile(zp_handle_t fileHandle, void* buffer, zp_size_t bytesToRead);

        zp_handle_t LoadExternalLibrary(const char* libraryPath);

        void UnloadExternalLibrary(zp_handle_t libraryHandle);

        zp_handle_t AllocateThreadPool(zp_uint32_t minThreads, zp_uint32_t maxThreads);

        void FreeThreadPool(zp_handle_t threadPool);

        zp_handle_t CreateThread(ThreadFunc threadFunc, void* param, zp_size_t stackSize, zp_uint32_t* threadId);

        void CloseThread(zp_handle_t threadHandle);

        void JoinThreads(zp_handle_t* threadHandles, zp_size_t threadHandleCount);

    private:
        zp_handle_t m_windowHandle;
    };

    Platform* GetPlatform();
}

#endif //ZP_PLATFORM_H
