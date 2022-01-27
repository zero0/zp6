//
// Created by phosg on 1/24/2022.
//

#include "Core/Allocator.h"
#include "Engine/Entity.h"

namespace zp
{
    EntityManager::EntityManager( MemoryLabel memoryLabel )
        : memoryLabel( memoryLabel )
        , m_componentSignatureMemoryLabel( memoryLabel )
        , m_entityMemoryLabel( memoryLabel )
        , m_componentSignatures( 16, m_componentSignatureMemoryLabel )
        , m_freeList( 16, m_entityMemoryLabel )
    {
    }

    EntityManager::~EntityManager()
    {
    }

    Entity EntityManager::createEntity()
    {
        Entity entity = ZP_NULL_ENTITY;

        if( !m_freeList.isEmpty())
        {
            entity = m_freeList.back();
            m_freeList.popBack();
        }
        else
        {
            entity = m_componentSignatures.size();
            m_componentSignatures.pushBackEmpty();
        }

        return entity;
    }

    Entity EntityManager::createEntity( const ComponentSignature& signature )
    {
        Entity entity = createEntity();

        m_componentSignatures[ entity ] = signature;

        return entity;
    }

    void EntityManager::destroyEntity( Entity entity )
    {
        m_componentSignatures[ entity ] = {};

        m_freeList.pushBack( entity );
    }

    const ComponentSignature& EntityManager::getSignature( Entity entity ) const
    {
        return m_componentSignatures[ entity ];
    }

    void EntityManager::setSignature( Entity entity, const ComponentSignature& signature )
    {
        m_componentSignatures[ entity ] = signature;
    }
}