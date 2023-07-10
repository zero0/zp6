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
        kMaxEntitiesPerArchetypeBlock = 64,
    };
    ZP_STATIC_ASSERT( kMaxEntitiesPerArchetypeBlock == sizeof(zp_uint64_t) * 8);

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

        [[nodiscard]] const ComponentSignature& getComponentSignature() const;

        void addEntity( Entity entity );

        void removeEntity( Entity entity );

        void* getArchetypeData( Entity entity );

        void setArchetypeData( Entity entity, void* data, zp_size_t length );

        void* getComponentData( Entity entity, ComponentType componentType );

        void setComponentData( Entity entity, ComponentType componentType, const void* data, zp_size_t length );

    private:
        [[nodiscard]] zp_size_t getEntityIndex( Entity entity ) const;

        [[nodiscard]] zp_size_t getComponentTypeIndex( ComponentType componentType ) const;

        ComponentBlockArchetype m_componentBlockArchetype;

        struct ArchetypeBlock
        {
            ArchetypeBlock* next;
            ArchetypeBlock* prev;
            zp_uint8_t* blockPtr;
            zp_uint64_t usedBits;
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

        ComponentType registerComponent( const ComponentDescriptor& componentDescriptor );

        TagType registerTag( const TagDescriptor& tagDescriptor );

        void registerComponentSignature( const ComponentSignature& componentSignature );

        [[nodiscard]] ComponentArchetypeManager* getComponentArchetype( const ComponentSignature& componentSignature ) const;

        [[nodiscard]] ComponentType getComponentTypeFromTypeHash( zp_hash64_t typeHash ) const;

        [[nodiscard]] TagType getTagTypeFromTypeHash( zp_hash64_t typeHash ) const;

        [[nodiscard]] zp_size_t getComponentDataSize( ComponentType componentType ) const;

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
