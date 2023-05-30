//
// Created by phosg on 2/16/2022.
//
#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Math.h"
#include "Core/Atomic.h"

#include "Engine/EntityQuery.h"
#include "Engine/Component.h"
#include "Engine/EntityComponentManager.h"

namespace zp
{
    namespace
    {
        enum EntityComponentCommandType : zp_uint32_t
        {
            ZP_ENTITY_COMPONENT_COMMAND_TYPE_CREATE_ENTITY,
            ZP_ENTITY_COMPONENT_COMMAND_TYPE_CREATE_ENTITY_WITH_SIGNATURE,
            ZP_ENTITY_COMPONENT_COMMAND_TYPE_SET_COMPONENT_DATA,
        };

        struct EntityComponentCommandCreateEntity
        {
            Entity entity;
        };

        struct EntityComponentCommandCreateEntityWithSignature
        {
            Entity entity;
            ComponentSignature componentSignature;
        };

        struct EntityComponentCommandSetComponentData
        {
            Entity entity;
            ComponentType componentType;
            zp_size_t size;
        };

        template<typename T>
        void write( zp_uint8_t* buffer, zp_size_t& length, const T& value )
        {
            const zp_size_t size = sizeof( T );
            *reinterpret_cast<T*>( buffer + length ) = value;
            length += size;
        }

        void write( zp_uint8_t* buffer, zp_size_t& length, const void* data, zp_size_t size )
        {
            zp_memcpy( buffer + length, size, data, size );
            length += size;
        }

        template<typename T>
        void read( const zp_uint8_t* buffer, zp_size_t& pos, T& value )
        {
            const zp_size_t size = sizeof( T );
            value = *reinterpret_cast<const T*>( buffer + pos );
            pos += size;
        }

        const void* read( const zp_uint8_t* buffer, zp_size_t& pos, zp_size_t size )
        {
            const void* ptr = buffer + pos;
            pos += size;

            return ptr;
        }
    }

    EntityComponentCommandBuffer::EntityComponentCommandBuffer( zp_uint8_t* data, zp_size_t capacity )
        : m_currentEntity( 0 )
        , m_buffer( data )
        , m_length( 0 )
        , m_capacity( capacity )
    {
    }

    Entity EntityComponentCommandBuffer::createEntity()
    {
        ZP_ASSERT( m_length < m_capacity );
        Entity entity = m_currentEntity++;

        EntityComponentCommandCreateEntity cmd {
            .entity = entity
        };

        write( m_buffer, m_length, ZP_ENTITY_COMPONENT_COMMAND_TYPE_CREATE_ENTITY );
        write( m_buffer, m_length, cmd );

        return entity;
    }

    Entity EntityComponentCommandBuffer::createEntity( const ComponentSignature& componentSignature )
    {
        ZP_ASSERT( m_length < m_capacity );
        Entity entity = m_currentEntity++;

        EntityComponentCommandCreateEntityWithSignature cmd {
            .entity = entity,
            .componentSignature = componentSignature
        };

        write( m_buffer, m_length, ZP_ENTITY_COMPONENT_COMMAND_TYPE_CREATE_ENTITY_WITH_SIGNATURE );
        write( m_buffer, m_length, cmd );

        return entity;
    }

    void EntityComponentCommandBuffer::setEntityComponentData( Entity entity, ComponentType componentType, const void* componentData, zp_size_t size )
    {
        ZP_ASSERT( m_length < m_capacity );
        EntityComponentCommandSetComponentData cmd {
            .entity = entity,
            .componentType = componentType,
            .size = size
        };

        write( m_buffer, m_length, ZP_ENTITY_COMPONENT_COMMAND_TYPE_SET_COMPONENT_DATA );
        write( m_buffer, m_length, cmd );

        write( m_buffer, m_length, componentData, size );
    }

    void EntityComponentCommandBuffer::replay( EntityComponentManager* entityComponentManager )
    {
        zp_size_t position = 0;

        Entity entityMap[64] {};

        while( position < m_length )
        {
            EntityComponentCommandType type {};
            read( m_buffer, position, type );

            switch( type )
            {
                case ZP_ENTITY_COMPONENT_COMMAND_TYPE_CREATE_ENTITY:
                {
                    EntityComponentCommandCreateEntity cmd {};
                    read( m_buffer, position, cmd );

                    Entity newEntity = entityComponentManager->createEntity();

                    entityMap[ cmd.entity ] = newEntity;
                }
                    break;

                case ZP_ENTITY_COMPONENT_COMMAND_TYPE_CREATE_ENTITY_WITH_SIGNATURE:
                {
                    EntityComponentCommandCreateEntityWithSignature cmd {};
                    read( m_buffer, position, cmd );

                    Entity newEntity = entityComponentManager->createEntity( cmd.componentSignature );

                    entityMap[ cmd.entity ] = newEntity;
                }
                    break;

                case ZP_ENTITY_COMPONENT_COMMAND_TYPE_SET_COMPONENT_DATA:
                {
                    EntityComponentCommandSetComponentData cmd {};
                    read( m_buffer, position, cmd );

                    const zp_size_t size = entityComponentManager->getComponentDataSize( cmd.componentType );
                    ZP_ASSERT( size == cmd.size );

                    const void* data = read( m_buffer, position, size );
                    entityComponentManager->setComponentData( entityMap[ cmd.entity ], cmd.componentType, data, size );
                }
                    break;

                default:
                    ZP_INVALID_CODE_PATH();
                    break;
            }
        }

        m_length = 0;
        m_currentEntity = 0;
    }

    //
    //
    //

    EntityComponentManager::EntityComponentManager( MemoryLabel memoryLabel )
        : m_entityManager( memoryLabel )
        , m_componentManager( memoryLabel )
        , m_commandBuffers( 4, memoryLabel )
        , memoryLabel( memoryLabel )
    {
    }

    EntityComponentManager::~EntityComponentManager()
    {
    }

    Entity EntityComponentManager::createEntity()
    {
        return m_entityManager.createEntity();
    }

    Entity EntityComponentManager::createEntity( const ComponentSignature& componentSignature )
    {
        Entity entity = m_entityManager.createEntity( componentSignature );

        m_componentManager.registerComponentSignature( componentSignature );
        ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );

        archetypeManager->addEntity( entity );

        return entity;
    }

    void EntityComponentManager::destroyEntity( Entity entity )
    {
        ComponentSignature componentSignature = m_entityManager.getSignature( entity );
        ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );

        archetypeManager->removeEntity( entity );

        m_entityManager.destroyEntity( entity );
    }

    void EntityComponentManager::setEntityTag( Entity entity, TagType tagType )
    {
        ComponentSignature componentSignature = m_entityManager.getSignature( entity );
        componentSignature.tagSignature |= 1 << tagType;
        m_entityManager.setSignature( entity, componentSignature );
    }

    void EntityComponentManager::clearEntityTag( Entity entity, TagType tagType )
    {
        ComponentSignature componentSignature = m_entityManager.getSignature( entity );
        componentSignature.tagSignature &= ~( 1 << tagType );
        m_entityManager.setSignature( entity, componentSignature );
    }

    void EntityComponentManager::setEntityComponentSignature( Entity entity, const ComponentSignature& newComponentSignature )
    {
        const ComponentSignature& componentSignature = m_entityManager.getSignature( entity );
        if( componentSignature.structuralSignature != newComponentSignature.structuralSignature )
        {
            // TODO: move component data to new archetype
            ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
            if( archetypeManager ) archetypeManager->removeEntity( entity );

            m_componentManager.registerComponentSignature( newComponentSignature );

            ComponentArchetypeManager* newArchetypeManager = m_componentManager.getComponentArchetype( newComponentSignature );
            if( newArchetypeManager ) newArchetypeManager->addEntity( entity );

            m_entityManager.setSignature( entity, newComponentSignature );
        }
        else if( componentSignature.tagSignature != newComponentSignature.tagSignature )
        {
            m_entityManager.setSignature( entity, newComponentSignature );
        }
    }

    void EntityComponentManager::registerComponentSignature( const ComponentSignature& componentSignature )
    {
        m_componentManager.registerComponentSignature( componentSignature );
    }

    ComponentType EntityComponentManager::getComponentType( zp_hash64_t typeHash ) const
    {
        return m_componentManager.getComponentTypeFromTypeHash( typeHash );
    }

    TagType EntityComponentManager::getTagType( zp_hash64_t typeHash ) const
    {
        return m_componentManager.getTagTypeFromTypeHash( typeHash );
    }

    zp_size_t EntityComponentManager::getComponentDataSize( ComponentType componentType ) const
    {
        return m_componentManager.getComponentDataSize( componentType );
    }

    void EntityComponentManager::iterateEntities( const EntityQuery* entityQuery, EntityQueryIterator* iterator )
    {
        iterator->m_query = *entityQuery;
        iterator->m_current = ZP_NULL_ENTITY;
        iterator->m_entityComponentManager = this;
    }

    zp_bool_t EntityComponentManager::next( EntityQueryIterator* iterator ) const
    {
        return m_entityManager.nextEntity( iterator );
    }

    const void* EntityComponentManager::getComponentDataReadOnly( Entity entity, ComponentType componentType ) const
    {
        const ComponentSignature& componentSignature = m_entityManager.getSignature( entity );
        ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
        const void* data = archetypeManager->getComponentData( entity, componentType );
        return data;
    }

    void* EntityComponentManager::getComponentData( Entity entity, ComponentType componentType )
    {
        const ComponentSignature& componentSignature = m_entityManager.getSignature( entity );
        ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
        void* data = archetypeManager->getComponentData( entity, componentType );
        return data;
    }

    void EntityComponentManager::setComponentData( Entity entity, ComponentType componentType, const void* data, zp_size_t size )
    {
        const ComponentSignature& componentSignature = m_entityManager.getSignature( entity );
        ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
        archetypeManager->setComponentData( entity, componentType, data, size );
    }

    EntityComponentCommandBuffer* EntityComponentManager::requestCommandBuffer()
    {
        return nullptr;
    }

    void EntityComponentManager::replayCommandBuffers()
    {
        m_commandBuffers.clear();
    }
}