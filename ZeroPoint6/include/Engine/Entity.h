//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_ENTITY_H
#define ZP_ENTITY_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Allocator.h"
#include "Core/Vector.h"

#include "Engine/ComponentSignature.h"

namespace zp
{
    typedef zp_uint64_t Entity;

    enum : Entity
    {
        ZP_NULL_ENTITY = ~0ULL
    };

    typedef void (*EntityQueryCallback)( Entity entity, const ComponentSignature& signature );

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
}
#endif //ZP_ENTITY_H
