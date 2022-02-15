//
// Created by phosg on 1/24/2022.
//

#include "Core/Allocator.h"

#include "Engine/Entity.h"
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

    void EntityManager::findEntitiesAll( const ComponentSignature& signature, EntityQueryCallback entityQueryCallback ) const
    {
        for( Entity entity = 0; entity < m_componentSignatures.size(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity ];
            if((componentSignature.structuralSignature & signature.structuralSignature) == signature.structuralSignature &&
               (componentSignature.tagSignature & signature.tagSignature) == signature.tagSignature )
            {
                entityQueryCallback( entity, componentSignature );
            }
        }
    }

    void EntityManager::findEntitiesAny( const ComponentSignature& signature, EntityQueryCallback entityQueryCallback ) const
    {
        for( Entity entity = 0; entity < m_componentSignatures.size(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity ];
            if((componentSignature.structuralSignature & signature.structuralSignature) != 0 &&
               (componentSignature.tagSignature & signature.tagSignature) != 0 )
            {
                entityQueryCallback( entity, componentSignature );
            }
        }
    }

    void EntityManager::findEntitiesWithTagsAll( const ComponentSignature& signature, EntityQueryCallback entityQueryCallback ) const
    {
        for( Entity entity = 0; entity < m_componentSignatures.size(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity ];
            if((componentSignature.tagSignature & signature.tagSignature) == signature.tagSignature )
            {
                entityQueryCallback( entity, componentSignature );
            }
        }
    }

    void EntityManager::findEntitiesWithTagsAny( const ComponentSignature& signature, EntityQueryCallback entityQueryCallback ) const
    {
        for( Entity entity = 0; entity < m_componentSignatures.size(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity ];
            if((componentSignature.tagSignature & signature.tagSignature) != 0 )
            {
                entityQueryCallback( entity, componentSignature );
            }
        }
    }

    void EntityManager::findEntitiesAll( const ComponentSignature& signature, Vector<Entity>& foundEntities ) const
    {
        for( Entity entity = 0; entity < m_componentSignatures.size(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity ];
            if((componentSignature.structuralSignature & signature.structuralSignature) == signature.structuralSignature &&
               (componentSignature.tagSignature & signature.tagSignature) == signature.tagSignature )
            {
                foundEntities.pushBack( entity );
            }
        }
    }

    void EntityManager::findEntitiesAny( const ComponentSignature& signature, Vector<Entity>& foundEntities ) const
    {
        for( Entity entity = 0; entity < m_componentSignatures.size(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity ];
            if((componentSignature.structuralSignature & signature.structuralSignature) != 0 &&
               (componentSignature.tagSignature & signature.tagSignature) != 0 )
            {
                foundEntities.pushBack( entity );
            }
        }
    }

    void EntityManager::findEntitiesWithTagsAll( const ComponentSignature& signature, Vector<Entity>& foundEntities ) const
    {
        for( Entity entity = 0; entity < m_componentSignatures.size(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity ];
            if((componentSignature.tagSignature & signature.tagSignature) == signature.tagSignature )
            {
                foundEntities.pushBack( entity );
            }
        }
    }

    void EntityManager::findEntitiesWithTagsAny( const ComponentSignature& signature, Vector<Entity>& foundEntities ) const
    {
        for( Entity entity = 0; entity < m_componentSignatures.size(); ++entity )
        {
            const ComponentSignature& componentSignature = m_componentSignatures[ entity ];
            if((componentSignature.tagSignature & signature.tagSignature) != 0 )
            {
                foundEntities.pushBack( entity );
            }
        }
    }
}