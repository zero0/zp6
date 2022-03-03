//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_COMPONENT_H
#define ZP_COMPONENT_H

#include "Core/Defines.h"
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

    typedef void (* DestroyComponentDataCallback)( void* componentData, zp_size_t componentSize );

    struct ComponentDescriptor
    {
        zp_hash64_t typeHash;
        zp_size_t size;
        DestroyComponentDataCallback destroyCallback;
    };

    struct TagDescriptor
    {
        zp_hash64_t typeHash;
    };

    struct ComponentBlockArchetype
    {
        ComponentSignature componentSignature;
        zp_uint32_t totalStride;
        zp_uint32_t componentCount;
        zp_uint32_t componentSize[kMaxComponentsPerArchetype];
        zp_uint32_t componentOffset[kMaxComponentsPerArchetype];
        ComponentType componentType[kMaxComponentsPerArchetype];
        DestroyComponentDataCallback destroyCallbacks[kMaxComponentsPerArchetype];
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

        void setComponentData( Entity entity, ComponentType componentType, const void* data, zp_size_t length );

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

    public:
        const MemoryLabel memoryLabel;
    };

    //
    //
    //

    class ComponentManager
    {
    ZP_NONCOPYABLE( ComponentManager );

    public:
        explicit ComponentManager( MemoryLabel memoryLabel );

        ComponentType registerComponent( ComponentDescriptor* componentDescriptor );

        TagType registerTag( TagDescriptor* tagDescriptor );

        void registerComponentSignature( const ComponentSignature& componentSignature );

        ComponentArchetypeManager* getComponentArchetype( const ComponentSignature& componentSignature ) const;

        ComponentType getComponentTypeFromTypeHash( zp_hash64_t typeHash ) const;

        TagType getTagTypeFromTypeHash( zp_hash64_t typeHash ) const;

        zp_size_t getComponentDataSize( ComponentType componentType ) const;

    private:
        struct RegisteredComponent
        {
            zp_hash64_t typeHash;
            zp_size_t size;
            ComponentType type;
            DestroyComponentDataCallback destroyCallback;
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
}

#endif //ZP_COMPONENT_H
