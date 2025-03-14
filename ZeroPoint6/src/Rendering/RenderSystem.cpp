//
// Created by phosg on 2/7/2022.
//

#include "Core/Defines.h"
#include "Core/Profiler.h"

#include "Platform/Platform.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/Shader.h"
#include "Rendering/GraphicsResource.h"

#include "Rendering/ImmediateModeRenderer.h"
#include "Rendering/BatchModeRenderer.h"

#include <cstddef>

namespace zp
{
    namespace
    {

        const zp_uint32_t vertexShaderCode[] = { 0x07230203, 0x00010000, 0x000d000a, 0x00000036,
                                                 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
                                                 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
                                                 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
                                                 0x0008000f, 0x00000000, 0x00000004, 0x6e69616d,
                                                 0x00000000, 0x00000022, 0x00000026, 0x00000031,
                                                 0x00030003, 0x00000002, 0x000001c2, 0x000a0004,
                                                 0x475f4c47, 0x4c474f4f, 0x70635f45, 0x74735f70,
                                                 0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365,
                                                 0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f,
                                                 0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572,
                                                 0x00657669, 0x00040005, 0x00000004, 0x6e69616d,
                                                 0x00000000, 0x00050005, 0x0000000c, 0x69736f70,
                                                 0x6e6f6974, 0x00000073, 0x00040005, 0x00000017,
                                                 0x6f6c6f63, 0x00007372, 0x00060005, 0x00000020,
                                                 0x505f6c67, 0x65567265, 0x78657472, 0x00000000,
                                                 0x00060006, 0x00000020, 0x00000000, 0x505f6c67,
                                                 0x7469736f, 0x006e6f69, 0x00070006, 0x00000020,
                                                 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953,
                                                 0x00000000, 0x00070006, 0x00000020, 0x00000002,
                                                 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e,
                                                 0x00070006, 0x00000020, 0x00000003, 0x435f6c67,
                                                 0x446c6c75, 0x61747369, 0x0065636e, 0x00030005,
                                                 0x00000022, 0x00000000, 0x00060005, 0x00000026,
                                                 0x565f6c67, 0x65747265, 0x646e4978, 0x00007865,
                                                 0x00050005, 0x00000031, 0x67617266, 0x6f6c6f43,
                                                 0x00000072, 0x00050048, 0x00000020, 0x00000000,
                                                 0x0000000b, 0x00000000, 0x00050048, 0x00000020,
                                                 0x00000001, 0x0000000b, 0x00000001, 0x00050048,
                                                 0x00000020, 0x00000002, 0x0000000b, 0x00000003,
                                                 0x00050048, 0x00000020, 0x00000003, 0x0000000b,
                                                 0x00000004, 0x00030047, 0x00000020, 0x00000002,
                                                 0x00040047, 0x00000026, 0x0000000b, 0x0000002a,
                                                 0x00040047, 0x00000031, 0x0000001e, 0x00000000,
                                                 0x00020013, 0x00000002, 0x00030021, 0x00000003,
                                                 0x00000002, 0x00030016, 0x00000006, 0x00000020,
                                                 0x00040017, 0x00000007, 0x00000006, 0x00000002,
                                                 0x00040015, 0x00000008, 0x00000020, 0x00000000,
                                                 0x0004002b, 0x00000008, 0x00000009, 0x00000003,
                                                 0x0004001c, 0x0000000a, 0x00000007, 0x00000009,
                                                 0x00040020, 0x0000000b, 0x00000006, 0x0000000a,
                                                 0x0004003b, 0x0000000b, 0x0000000c, 0x00000006,
                                                 0x0004002b, 0x00000006, 0x0000000d, 0x00000000,
                                                 0x0004002b, 0x00000006, 0x0000000e, 0xbf000000,
                                                 0x0005002c, 0x00000007, 0x0000000f, 0x0000000d,
                                                 0x0000000e, 0x0004002b, 0x00000006, 0x00000010,
                                                 0x3f000000, 0x0005002c, 0x00000007, 0x00000011,
                                                 0x00000010, 0x00000010, 0x0005002c, 0x00000007,
                                                 0x00000012, 0x0000000e, 0x00000010, 0x0006002c,
                                                 0x0000000a, 0x00000013, 0x0000000f, 0x00000011,
                                                 0x00000012, 0x00040017, 0x00000014, 0x00000006,
                                                 0x00000003, 0x0004001c, 0x00000015, 0x00000014,
                                                 0x00000009, 0x00040020, 0x00000016, 0x00000006,
                                                 0x00000015, 0x0004003b, 0x00000016, 0x00000017,
                                                 0x00000006, 0x0004002b, 0x00000006, 0x00000018,
                                                 0x3f800000, 0x0006002c, 0x00000014, 0x00000019,
                                                 0x00000018, 0x0000000d, 0x0000000d, 0x0006002c,
                                                 0x00000014, 0x0000001a, 0x0000000d, 0x00000018,
                                                 0x0000000d, 0x0006002c, 0x00000014, 0x0000001b,
                                                 0x0000000d, 0x0000000d, 0x00000018, 0x0006002c,
                                                 0x00000015, 0x0000001c, 0x00000019, 0x0000001a,
                                                 0x0000001b, 0x00040017, 0x0000001d, 0x00000006,
                                                 0x00000004, 0x0004002b, 0x00000008, 0x0000001e,
                                                 0x00000001, 0x0004001c, 0x0000001f, 0x00000006,
                                                 0x0000001e, 0x0006001e, 0x00000020, 0x0000001d,
                                                 0x00000006, 0x0000001f, 0x0000001f, 0x00040020,
                                                 0x00000021, 0x00000003, 0x00000020, 0x0004003b,
                                                 0x00000021, 0x00000022, 0x00000003, 0x00040015,
                                                 0x00000023, 0x00000020, 0x00000001, 0x0004002b,
                                                 0x00000023, 0x00000024, 0x00000000, 0x00040020,
                                                 0x00000025, 0x00000001, 0x00000023, 0x0004003b,
                                                 0x00000025, 0x00000026, 0x00000001, 0x00040020,
                                                 0x00000028, 0x00000006, 0x00000007, 0x00040020,
                                                 0x0000002e, 0x00000003, 0x0000001d, 0x00040020,
                                                 0x00000030, 0x00000003, 0x00000014, 0x0004003b,
                                                 0x00000030, 0x00000031, 0x00000003, 0x00040020,
                                                 0x00000033, 0x00000006, 0x00000014, 0x00050036,
                                                 0x00000002, 0x00000004, 0x00000000, 0x00000003,
                                                 0x000200f8, 0x00000005, 0x0003003e, 0x0000000c,
                                                 0x00000013, 0x0003003e, 0x00000017, 0x0000001c,
                                                 0x0004003d, 0x00000023, 0x00000027, 0x00000026,
                                                 0x00050041, 0x00000028, 0x00000029, 0x0000000c,
                                                 0x00000027, 0x0004003d, 0x00000007, 0x0000002a,
                                                 0x00000029, 0x00050051, 0x00000006, 0x0000002b,
                                                 0x0000002a, 0x00000000, 0x00050051, 0x00000006,
                                                 0x0000002c, 0x0000002a, 0x00000001, 0x00070050,
                                                 0x0000001d, 0x0000002d, 0x0000002b, 0x0000002c,
                                                 0x0000000d, 0x00000018, 0x00050041, 0x0000002e,
                                                 0x0000002f, 0x00000022, 0x00000024, 0x0003003e,
                                                 0x0000002f, 0x0000002d, 0x0004003d, 0x00000023,
                                                 0x00000032, 0x00000026, 0x00050041, 0x00000033,
                                                 0x00000034, 0x00000017, 0x00000032, 0x0004003d,
                                                 0x00000014, 0x00000035, 0x00000034, 0x0003003e,
                                                 0x00000031, 0x00000035, 0x000100fd, 0x00010038 };

        const zp_uint32_t fragmentShaderCode[] = { 0x07230203, 0x00010000, 0x000d000a, 0x00000013,
                                                   0x00000000, 0x00020011, 0x00000001, 0x0006000b,
                                                   0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
                                                   0x00000000, 0x0003000e, 0x00000000, 0x00000001,
                                                   0x0007000f, 0x00000004, 0x00000004, 0x6e69616d,
                                                   0x00000000, 0x00000009, 0x0000000c, 0x00030010,
                                                   0x00000004, 0x00000007, 0x00030003, 0x00000002,
                                                   0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f,
                                                   0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c,
                                                   0x7269645f, 0x69746365, 0x00006576, 0x00080004,
                                                   0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63,
                                                   0x69645f65, 0x74636572, 0x00657669, 0x00040005,
                                                   0x00000004, 0x6e69616d, 0x00000000, 0x00050005,
                                                   0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000,
                                                   0x00050005, 0x0000000c, 0x67617266, 0x6f6c6f43,
                                                   0x00000072, 0x00040047, 0x00000009, 0x0000001e,
                                                   0x00000000, 0x00040047, 0x0000000c, 0x0000001e,
                                                   0x00000000, 0x00020013, 0x00000002, 0x00030021,
                                                   0x00000003, 0x00000002, 0x00030016, 0x00000006,
                                                   0x00000020, 0x00040017, 0x00000007, 0x00000006,
                                                   0x00000004, 0x00040020, 0x00000008, 0x00000003,
                                                   0x00000007, 0x0004003b, 0x00000008, 0x00000009,
                                                   0x00000003, 0x00040017, 0x0000000a, 0x00000006,
                                                   0x00000003, 0x00040020, 0x0000000b, 0x00000001,
                                                   0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c,
                                                   0x00000001, 0x0004002b, 0x00000006, 0x0000000e,
                                                   0x3f800000, 0x00050036, 0x00000002, 0x00000004,
                                                   0x00000000, 0x00000003, 0x000200f8, 0x00000005,
                                                   0x0004003d, 0x0000000a, 0x0000000d, 0x0000000c,
                                                   0x00050051, 0x00000006, 0x0000000f, 0x0000000d,
                                                   0x00000000, 0x00050051, 0x00000006, 0x00000010,
                                                   0x0000000d, 0x00000001, 0x00050051, 0x00000006,
                                                   0x00000011, 0x0000000d, 0x00000002, 0x00070050,
                                                   0x00000007, 0x00000012, 0x0000000f, 0x00000010,
                                                   0x00000011, 0x0000000e, 0x0003003e, 0x00000009,
                                                   0x00000012, 0x000100fd, 0x00010038 };

        const zp_uint32_t colorVertexShader[] = {
//#include "Rendering/../../bin/Shaders/debug_color.vert.spv"
        };

        const zp_uint32_t colorFragmentShader[] = {
//#include "Rendering/../../bin/Shaders/debug_color.frag.spv"
        };

        struct WaitOnGPUJob
        {
            GraphicsDevice* graphicsDevice;

            static void Execute( const JobHandle& parentJobHandle, const WaitOnGPUJob* waitOnGpuJob )
            {
                ZP_PROFILE_CPU_BLOCK();

                //waitOnGpuJob->graphicsDevice->waitForGPU();
            }
        };

        struct BeginFrameGPUJob
        {
            zp_uint64_t frameIndex;
            GraphicsDevice* graphicsDevice;

            static void Execute( const JobHandle& parentJobHandle, const BeginFrameGPUJob* waitOnGpuJob )
            {
                ZP_PROFILE_CPU_BLOCK();

                //waitOnGpuJob->graphicsDevice->beginFrame();
            }
        };


        struct SubmitGPUJob
        {
            GraphicsDevice* m_graphicsDevice;

            static void Execute( const JobHandle& parentJobHandle, const SubmitGPUJob* waitOnGpuJob )
            {
                ZP_PROFILE_CPU_BLOCK();

                //waitOnGpuJob->m_graphicsDevice->submit();
            }
        };

        struct PresentGPUJob
        {
            GraphicsDevice* m_graphicsDevice;

            static void Execute( const JobHandle& parentJobHandle, const PresentGPUJob* presentGpuJob )
            {
                ZP_PROFILE_CPU_BLOCK();

                //presentGpuJob->m_graphicsDevice->present();
            }
        };
    }

    RenderSystem::RenderSystem( MemoryLabel memoryLabel )
        : m_graphicsDevice( nullptr )
        , m_currentRenderPipeline( nullptr )
        , m_nextRenderPipeline( nullptr )
        , memoryLabel( memoryLabel )
    {
    }

    RenderSystem::~RenderSystem()
    {
        destroy();
    }

    class TestRenderPipeline : public RenderPipeline
    {
    public:

        void onActivate( RenderSystem* renderSystem ) final
        {
            GraphicsDevice* graphicsDevice = renderSystem->getGraphicsDevice();

            // create render pass
            {
                RenderPassDesc renderPassDesc {};
                //graphicsDevice->createRenderPass( &renderPassDesc, m_renderPass.data() );
            }

            // create shaders
            {
                {
                    ShaderDesc shaderDesc {
                        .name = "vertex shaderHandle",
                        .entryPointName = "main",
                        .codeSizeInBytes = ZP_ARRAY_SIZE( vertexShaderCode ) * sizeof( zp_uint32_t ),
                        .codeData = vertexShaderCode,
                        .shaderStage = ZP_SHADER_STAGE_VERTEX,
                    };
                    //graphicsDevice->createShader( shaderDesc, m_vertexShader.data() );
                }

                {
                    ShaderDesc shaderDesc {
                        .name = "fragment shaderHandle",
                        .entryPointName = "main",
                        .codeSizeInBytes = ZP_ARRAY_SIZE( fragmentShaderCode ) * sizeof( zp_uint32_t ),
                        .codeData = fragmentShaderCode,
                        .shaderStage = ZP_SHADER_STAGE_FRAGMENT,
                    };
                    //graphicsDevice->createShader( shaderDesc, m_fragmentShader.data() );
                }

                {
                    char buff[256];
                    Platform::GetCurrentDir( buff );
                    zp_printfln( buff );

                    FileHandle fileHandle = Platform::OpenFileHandle( "../../ZeroPoint6/bin/Shaders/DebugColor.vert.spv", ZP_OPEN_FILE_MODE_READ );
                    zp_size_t fileSize = Platform::GetFileSize( fileHandle );

                    void* memPtr = GetAllocator( MemoryLabels::FileIO )->allocate( fileSize, kDefaultMemoryAlignment );
                    zp_size_t read = Platform::ReadFile( fileHandle, memPtr, fileSize );
                    ZP_ASSERT( read == fileSize );
                    Platform::CloseFileHandle( fileHandle );

                    ShaderDesc shaderDesc {
                        .name = "color vertex shaderHandle",
                        .entryPointName = "main",
                        .codeSizeInBytes = fileSize,
                        .codeData = memPtr,
                        .shaderStage = ZP_SHADER_STAGE_VERTEX,
                    };
                    //graphicsDevice->createShader( shaderDesc, m_colorVertexShader.data() );

                    GetAllocator( MemoryLabels::FileIO )->free( memPtr );
                }

                {
                    FileHandle fileHandle = Platform::OpenFileHandle( "../../ZeroPoint6/bin/Shaders/DebugColor.frag.spv", ZP_OPEN_FILE_MODE_READ );
                    zp_size_t fileSize = Platform::GetFileSize( fileHandle );

                    void* memPtr = GetAllocator( MemoryLabels::FileIO )->allocate( fileSize, kDefaultMemoryAlignment );
                    zp_size_t read = Platform::ReadFile( fileHandle, memPtr, fileSize );
                    ZP_ASSERT( read == fileSize );
                    Platform::CloseFileHandle( fileHandle );

                    ShaderDesc shaderDesc {
                        .name = "color fragment shaderHandle",
                        .entryPointName = "main",
                        .codeSizeInBytes = fileSize,
                        .codeData = memPtr,
                        .shaderStage = ZP_SHADER_STAGE_FRAGMENT,
                    };
                    //graphicsDevice->createShader( shaderDesc, m_colorFragmentShader.data() );

                    GetAllocator( MemoryLabels::FileIO )->free( memPtr );
                }
            }

            {
                PipelineLayoutDesc pipelineLayoutDesc {};
                //graphicsDevice->createPipelineLayout( &pipelineLayoutDesc, m_pipelineLayout.data() );
            }

            // create pipeline
            {
                GraphicsPipelineStateCreateDesc graphicsPipelineStateDesc {};
                graphicsPipelineStateDesc.name = "default pipeline";
                graphicsPipelineStateDesc.layout = PipelineLayoutResourceHandle( &m_pipelineLayout );
                graphicsPipelineStateDesc.renderPass = RenderPassResourceHandle( &m_renderPass );
                graphicsPipelineStateDesc.subPass = 0;

                graphicsPipelineStateDesc.shaderStageCount = 2;
                graphicsPipelineStateDesc.shaderStages[ 0 ] = ShaderResourceHandle( &m_vertexShader );
                graphicsPipelineStateDesc.shaderStages[ 1 ] = ShaderResourceHandle( &m_fragmentShader );

                graphicsPipelineStateDesc.primitiveTopology = ZP_TOPOLOGY_TRIANGLE_LIST;
                graphicsPipelineStateDesc.primitiveRestartEnable = false;

                graphicsPipelineStateDesc.patchControlPoints = 0;

                graphicsPipelineStateDesc.viewportCount = 1;
                graphicsPipelineStateDesc.viewports[ 0 ] = { 0, 0, 800, 600, 0, 1 };
                graphicsPipelineStateDesc.scissorRectCount = 1;
                graphicsPipelineStateDesc.scissorRects[ 0 ] = { 0, 0, 800, 600 };

                graphicsPipelineStateDesc.polygonFillMode = ZP_POLYGON_FILL_MODE_FILL;
                graphicsPipelineStateDesc.cullMode = ZP_CULL_MODE_NONE;
                graphicsPipelineStateDesc.frontFaceMode = ZP_FRONT_FACE_MODE_CCW;

                graphicsPipelineStateDesc.sampleCount = ZP_SAMPLE_COUNT_1;

                graphicsPipelineStateDesc.blendStateCount = 1;
                graphicsPipelineStateDesc.blendStates[ 0 ].blendEnable = false;
                graphicsPipelineStateDesc.blendStates[ 0 ].srcColorBlendFactor = ZP_BLEND_FACTOR_ONE;
                graphicsPipelineStateDesc.blendStates[ 0 ].dstColorBlendFactor = ZP_BLEND_FACTOR_ZERO;
                graphicsPipelineStateDesc.blendStates[ 0 ].colorBlendOp = ZP_BLEND_OP_ADD;
                graphicsPipelineStateDesc.blendStates[ 0 ].srcAlphaBlendFactor = ZP_BLEND_FACTOR_ONE;
                graphicsPipelineStateDesc.blendStates[ 0 ].dstAlphaBlendFactor = ZP_BLEND_FACTOR_ZERO;
                graphicsPipelineStateDesc.blendStates[ 0 ].alphaBlendOp = ZP_BLEND_OP_ADD;
                graphicsPipelineStateDesc.blendStates[ 0 ].writeMask = ZP_COLOR_COMPONENT_RGBA;

                //graphicsDevice->createGraphicsPipeline( &graphicsPipelineStateDesc, &m_graphicsPipelineState );

                // color
                m_colorMaterial = {};
                m_colorMaterial.get().vertShader = ShaderResourceHandle( &m_colorVertexShader );
                m_colorMaterial.get().fragShader = ShaderResourceHandle( &m_colorFragmentShader );

                graphicsPipelineStateDesc.name = "color pipeline";
                graphicsPipelineStateDesc.shaderStages[ 0 ] = m_colorMaterial.get().vertShader;
                graphicsPipelineStateDesc.shaderStages[ 1 ] = m_colorMaterial.get().fragShader;

                graphicsPipelineStateDesc.vertexBindingCount = 1;
                graphicsPipelineStateDesc.vertexAttributeCount = 3;
                graphicsPipelineStateDesc.vertexBindings[ 0 ] = { 0, sizeof( VertexVUC ), ZP_VERTEX_INPUT_RATE_VERTEX };
                graphicsPipelineStateDesc.vertexAttributes[ 0 ] = { 0, 0, offsetof( struct VertexVUC, vertexOS ), ZP_GRAPHICS_FORMAT_R32G32B32_SFLOAT };
                graphicsPipelineStateDesc.vertexAttributes[ 1 ] = { 1, 0, offsetof( struct VertexVUC, color ), ZP_GRAPHICS_FORMAT_R32G32B32A32_SFLOAT };
                graphicsPipelineStateDesc.vertexAttributes[ 2 ] = { 2, 0, offsetof( struct VertexVUC, uv0 ), ZP_GRAPHICS_FORMAT_R32G32_SFLOAT };

                //graphicsDevice->createGraphicsPipeline( &graphicsPipelineStateDesc, &m_colorMaterial.get().materialRenderPipeline );
            }

            {
                renderSystem->getImmediateModeRenderer()->registerMaterial( MaterialResourceHandle( &m_colorMaterial ) );
            }
        }

        void onDeactivate( RenderSystem* renderSystem ) final
        {
            GraphicsDevice* graphicsDevice = renderSystem->getGraphicsDevice();

            //graphicsDevice->destroyShader( m_fragmentShader.data() );
            //graphicsDevice->destroyShader( m_vertexShader.data() );
            //graphicsDevice->destroyShader( m_colorVertexShader.data() );
            //graphicsDevice->destroyShader( m_colorFragmentShader.data() );
            //graphicsDevice->destroyPipelineLayout( m_pipelineLayout.data() );
            //graphicsDevice->destroyGraphicsPipeline( &m_graphicsPipelineState );
            //graphicsDevice->destroyGraphicsPipeline( &m_colorMaterial.get().materialRenderPipeline );
            //graphicsDevice->destroyRenderPass( m_renderPass.data() );
        }

        struct PipelineJob
        {
            RenderPassResourceHandle renderPass;
            GraphicsPipelineState* graphicsPipelineState;
            RenderPipelineContext renderPipelineContext;

            static void Execute( const JobHandle& parentJobHandle, const PipelineJob* data )
            {
                RenderPipelineContext context = data->renderPipelineContext;
                GraphicsDevice* graphicsDevice = context.m_graphicsDevice;
#if 0
                context.cull();

                context.beginRenderPass();

                context.drawRenderers(); // opaque

                context.drawRenderers(); // transparent

                context.endRenderPass();
#endif
#if 0
                CommandQueue* cmd = graphicsDevice->requestCommandQueue( ZP_RENDER_QUEUE_GRAPHICS );

                graphicsDevice->beginRenderPass( data->renderPass.data(), cmd );

                graphicsDevice->bindPipeline( data->graphicsPipelineState, ZP_PIPELINE_BIND_POINT_GRAPHICS, cmd );

                graphicsDevice->draw( 3, 1, 0, 0, cmd );

                graphicsDevice->endRenderPass( cmd );

                graphicsDevice->releaseCommandQueue( cmd );
#endif
            }
        };

        JobHandle onProcessPipeline( RenderPipelineContext* renderPipelineContext, const JobHandle& inputHandle ) final
        {
            PipelineJob pipelineJob {
                .renderPass = RenderPassResourceHandle( &m_renderPass ),
                .graphicsPipelineState = &m_graphicsPipelineState,
                .renderPipelineContext = *renderPipelineContext,
            };
            //return renderPipelineContext->m_jobSystem->PrepareJobData( pipelineJob, inputHandle );
            return inputHandle;
        }

    private:
        RenderPassResource m_renderPass;
        PipelineLayoutResource m_pipelineLayout;
        GraphicsPipelineState m_graphicsPipelineState;
        ShaderResource m_vertexShader;
        ShaderResource m_fragmentShader;

        MaterialResource m_colorMaterial;
        ShaderResource m_colorVertexShader;
        ShaderResource m_colorFragmentShader;
    };

    namespace
    {
        TestRenderPipeline s_renderPipeline;
    }

    void RenderSystem::initialize( zp_handle_t windowHandle, const GraphicsDeviceDesc& graphicsDeviceDesc )
    {
        s_renderPipeline = {};

        //m_graphicsDevice = CreateGraphicsDevice( memoryLabel );

        //m_graphicsDevice->createSwapchain( windowHandle, 0, 0, 0, ZP_COLOR_SPACE_SRGB_NONLINEAR );

        BatchModeRendererConfig batchModeRendererConfig {};
        m_batchModeRenderer = ZP_NEW_ARGS( memoryLabel, BatchModeRenderer, &batchModeRendererConfig );

        ImmediateModeRendererConfig immediateModeRendererConfig {
            64,
            1024 * 16,
            1024 * 16,
            m_graphicsDevice,
            m_batchModeRenderer
        };
        m_immediateModeRenderer = ZP_NEW_ARGS( memoryLabel, ImmediateModeRenderer, &immediateModeRendererConfig );

        m_nextRenderPipeline = &s_renderPipeline;
    }

    void RenderSystem::destroy()
    {
        if( m_currentRenderPipeline )
        {
            m_currentRenderPipeline->onDeactivate( this );
            m_currentRenderPipeline = nullptr;
        }

        ZP_SAFE_DELETE( ImmediateModeRenderer, m_immediateModeRenderer );
        ZP_SAFE_DELETE( BatchModeRenderer, m_batchModeRenderer );

        DestroyGraphicsDevice( m_graphicsDevice );

        m_graphicsDevice = nullptr;
    }

    JobHandle RenderSystem::startSystem( zp_uint64_t frameIndex, void* jobSystem, const JobHandle& inputHandle )
    {
        m_batchModeRenderer->beginFrame( frameIndex );
        m_immediateModeRenderer->beginFrame( frameIndex );

        return inputHandle;
#if 0
        struct StartFrameJob
        {
            zp_uint64_t frameCount;
            RenderSystem* renderSystem;

            static void Execute( const JobHandle& jobHandle, const StartFrameJob* data )
            {
                RenderSystem* renderSystem = data->renderSystem;

                renderSystem->m_batchModeRenderer->beginFrame( data->frameCount );
                renderSystem->m_immediateModeRenderer->beginFrame( data->frameCount );
            }
        } startFrameJob { frameCount, this };
        return jobSystem->PrepareJobData( startFrameJob, inputHandle );
#endif
    }

    JobHandle RenderSystem::processSystem( zp_uint64_t frameIndex, void* jobSystem, EntityComponentManager* entityComponentManager, const JobHandle& inputHandle )
    {
        ZP_PROFILE_CPU_BLOCK();

        JobHandle gpuHandle = inputHandle;

        WaitOnGPUJob waitOnGpuJob {
            .graphicsDevice = m_graphicsDevice
        };
        //gpuHandle = jobSystem->PrepareJobData( waitOnGpuJob, gpuHandle );

        struct UpdateRenderPipelineJob
        {
            RenderSystem* renderSystem;

            static void Execute( const JobHandle& jobHandle, const UpdateRenderPipelineJob* data )
            {
                ZP_PROFILE_CPU_BLOCK();

                RenderSystem* renderSystem = data->renderSystem;
                if( renderSystem->m_nextRenderPipeline != renderSystem->m_currentRenderPipeline )
                {
                    if( renderSystem->m_currentRenderPipeline )
                    {
                        renderSystem->m_currentRenderPipeline->onDeactivate( renderSystem );
                    }

                    renderSystem->m_currentRenderPipeline = renderSystem->m_nextRenderPipeline;

                    if( renderSystem->m_currentRenderPipeline )
                    {
                        renderSystem->m_currentRenderPipeline->onActivate( renderSystem );
                    }
                }
            }
        } updateRenderPipelineJob {
            .renderSystem = this
        };
        //gpuHandle = jobSystem->PrepareJobData( updateRenderPipelineJob, gpuHandle );

        BeginFrameGPUJob beginFrameGpuJob {
            .frameIndex = frameIndex,
            .graphicsDevice = m_graphicsDevice
        };
        //gpuHandle = jobSystem->PrepareJobData( beginFrameGpuJob, gpuHandle );

        struct ProcessRenderPipeline
        {
            RenderPipelineContext renderPipelineContext;

            static void Execute( const JobHandle& parentJobHandle, const ProcessRenderPipeline* processRenderPipeline )
            {
                ZP_PROFILE_CPU_BLOCK();

                RenderPipelineContext ctx = processRenderPipeline->renderPipelineContext;
                RenderSystem* renderSystem = ctx.m_renderSystem;

                if( renderSystem->m_currentRenderPipeline )
                {
                    //JobHandle pipelineHandle = renderSystem->m_currentRenderPipeline->onProcessPipeline( &ctx );

                    //ctx.m_jobSystem->Schedule( pipelineHandle ).complete();
                }
                else
                {
                    GraphicsDevice* graphicsDevice = ctx.m_graphicsDevice;
                    //CommandQueue* cmd = graphicsDevice->requestCommandQueue( ZP_RENDER_QUEUE_GRAPHICS );

                    //graphicsDevice->beginRenderPass( nullptr, cmd );
                    //graphicsDevice->endRenderPass( cmd );

                    //graphicsDevice->releaseCommandQueue( cmd );
                }
            }
        } processRenderPipeline {
            .renderPipelineContext {
                .m_frameIndex = frameIndex,
                .m_graphicsDevice = m_graphicsDevice,
                .m_renderSystem = this,
            }
        };
        //gpuHandle = jobSystem->PrepareJobData( processRenderPipeline, gpuHandle );
#if 0
        struct RenderProfilerData
        {
            //ProfilerFrameRange range;
            ImmediateModeRenderer* immediateModeRenderer;

            static void Execute( const JobHandle& parentJobHandle, const RenderProfilerData* renderProfilerData )
            {
                ZP_PROFILE_CPU_BLOCK();

                int count = 0;

                //ProfilerFrameEnumerator enumerator = renderProfilerData->profiler->captureFrames( renderProfilerData->range );


                Rect2Df orthoRect { .offset { .x = 0, .y = 0 }, .size { .width = 800, .height = 600 } };
                zp_handle_t cmd = renderProfilerData->immediateModeRenderer->begin( 0, ZP_TOPOLOGY_TRIANGLE_LIST, 4 * ( count + 1 ), 6 * ( count + 1 ) );

                Matrix4x4f localToWorld = Math::OrthoLH( orthoRect, -10, 10, orthoRect.size.width / orthoRect.size.height );
                renderProfilerData->immediateModeRenderer->setLocalToWorld( cmd, Matrix4x4f::identity );

                Rect2Df rect { .offset { .x = 0, .y = 0 }, .size { .width = 800, .height = 480 } };
                Rect2Df r = rect;

                {
                    Color color = Color::gray25;
                    Vector4f v0 = Math::Mul( localToWorld, Math::Vec4f( r.bottomLeft().x, r.bottomLeft().y, 0.f, 1.f ) );
                    Vector4f v1 = Math::Mul( localToWorld, Math::Vec4f( r.topLeft().x, r.topLeft().y, 0.f, 1.f ) );
                    Vector4f v2 = Math::Mul( localToWorld, Math::Vec4f( r.topRight().x, r.topRight().y, 0.f, 1.f ) );
                    Vector4f v3 = Math::Mul( localToWorld, Math::Vec4f( r.bottomRight().x, r.bottomRight().y, 0.f, 1.f ) );

                    VertexVUC vertices[] = {
                        { .vertexOS = Math::Vec3f( v0 ), .color = Math::Mul( Color::red, color ) },
                        { .vertexOS = Math::Vec3f( v1 ), .color = Math::Mul( Color::green, color ) },
                        { .vertexOS = Math::Vec3f( v2 ), .color = Math::Mul( Color::blue, color ) },
                        { .vertexOS = Math::Vec3f( v3 ), .color = Math::Mul( Color::white, color ) },
                    };
                    renderProfilerData->immediateModeRenderer->addQuads( cmd, vertices );
                }

                renderProfilerData->immediateModeRenderer->end( cmd );

                const zp_float32_t frameWidth = rect.size.width / (float)enumerator.count();
                for( zp_size_t i = 0; i < enumerator.count(); ++i )
                {
                    zp_float32_t baseOffset = static_cast<zp_float32_t >(i) * frameWidth;
                    r = { .offset { .x = 0, .y = 0 }, .size { .width = 0, .height = 16 } };

                    auto frame = enumerator[ i ];
                    zp_uint64_t minTime = frame.cpuProfilerEventCount > 0 ? frame.cpuProfilerEvents[ 0 ].startCycle : 0;
                    zp_uint64_t maxTime = frame.cpuProfilerEventCount > 0 ? frame.cpuProfilerEvents[ frame.cpuProfilerEventCount - 1 ].endCycle : minTime;
                    zp_float32_t cpuDuration = static_cast<zp_float32_t >(maxTime - minTime);

                    cmd = renderProfilerData->immediateModeRenderer->begin( 0, ZP_TOPOLOGY_TRIANGLE_LIST, 4 * ( frame.cpuProfilerEventCount ), 6 * ( frame.cpuProfilerEventCount ) );

                    for( zp_size_t c = 0; c < zp_min(10ULL,frame.cpuProfilerEventCount); ++c )
                    {
                        auto cpu = frame.cpuProfilerEvents + c;

                        zp_float32_t duration = static_cast<zp_float32_t >(cpu->endCycle - cpu->startCycle) / cpuDuration;
                        zp_float32_t xOffset = static_cast<zp_float32_t >(cpu->startCycle - minTime) / cpuDuration;

                        r.offset.x = baseOffset + xOffset * frameWidth;
                        r.size.width = duration * frameWidth;

                        Color color = zp_debug_color( cpu->threadId % 255, 255 );
                        Vector4f v0 = Math::Mul( localToWorld, Math::Vec4f( r.bottomLeft().x, r.bottomLeft().y, 0.f, 1.f ) );
                        Vector4f v1 = Math::Mul( localToWorld, Math::Vec4f( r.topLeft().x, r.topLeft().y, 0.f, 1.f ) );
                        Vector4f v2 = Math::Mul( localToWorld, Math::Vec4f( r.topRight().x, r.topRight().y, 0.f, 1.f ) );
                        Vector4f v3 = Math::Mul( localToWorld, Math::Vec4f( r.bottomRight().x, r.bottomRight().y, 0.f, 1.f ) );

                        VertexVUC vertices[] = {
                            { .vertexOS = Math::Vec3f( v0 ), .color = color },
                            { .vertexOS = Math::Vec3f( v1 ), .color = color },
                            { .vertexOS = Math::Vec3f( v2 ), .color = color },
                            { .vertexOS = Math::Vec3f( v3 ), .color = color },
                        };
                        renderProfilerData->immediateModeRenderer->addQuads( cmd, vertices );

                        r.offset.y += r.size.height;
                    }

                    renderProfilerData->immediateModeRenderer->end( cmd );
                }
#if 0
                r = rect;
                r.size.height = 12;
                r.size.width = 64;

                for( int i = 0; i < count; ++i )
                {
                    Color color = zp_debug_color( i, count );

                    Vector4f v0 = Math::Mul( localToWorld, Math::Vec4f( r.bottomLeft().x, r.bottomLeft().y, 0.f, 1.f ) );
                    Vector4f v1 = Math::Mul( localToWorld, Math::Vec4f( r.topLeft().x, r.topLeft().y, 0.f, 1.f ) );
                    Vector4f v2 = Math::Mul( localToWorld, Math::Vec4f( r.topRight().x, r.topRight().y, 0.f, 1.f ) );
                    Vector4f v3 = Math::Mul( localToWorld, Math::Vec4f( r.bottomRight().x, r.bottomRight().y, 0.f, 1.f ) );

                    VertexVUC vertices[] = {
                        { .vertexOS = Math::Vec3f( v0 ), .uv0 = Vector2f::zero, .color = color },
                        { .vertexOS = Math::Vec3f( v1 ), .uv0 = Vector2f::zero, .color = color },
                        { .vertexOS = Math::Vec3f( v2 ), .uv0 = Vector2f::zero, .color = color },
                        { .vertexOS = Math::Vec3f( v3 ), .uv0 = Vector2f::zero, .color = color },
                    };
                    renderProfilerData->immediateModeRenderer->addQuads( cmd, vertices );
                    r.offset.y += r.size.height + 2;
                    r.size.width += 16;
                }
#endif
            }
        } renderProfilerDataJob {
            .profiler = GetProfiler(),
            .range = ProfilerFrameRange::Last( frameCount, 1 ),
            .immediateModeRenderer = m_immediateModeRenderer,
        };
        //gpuHandle = jobSystem->PrepareJobData( renderProfilerDataJob, gpuHandle );
#endif
        struct FinalizeBatchRenderingJob
        {
            RenderSystem* renderSystem;

            static void Execute( const JobHandle& parentJobHandle, const FinalizeBatchRenderingJob* data )
            {
                ZP_PROFILE_CPU_BLOCK();

                data->renderSystem->m_immediateModeRenderer->process( data->renderSystem->m_batchModeRenderer );
                data->renderSystem->m_batchModeRenderer->process( data->renderSystem->m_graphicsDevice );
            }
        } finalizeBatchRenderingJob {
            .renderSystem = this,
        };
        //gpuHandle = jobSystem->PrepareJobData( finalizeBatchRenderingJob, gpuHandle );

        SubmitGPUJob submitGpuJob {
            .m_graphicsDevice = m_graphicsDevice
        };
        //gpuHandle = jobSystem->PrepareJobData( submitGpuJob, gpuHandle );

        PresentGPUJob presentGpuJob {
            .m_graphicsDevice = m_graphicsDevice
        };
        //gpuHandle = jobSystem->PrepareJobData( presentGpuJob, gpuHandle );

        return gpuHandle;
    }
}
