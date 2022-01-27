//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_COMPONENT_H
#define ZP_COMPONENT_H

#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Vector.h"

#include "Engine/ComponentSignature.h"
#include "Engine/Entity.h"

#include <cstdarg>

namespace zp
{
    enum
    {
        kMaxComponentTypes = 64,
        kMaxTagTypes = 64,
        kMaxComponentsPerArchetype = 16,
        kMaxEntitiesPerArchetype = 64,
    };


    struct ComponentDescriptor
    {
        zp_hash64_t typeHash;
        zp_size_t size;
    };

    struct TagDescriptor
    {
        zp_hash64_t typeHash;
    };

    struct ComponentBlockArchetype
    {
        ComponentSignature componentSignature;
        zp_size_t totalStride;
        zp_size_t componentOffset[kMaxComponentsPerArchetype];
        ComponentType componentType[kMaxComponentsPerArchetype];
        zp_uint8_t componentCount;
    };

    class ComponentArchetypeManager
    {
    ZP_NONCOPYABLE( ComponentArchetypeManager );

    public:
        ComponentArchetypeManager( MemoryLabel memoryLabel, const ComponentBlockArchetype& archetype );

        ~ComponentArchetypeManager();

        const ComponentSignature& getComponentSignature() const;

        void addEntity( Entity entity );

        void removeEntity( Entity entity );

        void* getArchetypeData( Entity entity );

        void setArchetypeData( Entity entity, void* data, zp_size_t length );

        void* getComponentData( Entity entity, ComponentType componentType );

        void setComponentData( Entity entity, ComponentType componentType, void* data, zp_size_t length );

    public:
        const MemoryLabel memoryLabel;

    private:
        zp_size_t getEntityIndex( const Entity entity ) const;

        zp_uint8_t getComponentTypeIndex( const ComponentType componentType ) const;

        ComponentBlockArchetype m_componentBlockArchetype;

        struct ArchetypeBlock
        {
            ArchetypeBlock* next;
            ArchetypeBlock* prev;
            zp_uint64_t usedBits;
            zp_uint8_t* blockPtr;
        };

        ArchetypeBlock m_root;
        ArchetypeBlock* m_head;

        struct EntityMap
        {
            ArchetypeBlock* block;
            zp_size_t index;
        };

        Vector<EntityMap> m_entityMap;
        Vector<Entity> m_entities;
    };

    class ComponentManager
    {
    ZP_NONCOPYABLE( ComponentManager );

    public:
        ComponentManager( MemoryLabel memoryLabel );

        ComponentType registerComponent( ComponentDescriptor* componentDescriptor );

        TagType registerTag( TagDescriptor* tagDescriptor );

        void registerComponentSignature( const ComponentSignature componentSignature );

        ComponentArchetypeManager* getComponentArchetype( const ComponentSignature componentSignature ) const;

        ComponentType getComponentTypeFromTypeHash( const zp_hash64_t typeHash ) const;

    public:
        const MemoryLabel memoryLabel;

    private:
        zp_size_t m_registeredComponents;
        ComponentDescriptor m_componentDescriptors[kMaxComponentTypes];

        zp_size_t m_registeredTags;
        TagDescriptor m_tagDescriptors[kMaxTagTypes];

        Vector<ComponentArchetypeManager*> m_componentArchetypes;
    };

    class EntityComponentManager
    {
    public:
        Entity createEntity();

        Entity createEntity( ComponentSignature componentSignature );

        void destroyEntity( Entity entity );

        void setEntityComponentSignature( Entity entity, ComponentSignature newComponentSignature );

        void registerComponentSignature( ComponentSignature componentSignature );

        template<typename T>
        ComponentType registerComponent()
        {
            ComponentDescriptor componentDescriptor = {
                .typeHash = zp_type_hash<T>(),
                .size = sizeof( T ),
            };

            return m_componentManager.registerComponent( &componentDescriptor );
        }

        template<typename T>
        TagType registerTag()
        {
            TagDescriptor tagDescriptor = {
                .typeHash = zp_type_hash<T>()
            };

            return m_componentManager.registerTag( &tagDescriptor );
        }

        template<typename T>
        void setComponentData( Entity entity, const T& data )
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            ComponentSignature componentSignature = m_entityManager.getSignature( entity );
            ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
            archetypeManager->setComponentData( entity, componentType, &data, sizeof( T ));
        }

        template<typename T>
        T getComponentData( Entity entity ) const
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            const ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            const ComponentSignature componentSignature = m_entityManager.getSignature( entity );
            ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
            void* data = archetypeManager->getComponentData( entity, componentType );

            T* componentData = static_cast<T*>( data );
            return *componentData;
        }

        template<typename T>
        T* getComponentDataPtr( Entity entity )
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            const ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            const ComponentSignature componentSignature = m_entityManager.getSignature( entity );
            ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
            void* data = archetypeManager->getComponentData( entity, componentType );

            T* componentData = static_cast<T*>( data );
            return componentData;
        }

    protected:

    private:
        EntityManager m_entityManager;
        ComponentManager m_componentManager;
    };
}

#endif //ZP_COMPONENT_H
