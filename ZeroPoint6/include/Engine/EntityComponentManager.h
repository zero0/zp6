//
// Created by phosg on 2/16/2022.
//

#ifndef ZP_ENTITYCOMPONENTMANAGER_H
#define ZP_ENTITYCOMPONENTMANAGER_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Vector.h"

#include "Engine/ComponentSignature.h"
#include "Engine/Entity.h"
#include "Engine/EntityQuery.h"
#include "Engine/MemoryLabels.h"
#include "Engine/Component.h"

namespace zp
{
    class EntityComponentManager;

    class EntityComponentCommandBuffer
    {
    public:
        EntityComponentCommandBuffer( zp_uint8_t* data, zp_size_t capacity );

        ~EntityComponentCommandBuffer() = default;

        Entity createEntity();

        Entity createEntity( const ComponentSignature& componentSignature );

        template<typename T>
        void setEntityComponentData( Entity entity, ComponentType componentType, const T& componentData )
        {
            setEntityComponentData( entity, componentType, &componentData, sizeof( T ) );
        }

        void setEntityComponentData( Entity entity, ComponentType componentType, const void* componentData, zp_size_t size );

        void replay( EntityComponentManager* entityComponentManager );

        zp_size_t size() const
        {
            return m_length;
        }

    private:
        Entity m_currentEntity;
        zp_uint8_t* m_buffer;
        zp_size_t m_length;
        zp_size_t m_capacity;
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

        Entity createEntity( const ComponentSignature& componentSignature );

        void destroyEntity( Entity entity );

        void setEntityTag( Entity entity, TagType tagType );

        void clearEntityTag( Entity entity, TagType tagType );

        void setEntityComponentSignature( Entity entity, const ComponentSignature& newComponentSignature );

        void registerComponentSignature( const ComponentSignature& componentSignature );

        ComponentType getComponentType( zp_hash64_t typeHash ) const;

        TagType getTagType( zp_hash64_t typeHash ) const;

        zp_size_t getComponentDataSize( ComponentType componentType ) const;

        void iterateEntities( const EntityQuery* entityQuery, EntityQueryIterator* iterator );

        zp_bool_t next( EntityQueryIterator* iterator ) const;

        const void* getComponentDataReadOnly( Entity entity, ComponentType componentType ) const;

        void* getComponentData( Entity entity, ComponentType componentType );

        void setComponentData( Entity entity, ComponentType componentType, const void* data, zp_size_t size );

        EntityComponentCommandBuffer* requestCommandBuffer();

        void replayCommandBuffers();

    private:
        template<class T>
        static void buildComponentSignature( const ComponentManager& m_componentManager, StructuralSignature& structuralSignature )
        {
            structuralSignature |= 1 << m_componentManager.getComponentTypeFromTypeHash( zp_type_hash<T>() );
        }

        template<class T, class ... TArgs>
        static void buildComponentsSignature( const ComponentManager& m_componentManager, StructuralSignature& structuralSignature )
        {
            buildComponentSignature<T>( m_componentManager, structuralSignature );
            if constexpr( sizeof...( TArgs ) > 0 )
            {
                buildComponentsSignature<TArgs...>( m_componentManager, structuralSignature );
            }
        }

        template<class T>
        static void buildTagSignature( const ComponentManager& m_componentManager, TagSignature& tagSignature )
        {
            tagSignature |= 1 << m_componentManager.getTagTypeFromTypeHash( zp_type_hash<T>() );
        }

        template<class T, class ... TArgs>
        static void buildTagsSignature( const ComponentManager& m_componentManager, TagSignature& tagSignature )
        {
            buildTagSignature<T>( m_componentManager, tagSignature );
            if constexpr( sizeof...( TArgs ) > 0 )
            {
                buildTagsSignature<TArgs...>( m_componentManager, tagSignature );
            }
        }

    public:
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
            return m_componentManager.getTagTypeFromTypeHash( typeHash );
        }

        template<typename ... T>
        TagSignature getTagSignature() const
        {
            TagSignature tagSignature = 0;
            buildTagsSignature<T...>( m_componentManager, tagSignature );
            return tagSignature;
        }

        template<typename ... T>
        StructuralSignature getComponentSignature() const
        {
            StructuralSignature structuralSignature = 0;
            buildComponentsSignature<T...>( m_componentManager, structuralSignature );
            return structuralSignature;
        }

    public:
        template<typename T>
        ComponentType registerComponent( DestroyComponentDataCallback destroyComponentDataCallback = nullptr )
        {
            ComponentDescriptor componentDescriptor {
                .typeHash = zp_type_hash<T>(),
                .size = sizeof( T ),
                .destroyCallback = destroyComponentDataCallback
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

    public:
        template<typename T>
        void setComponentData( Entity entity, const T& data )
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            setComponentData( entity, componentType, &data, sizeof( T ) );
        }

        template<typename T>
        const T* getComponentDataReadOnly( Entity entity ) const
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            const ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            const T* componentData = static_cast<const T*>( getComponentDataReadOnly( entity, componentType ) );
            return componentData;
        }

        template<typename T>
        T* getComponentData( Entity entity )
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();
            const ComponentType componentType = m_componentManager.getComponentTypeFromTypeHash( typeHash );

            T* componentData = static_cast<T*>( getComponentData( entity, componentType ) );
            return componentData;
        }

    protected:

    private:
        EntityManager m_entityManager;
        ComponentManager m_componentManager;
        Vector<EntityComponentCommandBuffer> m_commandBuffers;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_ENTITYCOMPONENTMANAGER_H
