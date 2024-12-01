//
// Created by phosg on 3/25/2023.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Allocator.h"
#include "Core/Function.h"

#include "Engine/Subsystem.h"

namespace zp
{
    SubsystemManager::SubsystemManager( MemoryLabel memoryLabel )
        : m_subsystems( 8, memoryLabel )
        , memoryLabel( memoryLabel )
    {
    }

    SubsystemManager::~SubsystemManager()
    {
        for( auto s : m_subsystems )
        {
            ZP_ASSERT( s->refCount == 0 );
            ZP_DELETE_LABEL( s->memoryLabel, ISubsystem, s->subsystem );

            ZP_FREE( memoryLabel, s );
        }

        m_subsystems.clear();
    }

    void SubsystemManager::BuildExecutionGraph( zp::ExecutionGraph& executionGraph )
    {
        m_subsystems.sort(
            []( auto lh, auto rh ) -> zp_int32_t
            {
                return zp_cmp_asc( lh->order, rh->order );
            } );

        for( auto s : m_subsystems )
        {
            if( s->refCount > 0 )
            {
                auto builder = executionGraph.AddExecutionPass();
                builder.AddPass( ExecutionGraphProcess::from_method<ISubsystem, &ISubsystem::Process>( s->subsystem ) );
            }
        }
    }

    void SubsystemManager::RegisterSubsystem( const SubsystemManager::RegisterSubsystemDesc& desc )
    {
        const zp_int32_t order = zp_int32_t( m_subsystems.size() ) * 100;

        auto subsystem = ZP_MALLOC_T( memoryLabel, SubsystemInstance );
        *subsystem = {
            .subsystem = desc.subsystem,
            .id = desc.id,
            .refCount = 0,
            .order = order,
            .memoryLabel = desc.memoryLabel,
        };

        m_subsystems.pushBack( subsystem );
    }

    SubsystemInstance* SubsystemManager::FindSubsystem( zp_hash64_t id ) const
    {
        const zp_size_t index = m_subsystems.findIndexOf(
            [ &id ]( auto x ) -> zp_bool_t
            {
                return x->id == id;
            } );
        return index == Vector<SubsystemInstance*>::npos ? nullptr : m_subsystems[ index ];
    }
}
