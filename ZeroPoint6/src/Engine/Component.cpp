//
// Created by phosg on 1/24/2022.
//

#include "Core/Defines.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

#include "Engine/Component.h"
#include "Engine/MemoryLabels.h"

namespace zp
{

    ComponentArchetypeManager::ComponentArchetypeManager( MemoryLabel memoryLabel, const ComponentBlockArchetype& archetype )
        : m_componentBlockArchetype( archetype )
        , m_root()
        , m_head( &m_root )
        , m_entityMap( 4, memoryLabel )
        , m_entities( 4, memoryLabel )
        , memoryLabel( memoryLabel )
    {
        m_head->next = m_head;
        m_head->prev = m_head;
        m_head->blockPtr = nullptr;
        m_head->usedBits = ~0ULL;
    }

    ComponentArchetypeManager::~ComponentArchetypeManager()
    {
        if( m_head )
        {
            ArchetypeBlock* block = m_head->next;
            while( block != m_head )
            {
                ArchetypeBlock* next = block->next;
                ZP_FREE_( memoryLabel, block );

                block = next;
            }

            m_head = nullptr;
        }
    }

    const ComponentSignature& ComponentArchetypeManager::getComponentSignature() const
    {
        return m_componentBlockArchetype.componentSignature;
    }

    void ComponentArchetypeManager::addEntity( Entity entity )
    {
        m_entities.pushBack( entity );

        zp_bool_t found = true;
        ArchetypeBlock* freeBlock = m_head->next;

        do
        {
            freeBlock = freeBlock->next;

            if( freeBlock == m_head )
            {
                found = false;
                break;
            }
        } while( freeBlock->usedBits != ~0ULL );

        if( !found )
        {
            // allocate block memory
            void* blockMemory = ZP_MALLOC_( memoryLabel, sizeof( ArchetypeBlock ) + kMaxEntitiesPerArchetypeBlock * m_componentBlockArchetype.totalStride );

            freeBlock = static_cast<ArchetypeBlock*>( blockMemory );
            freeBlock->usedBits = 0;
            freeBlock->blockPtr = static_cast<zp_uint8_t*>(blockMemory) + sizeof( ArchetypeBlock );

            // insert the new block at the head of the ring buffer
            freeBlock->next = m_head->next;
            freeBlock->prev = m_head;

            m_head->next->prev = freeBlock;
            m_head->next = freeBlock;
        }

        zp_int32_t index;
        if( freeBlock->usedBits == 0 )
        {
            index = 0;
        }
        else
        {
            index = zp_bitscan_reverse( freeBlock->usedBits );
            ++index;
        }

        freeBlock->usedBits |= 1 << index;

        EntityMap entityMap {
            .block = freeBlock,
            .index = static_cast<zp_size_t>(index),
        };

        m_entityMap.pushBack( entityMap );
    }

    void ComponentArchetypeManager::removeEntity( Entity entity )
    {
        zp_size_t index = getEntityIndex( entity );
        if( index != Vector<Entity>::npos )
        {
            const EntityMap& entityMap = m_entityMap[ index ];
            EntityMap& endEntityMap = m_entityMap.back();

            ArchetypeBlock* block = entityMap.block;

            // destroy component data
            zp_uint8_t* componentData = entityMap.block->blockPtr;
            componentData += m_componentBlockArchetype.totalStride * entityMap.index;

            for( zp_uint32_t i = 0; i < m_componentBlockArchetype.componentCount; ++i )
            {
                DestroyComponentDataCallback callback = m_componentBlockArchetype.destroyCallbacks[ i ];
                if( callback )
                {
                    callback( componentData + m_componentBlockArchetype.componentOffset[ i ], m_componentBlockArchetype.componentSize[ i ] );
                }
            }

            // copy end data block to index that it will be at after swap back
            zp_uint8_t* srcArchData = block->blockPtr + m_componentBlockArchetype.totalStride * endEntityMap.index;
            zp_uint8_t* dstArchData = block->blockPtr + m_componentBlockArchetype.totalStride * entityMap.index;

            zp_memcpy( dstArchData, m_componentBlockArchetype.totalStride, srcArchData, m_componentBlockArchetype.totalStride );

            // remove top used bit
            block->usedBits >>= 1;

            // set end index to new swapped index
            endEntityMap.index = entityMap.index;

            // remove entity and entity map
            m_entities.eraseAtSwapBack( index );
            m_entityMap.eraseAtSwapBack( index );

            // if there is nothing being used, move block to the end of the ring buffer
            if( block->usedBits == 0 )
            {
                block->next = m_head;
                block->prev = m_head->prev;

                m_head->prev->next = block;
                m_head->prev = block;
            }
        }
    }

    void* ComponentArchetypeManager::getArchetypeData( Entity entity )
    {
        zp_size_t index = getEntityIndex( entity );

        const EntityMap& map = m_entityMap[ index ];

        zp_uint8_t* archetypeData = map.block->blockPtr;
        archetypeData += m_componentBlockArchetype.totalStride * map.index;

        return archetypeData;
    }

    void ComponentArchetypeManager::setArchetypeData( Entity entity, void* data, zp_size_t length )
    {
        zp_size_t index = getEntityIndex( entity );

        const EntityMap& map = m_entityMap[ index ];

        zp_uint8_t* archetypeData = map.block->blockPtr;
        archetypeData += m_componentBlockArchetype.totalStride * map.index;

        zp_memcpy( archetypeData, m_componentBlockArchetype.totalStride, data, length );
    }

    void* ComponentArchetypeManager::getComponentData( Entity entity, ComponentType componentType )
    {
        const zp_size_t index = getEntityIndex( entity );
        const zp_uint8_t componentIndex = getComponentTypeIndex( componentType );

        const EntityMap& map = m_entityMap[ index ];

        zp_uint8_t* componentData = map.block->blockPtr;
        componentData += m_componentBlockArchetype.totalStride * map.index;
        componentData += m_componentBlockArchetype.componentOffset[ componentIndex ];

        return componentData;
    }

    void ComponentArchetypeManager::setComponentData( Entity entity, ComponentType componentType, const void* data, zp_size_t length )
    {
        const zp_size_t index = getEntityIndex( entity );
        const zp_uint8_t componentIndex = getComponentTypeIndex( componentType );

        const EntityMap& map = m_entityMap[ index ];

        zp_uint8_t* componentData = map.block->blockPtr;
        componentData += m_componentBlockArchetype.totalStride * map.index;
        componentData += m_componentBlockArchetype.componentOffset[ componentIndex ];

        zp_memcpy( componentData, length, data, length );
    }

    zp_size_t ComponentArchetypeManager::getEntityIndex( const Entity entity ) const
    {
        zp_size_t index = Vector<Entity>::npos;
        for( zp_size_t i = 0; i < m_entities.size(); ++i )
        {
            if( m_entities[ i ] == entity )
            {
                index = i;
                break;
            }
        }

        return index;
    }

    zp_size_t ComponentArchetypeManager::getComponentTypeIndex( const ComponentType componentType ) const
    {
        zp_size_t index = ~0;

        for( zp_size_t i = 0; i < kMaxComponentsPerArchetype; ++i )
        {
            if( m_componentBlockArchetype.componentType[ i ] == componentType )
            {
                index = i;
                break;
            }
        }

        return index;
    }

    //
    //
    //

    ComponentManager::ComponentManager( MemoryLabel memoryLabel )
        : m_registeredComponents( 0 )
        , m_registeredTags( 0 )
        , m_components {}
        , m_tags {}
        , m_componentArchetypes( 16, memoryLabel )
        , memoryLabel( memoryLabel )
    {
    }

    ComponentType ComponentManager::registerComponent( const ComponentDescriptor& componentDescriptor )
    {
        ZP_ASSERT( m_registeredComponents < kMaxComponentTypes );
        ComponentType componentType = m_registeredComponents;

        m_components[ m_registeredComponents ] = {
            componentDescriptor.typeHash,
            componentDescriptor.size,
            componentType,
            componentDescriptor.destroyCallback
        };
        ++m_registeredComponents;

        return componentType;
    }

    TagType ComponentManager::registerTag( const TagDescriptor& tagDescriptor )
    {
        ZP_ASSERT( m_registeredTags < kMaxTagTypes );
        TagType tagType = m_registeredTags;

        m_tags[ m_registeredTags ] = {
            tagDescriptor.typeHash,
            tagType
        };
        ++m_registeredTags;

        return tagType;
    }

#if 0
    void ComponentManager::registerComponentSignatureFromComponentTypes( zp_uint32_t count, ... )
    {
        ComponentSignature componentSignature {};

        va_list args;
        va_start( args, count );

        for( zp_uint32_t i = 0; i < count; ++i )
        {
            ComponentType type = va_arg( args, int );
            componentSignature.structuralSignature |= 1 << type;
        }

        va_end( args );

        registerComponentSignature( componentSignature );
    }
#endif

    void ComponentManager::registerComponentSignature( const ComponentSignature& componentSignature )
    {
        if( componentSignature.structuralSignature != 0 )
        {
            zp_bool_t found = false;
            for( const ComponentArchetypeManager* componentArchetype : m_componentArchetypes )
            {
                if( componentArchetype->getComponentSignature().structuralSignature == componentSignature.structuralSignature )
                {
                    found = true;
                    break;
                }
            }

            if( !found )
            {
                ComponentBlockArchetype archetype {
                    .componentSignature = componentSignature,
                };
                archetype.componentSignature.tagSignature = 0;

                for( zp_size_t index = 0; index < kMaxComponentTypes && archetype.componentCount < kMaxComponentsPerArchetype; ++index )
                {
                    const zp_uint64_t componentMask = ~( 1 << index );
                    if( componentSignature.structuralSignature & componentMask )
                    {
                        const RegisteredComponent& registeredComponent = m_components[ index ];

                        archetype.componentSize[ archetype.componentCount ] = registeredComponent.size;
                        archetype.componentOffset[ archetype.componentCount ] = archetype.totalStride;
                        archetype.componentType[ archetype.componentCount ] = index;
                        archetype.destroyCallbacks[ archetype.componentCount ] = registeredComponent.destroyCallback;

                        archetype.totalStride += registeredComponent.size;
                        ++archetype.componentCount;
                    }
                }

                ComponentArchetypeManager* archetypeManager;
                archetypeManager = ZP_NEW_ARGS_( memoryLabel, ComponentArchetypeManager, archetype );

                m_componentArchetypes.pushBack( archetypeManager );
            }
        }
    }

    ComponentArchetypeManager* ComponentManager::getComponentArchetype( const ComponentSignature& componentSignature ) const
    {
        ComponentArchetypeManager* archetypeManager = nullptr;

        for( ComponentArchetypeManager* componentArchetype : m_componentArchetypes )
        {
            if( componentArchetype->getComponentSignature().structuralSignature == componentSignature.structuralSignature )
            {
                archetypeManager = componentArchetype;
                break;
            }
        }

        return archetypeManager;
    }

    ComponentType ComponentManager::getComponentTypeFromTypeHash( const zp_hash64_t typeHash ) const
    {
        ComponentType componentType {};

        for( const RegisteredComponent& component : m_components )
        {
            if( component.typeHash == typeHash )
            {
                componentType = component.type;
                break;
            }
        }

        return componentType;
    }

    TagType ComponentManager::getTagTypeFromTypeHash( zp_hash64_t typeHash ) const
    {
        TagType tagType {};

        for( const RegisteredTag& tag : m_tags )
        {
            if( tag.typeHash == typeHash )
            {
                tagType = tag.type;
                break;
            }
        }

        return tagType;
    }

    zp_size_t ComponentManager::getComponentDataSize( ComponentType componentType ) const
    {
        return m_components[ componentType ].size;
    }

}


