//
// Created by phosg on 7/27/2024.
//

#ifndef ZP_EXECUTIONGRAPH_H
#define ZP_EXECUTIONGRAPH_H

#include "Core/Macros.h"
#include "Core/Job.h"
#include "Core/Vector.h"
#include "Core/Function.h"

namespace zp
{
    struct ExecutionGraphContext
    {
    };

    struct ExecutionGraphCompilerContext
    {
    };

    //typedef JobHandle (* ExecutionGraphProcess)( const JobHandle& parentHandle, const JobHandle& inputHandle );
    typedef Function<JobHandle, const JobHandle&, const JobHandle&> ExecutionGraphProcess;

    typedef zp_bool_t (* ExecutionGraphCompileTimeCondition)( const ExecutionGraphCompilerContext& context );

    typedef zp_bool_t (* ExecutionGraphRuntimeCondition)( const ExecutionGraphContext& context );

    class ExecutionGraph;

    class ExecutionGraphBuilder
    {
    ZP_NONCOPYABLE( ExecutionGraphBuilder );

    public:
        ~ExecutionGraphBuilder();

        ExecutionGraphBuilder( ExecutionGraphBuilder&& builder ) noexcept;

        ExecutionGraphBuilder& operator=( ExecutionGraphBuilder&& builder ) noexcept;

        void AddPass( ExecutionGraphProcess pass );

        void AddCompileTimeCondition( ExecutionGraphCompileTimeCondition callback );

        void AddRuntimeCondition( ExecutionGraphRuntimeCondition callback );

    private:
        explicit ExecutionGraphBuilder( ExecutionGraph* executionGraph );

        ExecutionGraph* m_executionGraph;
        zp_size_t m_executionGraphNodeIndex;

        friend class ExecutionGraph;
    };

    class CompiledExecutionGraph
    {
    public:
        explicit CompiledExecutionGraph( const MemoryLabel memoryLabel );

        JobHandle Execute( const JobHandle& parentHandle, const JobHandle& inputHandle );

    private:
        struct ExecutionNode
        {
            ExecutionGraphProcess process;
            ExecutionGraphRuntimeCondition runtimeCondition;
        };

        Vector<ExecutionNode> m_executionNodes;

        friend class ExecutionGraph;
    };

    class ExecutionGraph
    {
    public:
        explicit ExecutionGraph( const MemoryLabel memoryLabel );

        void BeginRecording();

        ExecutionGraphBuilder AddExecutionPass();

        void EndRecording();

        void Compile( CompiledExecutionGraph& compiledExecutionGraph );

    private:
        zp_size_t BeginPass();

        void AddPass( zp_size_t index, ExecutionGraphProcess pass );

        void AddCompileTimeCondition( zp_size_t index, ExecutionGraphCompileTimeCondition callback );

        void AddRuntimeCondition( zp_size_t index, ExecutionGraphRuntimeCondition callback );

        void FinishPass( zp_size_t index );

        struct ExecutionNode
        {
            ExecutionGraphProcess process;
            ExecutionGraphCompileTimeCondition compileTimeCondition;
            ExecutionGraphRuntimeCondition runtimeCondition;
            zp_size_t index;
        };

    private:
        Vector<ExecutionNode> m_executionNodes;

        friend class ExecutionGraphBuilder;
    };
}

#endif //ZP_EXECUTIONGRAPH_H
