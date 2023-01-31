#ifndef ZP_CAMERA_H
#define ZP_CAMERA_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Vector.h"
#include "Core/Math.h"

#include "Rendering/GraphicsDevice.h"

namespace zp
{
    enum CameraType
    {
        ZP_CAMERA_TYPE_GAME,
        ZP_CAMERA_TYPE_DEBUG,
    };

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
        CameraType type;
        CameraProjectionType projectionType;
        CameraClearMode clearMode;
        Color clearColor;
        zp_float32_t clearDepth;
        zp_uint32_t clearStencil;
    };
}

#endif //ZP_CAMERA_H
