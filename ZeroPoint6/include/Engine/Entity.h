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

    class EntityManager
    {
    ZP_NONCOPYABLE( EntityManager );

    public:
        EntityManager( MemoryLabel memoryLabel );

        ~EntityManager();

        Entity createEntity();

        Entity createEntity( const ComponentSignature& signature );

        void destroyEntity( Entity entity );

        const ComponentSignature& getSignature( Entity entity ) const;

        void setSignature( Entity entity, const ComponentSignature& signature );

    public:
        const MemoryLabel memoryLabel;

    private:
        MemoryLabel m_entityMemoryLabel;
        MemoryLabel m_componentSignatureMemoryLabel;

        Vector<Entity> m_freeList;
        Vector<ComponentSignature> m_componentSignatures;
    };
}
#endif //ZP_ENTITY_H
