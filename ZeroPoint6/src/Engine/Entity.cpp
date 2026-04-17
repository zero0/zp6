//
// Created by phosg on 1/24/2022.
//

#include "Core/Allocator.h"

#include "Engine/Entity.h"
#include "Engine/EntityQuery.h"
#include "Engine/MemoryLabels.h"

namespace zp
{
    EntityManager::EntityManager( MemoryLabel memoryLabel )
        : m_componentSignatureMemoryLabel( memoryLabel )
        , m_entityMemoryLabel( memoryLabel )
        , m_componentSignatures( 16, m_componentSignatureMemoryLabel )
        , m_freeList( 16, m_entityMemoryLabel )
        , memoryLabel( memoryLabel )
    {
    }

    EntityManager::~EntityManager()
    {
    }

    Entity EntityManager::createEntity()
    {
        Entity entity;

        if( !m_freeList.isEmpty() )
        {
            entity = m_freeList.back();
            m_freeList.popBack();
        }
        else
        {
            entity = { m_componentSignatures.length() };
            m_componentSignatures.pushBackEmpty();
        }

        return entity;
    }

    Entity EntityManager::createEntity( const ComponentSignature& signature )
    {
        Entity entity = createEntity();

        m_componentSignatures[ entity.id() ] = signature;

        return entity;
    }

    void EntityManager::destroyEntity( Entity entity )
    {
        m_componentSignatures[ entity.id() ] = {};

        m_freeList.pushBack( entity );
    }

    const ComponentSignature& EntityManager::getSignature( Entity entity ) const
    {
        return m_componentSignatures[ entity.id() ];
    }

    void EntityManager::setSignature( Entity entity, const ComponentSignature& signature )
    {
        m_componentSignatures[ entity.id() ] = zp_move( signature );
    }

    zp_bool_t EntityManager::nextEntity( EntityQueryIterator* entityQueryIterator ) const
    {
        Entity entity = entityQueryIterator->m_current == Entity() ? 0 : entityQueryIterator->m_current; // + 1;
        zp_bool_t found = false;

        //for( ; entity < m_componentSignatures.length(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity.id() ];
            if( componentSignature.tagSignature != 0 && componentSignature.structuralSignature != 0 )
            {
                EntityQuery& entityQuery = entityQueryIterator->m_query;

                zp_bool_t pass = ( componentSignature.tagSignature & entityQuery.requiredTags ) == entityQuery.requiredTags;
                pass &= pass && ( componentSignature.tagSignature | ~entityQuery.notIncludedStructures ) == ~entityQuery.requiredTags;
                pass &= pass && ( entityQuery.anyTags == 0 || ( componentSignature.tagSignature & entityQuery.anyTags ) != 0 );

                pass &= pass && ( componentSignature.structuralSignature & entityQuery.requiredStructures ) == entityQuery.requiredStructures;
                pass &= pass && ( componentSignature.structuralSignature | ~entityQuery.notIncludedStructures ) == ~entityQuery.requiredStructures;
                pass &= pass && ( entityQuery.anyStructures == 0 || ( componentSignature.structuralSignature & entityQuery.anyStructures ) != 0 );

                if( pass )
                {
                    entityQueryIterator->m_current = entity;
                    found = true;
                    //break;
                }
            }
        }

        return found;
    }
}
