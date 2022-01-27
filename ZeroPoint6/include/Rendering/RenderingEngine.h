//
// Created by phosg on 11/6/2021.
//

#ifndef ZP_RENDERINGENGINE_H
#define ZP_RENDERINGENGINE_H

#include <cstdint>

namespace zp
{
    void InitializeRenderingEngine(void* hinst, void* hwnd, int width, int height);

    void DestroyRenderingEngine();

    void RenderFrame();

    void ResizeRenderingEngine(uint32_t width, uint32_t height);
}

#endif //ZP_RENDERINGENGINE_H
