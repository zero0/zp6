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
#include "Engine/MemoryLabels.h"

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

        zp_size_t getComponentTypeIndex( const ComponentType componentType ) const;

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

        void registerComponentSignature( const ComponentSignature& componentSignature );

        ComponentArchetypeManager* getComponentArchetype( const ComponentSignature& componentSignature ) const;

        ComponentType getComponentTypeFromTypeHash( zp_hash64_t typeHash ) const;

        TagType getTagTypeForTypeHash( zp_hash64_t typeHash ) const;

    private:
        struct RegisteredComponent
        {
            zp_hash64_t typeHash;
            zp_size_t size;
            ComponentType type;
        };

        struct RegisteredTag
        {
            zp_hash64_t typeHash;
            TagType type;
        };

        zp_size_t m_registeredComponents;
        zp_size_t m_registeredTags;

        RegisteredComponent m_components[kMaxComponentTypes];
        RegisteredTag m_tags[kMaxTagTypes];

        Vector<ComponentArchetypeManager*> m_componentArchetypes;

    public:
        const MemoryLabel memoryLabel;
    };

    struct EntityQuery
    {
        ComponentSignature componentSignature;
        ZP_BOOL32( tagsOnly );
        ZP_BOOL32( allStructureMatches );
        ZP_BOOL32( allTagsMatches );
    };

    //
    //
    //

    template<typename T0>
    struct EntityQueryCallbackT
    {
        typedef void (* Func)( Entity, T0* );
    };

    template<typename T0, typename T1>
    struct EntityQueryCallbackTT
    {
        typedef void (* Func)( Entity, T0*, T1* );
    };

    template<typename C0>
    struct EntityQueryCallbackC
    {
        typedef void (* Func)( Entity, const C0* );
    };

    template<typename C0, typename C1>
    struct EntityQueryCallbackCC
    {
        typedef void (* Func)( Entity, const C0*, const C1* );
    };

    template<typename T0, typename C1>
    struct EntityQueryCallbackTC
    {
        typedef void (* Func)( Entity, T0*, const C1* );
    };

    //
    //
    //

    class EntityComponentManager
    {
    public:
        explicit EntityComponentManager( MemoryLabel memoryLabel );

        ~EntityComponentManager();

        Entity createEntity();

        Entity createEntity( ComponentSignature componentSignature );

        void destroyEntity( Entity entity );

        void setEntityComponentSignature( Entity entity, ComponentSignature newComponentSignature );

        void registerComponentSignature( ComponentSignature componentSignature );

        ComponentType getComponentType( zp_hash64_t typeHash ) const;

        TagType getTagType( zp_hash64_t typeHash ) const;

        void findEntities( const EntityQuery* entityQuery, EntityQueryCallback entityQueryCallback ) const;

        void findEntities( const EntityQuery* entityQuery, Vector<Entity>& foundEntities ) const;

        const void* getComponentDataReadOnly( Entity entity, ComponentType componentType ) const;

        void* getComponentData( Entity entity, ComponentType componentType );

        template<typename T0>
        void queryEntities( const EntityQuery* entityQuery, typename EntityQueryCallbackT<T0>::Func callback )
        {
            const ComponentType componentType = getComponentType < T0 > ();

            Vector<Entity> foundEntities( 16, MemoryLabels::Temp );
            findEntities( entityQuery, foundEntities );

            for( Entity entity: foundEntities )
            {
                T0* data0 = static_cast<T0*>( getComponentData( entity, componentType ));

                callback( entity, data0 );
            }
        }

        template<typename C0>
        void queryEntities( const EntityQuery* entityQuery, typename EntityQueryCallbackC<C0>::Func callback )
        {
            const ComponentType componentType = getComponentType < C0 > ();

            Vector<Entity> foundEntities( 16, MemoryLabels::Temp );
            findEntities( entityQuery, foundEntities );

            for( Entity entity: foundEntities )
            {
                const C0* data0 = static_cast<const C0*>( getComponentDataReadOnly( entity, componentType ));

                callback( entity, data0 );
            }
        }


        template<typename T0, typename C1>
        void queryEntities( const EntityQuery* entityQuery, typename EntityQueryCallbackTC<T0, C1>::Func callback )
        {
            const ComponentType componentType0 = getComponentType < T0 > ();
            const ComponentType componentType1 = getComponentType < C1 > ();

            Vector<Entity> foundEntities( 16, MemoryLabels::Temp );
            findEntities( entityQuery, foundEntities );

            for( Entity entity: foundEntities )
            {
                T0* data0 = static_cast<T0*>( getComponentData( entity, componentType0 ));
                const C1* data1 = static_cast<const C1*>( getComponentDataReadOnly( entity, componentType1 ));

                callback( entity, data0, data1 );
            }
        }

        template<typename T>
        ComponentType getComponentType() const
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            return m_componentManager.getComponentTypeFromTypeHash( typeHash );
        }

        template<typename T>
        TagType getTagType() const
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            return m_componentManager.getTagTypeForTypeHash( typeHash );
        }

        template<typename T>
        ComponentType registerComponent()
        {
            ComponentDescriptor componentDescriptor {
                .typeHash = zp_type_hash<T>(),
                .size = sizeof( T ),
            };

            return m_componentManager.registerComponent( &componentDescriptor );
        }

        template<typename T>
        TagType registerTag()
        {
            TagDescriptor tagDescriptor {
                .typeHash = zp_type_hash<T>()
            };

            return m_componentManager.registerTag( &tagDescriptor );
        }

        template<typename T>
        void setComponentData( Entity entity, const T& data )
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            const ComponentSignature& componentSignature = m_entityManager.getSignature( entity );
            ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
            archetypeManager->setComponentData( entity, componentType, &data, sizeof( T ));
        }

        template<typename T>
        const T* getComponentDataReadOnly( Entity entity ) const
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            const ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            const ComponentSignature& componentSignature = m_entityManager.getSignature( entity );
            ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
            void* data = archetypeManager->getComponentData( entity, componentType );

            T* componentData = static_cast<T*>( data );
            return componentData;
        }

        template<typename T>
        T* getComponentData( Entity entity )
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            const ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            const ComponentSignature& componentSignature = m_entityManager.getSignature( entity );
            ComponentArchetypeManager* archetypeManager = m_componentManager.getComponentArchetype( componentSignature );
            void* data = archetypeManager->getComponentData( entity, componentType );

            T* componentData = static_cast<T*>( data );
            return componentData;
        }

    protected:

    private:
        EntityManager m_entityManager;
        ComponentManager m_componentManager;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_COMPONENT_H
