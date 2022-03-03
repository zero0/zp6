//
// Created by phosg on 2/16/2022.
//

#ifndef ZP_ENTITYQUERY_H
#define ZP_ENTITYQUERY_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Engine/Entity.h"

namespace zp
{
    struct EntityQuery
    {
       TagSignature requiredTags;
       TagSignature anyTags;
       TagSignature notIncludedTags;

       StructuralSignature requiredStructures;
       StructuralSignature anyStructures;
       StructuralSignature notIncludedStructures;
    };

    //
    //
    //

    class EntityComponentManager;

    class EntityManager;

    class EntityQueryIterator
    {
    public:
        zp_bool_t next();

        void destroyEntity();

        void* getComponentData( zp_hash64_t componentTypeHash );

        const void* getComponentDataReadOnly( zp_hash64_t componentTypeHash ) const;

        template<class T>
        T* getComponentData()
        {
            return getComponentData( zp_type_hash<T>());
        }

        template<class T>
        const T* getComponentDataReadOnly() const
        {
            return getComponentDataReadOnly( zp_type_hash<T>());
        }

        Entity current() const
        {
            return m_current;
        }

    private:
        EntityQuery m_query;
        Entity m_current;
        EntityComponentManager* m_entityComponentManager;

        friend class EntityComponentManager;

        friend class EntityManager;
    };
}

#endif //ZP_ENTITYQUERY_H
