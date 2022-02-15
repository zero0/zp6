//
// Created by phosg on 2/7/2022.
//

#include "Core/Defines.h"
#include "Core/Profiler.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/Shader.h"
#include "Rendering/GraphicsResource.h"

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

        struct WaitOnGPUJob
        {
            zp_uint64_t frameIndex;
            GraphicsDevice* m_graphicsDevice;

            static void Execute( const JobHandle& parentJobHandle, const WaitOnGPUJob* waitOnGpuJob )
            {
                waitOnGpuJob->m_graphicsDevice->waitForGPU();
            }
        };

        struct BeginFrameGPUJob
        {
            zp_uint64_t frameIndex;
            GraphicsDevice* m_graphicsDevice;

            static void Execute( const JobHandle& parentJobHandle, const BeginFrameGPUJob* waitOnGpuJob )
            {
                waitOnGpuJob->m_graphicsDevice->beginFrame( waitOnGpuJob->frameIndex );
            }
        };


        struct SubmitGPUJob
        {
            zp_uint64_t frameIndex;
            GraphicsDevice* m_graphicsDevice;

            static void Execute( const JobHandle& parentJobHandle, const SubmitGPUJob* waitOnGpuJob )
            {
                waitOnGpuJob->m_graphicsDevice->submit( waitOnGpuJob->frameIndex );
            }
        };

        struct PresentGPUJob
        {
            zp_uint64_t frameIndex;
            GraphicsDevice* m_graphicsDevice;

            static void Execute( const JobHandle& parentJobHandle, const PresentGPUJob* presentGpuJob )
            {
                presentGpuJob->m_graphicsDevice->present( presentGpuJob->frameIndex );
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

        void onActivate( GraphicsDevice* graphicsDevice ) final
        {
            // create render pass
            {
                RenderPassDesc renderPassDesc {};
                graphicsDevice->createRenderPass( &renderPassDesc, m_renderPass.data());
            }

            // create shaders
            {
                ShaderDesc shaderDesc {};
                shaderDesc.entryPointName = "main";

                shaderDesc.shaderStage = ZP_SHADER_STAGE_VERTEX;
                shaderDesc.codeData = vertexShaderCode;
                shaderDesc.codeSize = ZP_ARRAY_SIZE( vertexShaderCode );
                shaderDesc.name = "vertex shader";
                graphicsDevice->createShader( &shaderDesc, m_vertexShader.data());

                shaderDesc.shaderStage = ZP_SHADER_STAGE_FRAGMENT;
                shaderDesc.codeData = fragmentShaderCode;
                shaderDesc.codeSize = ZP_ARRAY_SIZE( fragmentShaderCode );
                shaderDesc.name = "fragment shader";
                graphicsDevice->createShader( &shaderDesc, m_fragmentShader.data());
            }

            {
                PipelineLayoutDesc pipelineLayoutDesc {};
                graphicsDevice->createPipelineLayout( &pipelineLayoutDesc, m_pipelineLayout.data());
            }

            // create pipeline
            {
                GraphicsPipelineStateDesc graphicsPipelineStateDesc {};
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

                graphicsDevice->createGraphicsPipeline( &graphicsPipelineStateDesc, &m_graphicsPipelineState );
            }
        }

        void onDeactivate( GraphicsDevice* graphicsDevice ) final
        {
            graphicsDevice->destroyShader( m_fragmentShader.data());
            graphicsDevice->destroyShader( m_vertexShader.data());
            graphicsDevice->destroyPipelineLayout( m_pipelineLayout.data());
            graphicsDevice->destroyGraphicsPipeline( &m_graphicsPipelineState );
            graphicsDevice->destroyRenderPass( m_renderPass.data());
        }

        struct PipelineJob
        {
            RenderPassResourceHandle renderPass;
            GraphicsPipelineState* graphicsPipelineState;
            RenderPipelineContext renderPipelineContext;
            zp_uint64_t frameIndex;

            static void Execute( const JobHandle& parentJobHandle, const PipelineJob* data )
            {
                CommandQueue* cmd = data->renderPipelineContext.m_graphicsDevice->requestCommandQueue( ZP_RENDER_QUEUE_GRAPHICS, data->frameIndex );

                data->renderPipelineContext.m_graphicsDevice->beginRenderPass( data->renderPass.data(), cmd );

                data->renderPipelineContext.m_graphicsDevice->bindPipeline( data->graphicsPipelineState, ZP_PIPELINE_BIND_POINT_GRAPHICS, cmd );

                data->renderPipelineContext.m_graphicsDevice->draw( 3, 1, 0, 0, cmd );

                data->renderPipelineContext.m_graphicsDevice->endRenderPass( cmd );
            }
        };

        PreparedJobHandle onProcessPipeline( RenderPipelineContext* renderPipelineContext, const PreparedJobHandle& inputHandle ) final
        {
            PipelineJob pipelineJob {
                .renderPass = RenderPassResourceHandle( &m_renderPass ),
                .graphicsPipelineState = &m_graphicsPipelineState,
                .renderPipelineContext = *renderPipelineContext,
            };
            return renderPipelineContext->m_jobSystem->PrepareJobData( pipelineJob, inputHandle );
        }

    private:
        RenderPassResource m_renderPass;
        PipelineLayoutResource m_pipelineLayout;
        GraphicsPipelineState m_graphicsPipelineState;
        ShaderResource m_vertexShader;
        ShaderResource m_fragmentShader;
    };

    namespace
    {
        TestRenderPipeline s_renderPipeline;
    }

    void RenderSystem::initialize( zp_handle_t windowHandle, GraphicsDeviceFeatures graphicsDeviceFeatures )
    {
        s_renderPipeline = {};

        m_graphicsDevice = CreateGraphicsDevice( memoryLabel, graphicsDeviceFeatures );

        m_graphicsDevice->createSwapChain( windowHandle, 0, 0, 0, 0 );

        //m_nextRenderPipeline = &s_renderPipeline;
    }

    void RenderSystem::destroy()
    {
        if( m_currentRenderPipeline )
        {
            m_currentRenderPipeline->onDeactivate( m_graphicsDevice );
            m_currentRenderPipeline = nullptr;
        }

        DestroyGraphicsDevice( m_graphicsDevice );

        m_graphicsDevice = nullptr;
    }

    PreparedJobHandle RenderSystem::processSystem( zp_uint64_t frameIndex, JobSystem* jobSystem, const PreparedJobHandle& parentJobHandle, const PreparedJobHandle& inputHandle )
    {
        struct UpdateRenderPipelineJob
        {
            RenderSystem* renderSystem;

            static void Execute( const JobHandle& jobHandle, const UpdateRenderPipelineJob* data )
            {
                RenderSystem* renderSystem = data->renderSystem;
                if( renderSystem->m_nextRenderPipeline != renderSystem->m_currentRenderPipeline )
                {
                    renderSystem->m_graphicsDevice->waitForGPU();

                    if( renderSystem->m_currentRenderPipeline ) renderSystem->m_currentRenderPipeline->onDeactivate( renderSystem->m_graphicsDevice );

                    renderSystem->m_currentRenderPipeline = renderSystem->m_nextRenderPipeline;

                    if( renderSystem->m_currentRenderPipeline ) renderSystem->m_currentRenderPipeline->onActivate( renderSystem->m_graphicsDevice );
                }
            }
        } updateRenderPipelineJob { this };

        PreparedJobHandle gpuHandle = jobSystem->PrepareChildJobData( parentJobHandle, updateRenderPipelineJob, inputHandle );
        PreparedJobHandle renderHandle = gpuHandle;

        WaitOnGPUJob waitOnGpuJob { frameIndex, m_graphicsDevice };
        gpuHandle = jobSystem->PrepareChildJobData( parentJobHandle, waitOnGpuJob, gpuHandle );

        BeginFrameGPUJob beginFrameGpuJob { frameIndex, m_graphicsDevice };
        gpuHandle = jobSystem->PrepareChildJobData( parentJobHandle, beginFrameGpuJob, gpuHandle );

        struct ProcessRenderPipeline
        {
            zp_uint64_t frameIndex;
            RenderSystem* renderSystem;

            static void Execute( const JobHandle& parentJobHandle, const ProcessRenderPipeline* processRenderPipeline )
            {
                if( processRenderPipeline->renderSystem->m_currentRenderPipeline )
                {

                }
                else
                {
                    CommandQueue* cmd = processRenderPipeline->renderSystem->m_graphicsDevice->requestCommandQueue( ZP_RENDER_QUEUE_GRAPHICS, processRenderPipeline->frameIndex );

                    processRenderPipeline->renderSystem->m_graphicsDevice->beginRenderPass( nullptr, cmd );
                    processRenderPipeline->renderSystem->m_graphicsDevice->endRenderPass( cmd );
                }
            }
        } processRenderPipeline { frameIndex, this };
        gpuHandle = jobSystem->PrepareChildJobData( parentJobHandle, processRenderPipeline, gpuHandle );

        //if( m_currentRenderPipeline )
        //{
        //    //ZP_PROFILE_CPU_BLOCK_E( "RenderSystem - Process Render Pipeline" );
        //
        //    RenderPipelineContext renderPipelineContext { .m_graphicsDevice = m_graphicsDevice, .m_jobSystem = jobSystem };
        //    gpuHandle = m_currentRenderPipeline->onProcessPipeline( &renderPipelineContext, gpuHandle );
        //}

        SubmitGPUJob submitGpuJob { frameIndex, m_graphicsDevice };
        gpuHandle = jobSystem->PrepareChildJobData( parentJobHandle, submitGpuJob, gpuHandle );

        PresentGPUJob presentGpuJob { frameIndex, m_graphicsDevice };
        gpuHandle = jobSystem->PrepareChildJobData( parentJobHandle, presentGpuJob, gpuHandle );

        return gpuHandle;
#if 0
        if( m_nextRenderPipeline != m_currentRenderPipeline )
        {
            m_graphicsDevice->waitForGPU();

            if( m_currentRenderPipeline ) m_currentRenderPipeline->onDeactivate( m_graphicsDevice );

            m_currentRenderPipeline = m_nextRenderPipeline;

            if( m_currentRenderPipeline ) m_currentRenderPipeline->onActivate( m_graphicsDevice );
        }

        //m_graphicsDevice->waitForGPU();

        PreparedJobHandle gpuHandle = inputHandle;

        WaitOnGPUJob waitOnGpuJob { s_frameIndex, m_graphicsDevice };
        //gpuHandle = jobSystem->PrepareJobData( waitOnGpuJob, gpuHandle );

        BeginFrameGPUJob beginFrameGpuJob { s_frameIndex, m_graphicsDevice };
        gpuHandle = jobSystem->PrepareJobData( beginFrameGpuJob, gpuHandle );

        if( m_currentRenderPipeline )
        {
            //ZP_PROFILE_CPU_BLOCK_E( "RenderSystem - Process Render Pipeline" );

            RenderPipelineContext renderPipelineContext { .m_graphicsDevice = m_graphicsDevice, .m_jobSystem = jobSystem };
            gpuHandle = m_currentRenderPipeline->onProcessPipeline( &renderPipelineContext, gpuHandle );
        }

        SubmitGPUJob submitGpuJob { s_frameIndex, m_graphicsDevice };
        gpuHandle = jobSystem->PrepareJobData( submitGpuJob, gpuHandle );

        PresentGPUJob presentGpuJob { s_frameIndex, m_graphicsDevice };
        gpuHandle = jobSystem->PrepareJobData( presentGpuJob, gpuHandle );
        s_frameIndex++;
        return gpuHandle;
#endif
    }
}