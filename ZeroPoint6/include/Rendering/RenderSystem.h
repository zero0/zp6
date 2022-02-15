//
// Created by phosg on 11/6/2021.
//

#ifndef ZP_RENDERSYSTEM_H
#define ZP_RENDERSYSTEM_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Vector.h"
#include "Core/Allocator.h"
#include "Core/Math.h"

#include "Engine/JobSystem.h"

#include "Rendering/GraphicsDevice.h"
#include "Rendering/RenderPipeline.h"

namespace zp
{
    enum CameraProjectionType
    {
        ZP_CAMERA_PROJECTION_ORTHOGRAPHIC,
        ZP_CAMERA_PROJECTION_ORTHOGRAPHIC_CENTERED,
        ZP_CAMERA_PROJECTION_ORTHOGRAPHIC_OFFSET,

        ZP_CAMERA_PROJECTION_PERSPECTIVE,
        ZP_CAMERA_PROJECTION_PERSPECTIVE_OFFSET,
    };

    enum CameraClearMode
    {
        ZP_CAMERA_CLEAR_MODE_NONE,
        ZP_CAMERA_CLEAR_MODE_COLOR,
        ZP_CAMERA_CLEAR_MODE_DEPTH_STENCIL,
        ZP_CAMERA_CLEAR_MODE_COLOR_AND_DEPTH_STENCIL,
        ZP_CAMERA_CLEAR_MODE_SKY_BOX,
    };

    struct CameraComponentData
    {
        Matrix4x4f view;
        Matrix4x4f projection;
        Matrix4x4f viewProjection;
        Matrix4x4f invViewProjection;

        Viewport viewport;
        ScissorRect scissorRect;
        zp_float32_t fov;
        CameraProjectionType projectionType;
        CameraClearMode clearMode;
    };

    class RenderSystem
    {
    ZP_NONCOPYABLE( RenderSystem );

    public:
        explicit RenderSystem( MemoryLabel memoryLabel );

        ~RenderSystem();

        void initialize( zp_handle_t windowHandle, GraphicsDeviceFeatures graphicsDeviceFeatures );

        void destroy();

        PreparedJobHandle processSystem( zp_uint64_t frameIndex, JobSystem* jobSystem, const PreparedJobHandle& parentJobHandle, const PreparedJobHandle& inputHandle );

    private:
        GraphicsDevice* m_graphicsDevice;
        RenderPipeline* m_currentRenderPipeline;
        RenderPipeline* m_nextRenderPipeline;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_RENDERSYSTEM_H
