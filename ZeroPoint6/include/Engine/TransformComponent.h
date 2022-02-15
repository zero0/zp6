//
// Created by phosg on 2/11/2022.
//

#ifndef ZP_TRANSFORMCOMPONENT_H
#define ZP_TRANSFORMCOMPONENT_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Math.h"

#include "Engine/Entity.h"

namespace zp
{
    struct DestroyedTag
    {
    };

    struct DisabledTag
    {
    };

    struct StaticTag
    {
    };

    struct TransformComponentData
    {
        Quaternion localRotation;
        Vector3f localPosition;
        Vector3f localScale;
    };

    struct RigidTransformComponentData
    {
        Quaternion localRotation;
        Vector3f localPosition;
    };

    struct UniformTransformComponentData
    {
        Quaternion localRotation;
        Vector3f localPosition;
        zp_float32_t uniformScale;
    };

    struct PositionComponentData
    {
        Vector3f localPosition;
    };

    struct RotationComponentData
    {
        Quaternion localRotation;
    };

    struct UniformScaleComponentData
    {
        zp_float32_t uniformScale;
    };

    struct ScaleComponentData
    {
        Vector3f localScale;
    };

    struct ChildComponentData
    {
        Entity parentEntity;
    };
}

#endif //ZP_TRANSFORMCOMPONENT_H
