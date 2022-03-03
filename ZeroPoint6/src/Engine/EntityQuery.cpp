//
// Created by phosg on 2/16/2022.
//

#include "Engine/EntityQuery.h"
#include "Engine/EntityComponentManager.h"

namespace zp
{
    void EntityQueryIterator::destroyEntity()
    {
        if( m_current != ZP_NULL_ENTITY )
        {
            m_entityComponentManager->destroyEntity( m_current );
        }
    }

    void* EntityQueryIterator::getComponentData( zp_hash64_t componentTypeHash )
    {
        return m_current == ZP_NULL_ENTITY ? nullptr : m_entityComponentManager->getComponentData( m_current, m_entityComponentManager->getComponentType( componentTypeHash ));
    }

    const void* EntityQueryIterator::getComponentDataReadOnly( zp_hash64_t componentTypeHash ) const
    {
        return m_current == ZP_NULL_ENTITY ? nullptr : m_entityComponentManager->getComponentDataReadOnly( m_current, m_entityComponentManager->getComponentType( componentTypeHash ));
    }

    zp_bool_t EntityQueryIterator::next()
    {
        return m_entityComponentManager->next( this );
    }
}