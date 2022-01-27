module;

export module Platform;

import Core;

export namespace zp::Platform
{
    void Printf(const char* text, ...);

    zp_int32_t ShowPopupModal(const char* title);

    zp_handle_t CreatePlatformWindow();
}