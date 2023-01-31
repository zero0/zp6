//
// Created by phosg on 2/17/2022.
//

#ifndef ZP_MATERIAL_H
#define ZP_MATERIAL_H

#include "Rendering/Shader.h"
#include "Rendering/GraphicsResource.h"

namespace zp
{
    struct MaterialCreateDesc
    {

    };

    struct MaterialData
    {
        ShaderResourceHandle vertShader;
        ShaderResourceHandle tessControlShader;
        ShaderResourceHandle tessEvalShader;
        ShaderResourceHandle geomShader;
        ShaderResourceHandle fragShader;
        ShaderResourceHandle taskShader;
        ShaderResourceHandle meshShader;

        GraphicsPipelineState materialRenderPipeline;
    };

    typedef GraphicsResource<MaterialData> MaterialResource;
    typedef GraphicsResourceHandle<MaterialData> MaterialResourceHandle;
}

#endif //ZP_MATERIAL_H
