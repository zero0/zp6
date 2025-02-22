//
// Created by phosg on 7/27/2024.
//

#include "Core/Profiler.h"
#include "Engine/ExecutionGraph.h"

namespace zp
{
    ExecutionGraphBuilder::ExecutionGraphBuilder( ExecutionGraph* executionGraph )
        : m_executionGraph( executionGraph )
        , m_executionGraphNodeIndex( m_executionGraph->BeginPass() )
    {
    }

    ExecutionGraphBuilder::ExecutionGraphBuilder( zp::ExecutionGraphBuilder&& builder ) noexcept
        : m_executionGraph( builder.m_executionGraph )
        , m_executionGraphNodeIndex( builder.m_executionGraphNodeIndex )
    {
        builder.m_executionGraph = nullptr;
        builder.m_executionGraphNodeIndex = -1;
    }

    ExecutionGraphBuilder::~ExecutionGraphBuilder()
    {
        if( m_executionGraph )
        {
            m_executionGraph->FinishPass( m_executionGraphNodeIndex );
        }

        m_executionGraph = nullptr;
        m_executionGraphNodeIndex = -1;
    }

    ExecutionGraphBuilder& ExecutionGraphBuilder::operator=( ExecutionGraphBuilder&& builder ) noexcept
    {
        m_executionGraph = builder.m_executionGraph;
        m_executionGraphNodeIndex = builder.m_executionGraphNodeIndex;

        builder.m_executionGraph = nullptr;
        builder.m_executionGraphNodeIndex = -1;

        return *this;
    }

    void ExecutionGraphBuilder::AddPass( zp::ExecutionGraphProcess pass )
    {
        m_executionGraph->AddPass( m_executionGraphNodeIndex, pass );
    }

    void ExecutionGraphBuilder::AddCompileTimeCondition( zp::ExecutionGraphCompileTimeCondition callback )
    {
        m_executionGraph->AddCompileTimeCondition( m_executionGraphNodeIndex, callback );
    }

    void ExecutionGraphBuilder::AddRuntimeCondition( zp::ExecutionGraphRuntimeCondition callback )
    {
        m_executionGraph->AddRuntimeCondition( m_executionGraphNodeIndex, callback );
    }
}

namespace zp
{
    CompiledExecutionGraph::CompiledExecutionGraph( const zp::MemoryLabel memoryLabel )
        : m_executionNodes( 16, memoryLabel )
    {
    }

    JobHandle CompiledExecutionGraph::Execute( const zp::JobHandle& parentHandle, const zp::JobHandle& inputHandle )
    {
        ZP_PROFILE_CPU_BLOCK();

        ExecutionGraphContext context {};

        JobHandle handle = inputHandle;

        for( const ExecutionNode& x : m_executionNodes )
        {
            if( x.runtimeCondition == nullptr || x.runtimeCondition( context ) )
            {
                handle = x.process( parentHandle, handle );
            }
        }

        JobSystem::ScheduleBatchJobs();

        return handle;
    }
}

namespace zp
{
    ZP_PROFILE_CPU_DEF( Recording );

    ExecutionGraph::ExecutionGraph( const zp::MemoryLabel memoryLabel )
        : m_executionNodes( 16, memoryLabel )
    {
    }

    void ExecutionGraph::BeginRecording()
    {
        ZP_PROFILE_CPU_START( Recording );
        m_executionNodes.clear();
    }

    ExecutionGraphBuilder ExecutionGraph::AddExecutionPass()
    {
        return ExecutionGraphBuilder( this );
    }

    void ExecutionGraph::EndRecording()
    {
        ZP_PROFILE_CPU_END( Recording );
    }

    void ExecutionGraph::Compile( zp::CompiledExecutionGraph& compiledExecutionGraph )
    {
        ZP_PROFILE_CPU_BLOCK();

        ExecutionGraphCompilerContext context {};

        compiledExecutionGraph.m_executionNodes.clear();

        for( const ExecutionNode& node : m_executionNodes )
        {
            if( node.process && ( node.compileTimeCondition == nullptr || node.compileTimeCondition( context ) ) )
            {
                compiledExecutionGraph.m_executionNodes.pushBack( {
                    .process = node.process,
                    .runtimeCondition = node.runtimeCondition
                } );
            }
        }

        m_executionNodes.clear();
    }

    zp_size_t ExecutionGraph::BeginPass()
    {
        const zp_size_t index = m_executionNodes.length();

        m_executionNodes.pushBackEmpty();

        return index;
    }

    void ExecutionGraph::AddPass( zp_size_t index, zp::ExecutionGraphProcess pass )
    {
        m_executionNodes[ index ].process = pass;
    }

    void ExecutionGraph::AddCompileTimeCondition( zp_size_t index, zp::ExecutionGraphCompileTimeCondition callback )
    {
        m_executionNodes[ index ].compileTimeCondition = callback;
    }

    void ExecutionGraph::AddRuntimeCondition( zp_size_t index, ExecutionGraphRuntimeCondition callback )
    {
        m_executionNodes[ index ].runtimeCondition = callback;
    }

    void ExecutionGraph::FinishPass( zp_size_t index )
    {
        m_executionNodes[ index ].index = index;
    }
}
