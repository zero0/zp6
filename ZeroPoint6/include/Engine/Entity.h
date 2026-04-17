//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_ENTITY_H
#define ZP_ENTITY_H

#include "Core/Allocator.h"
#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Vector.h"

#include "Engine/ComponentSignature.h"

namespace zp
{
    struct Entity
    {
        enum
        {
            ZP_NULL_ENTITY = 0ULL
        };

        constexpr Entity() : m_id( ZP_NULL_ENTITY )
        {
        }

        constexpr Entity( const zp_uint64_t id ) : m_id( id )
        {
        }

        [[nodiscard]] constexpr auto id() const -> zp_uint64_t
        {
            return m_id;
        }

        [[nodiscard]] constexpr auto valid() const -> bool
        {
            return m_id != ZP_NULL_ENTITY;
        }

        [[nodiscard]] constexpr auto operator==( const Entity& other ) const -> bool
        {
            return m_id == other.m_id;
        }

    private:
        zp_uint64_t m_id;
    };

    typedef void ( *EntityQueryCallback )( Entity entity, const ComponentSignature& signature );

    class EntityQueryIterator;

    class EntityManager
    {
        ZP_NONCOPYABLE( EntityManager );

    public:
        explicit EntityManager( MemoryLabel memoryLabel );

        ~EntityManager();

        Entity createEntity();

        Entity createEntity( const ComponentSignature& signature );

        void destroyEntity( Entity entity );

        const ComponentSignature& getSignature( Entity entity ) const;

        void setSignature( Entity entity, const ComponentSignature& signature );

        zp_bool_t nextEntity( EntityQueryIterator* entityQueryIterator ) const;

    private:
        MemoryLabel m_entityMemoryLabel;
        MemoryLabel m_componentSignatureMemoryLabel;

        Vector<Entity> m_freeList;
        Vector<ComponentSignature> m_componentSignatures;

    public:
        const MemoryLabel memoryLabel;
    };
} // namespace zp
#endif // ZP_ENTITY_H
