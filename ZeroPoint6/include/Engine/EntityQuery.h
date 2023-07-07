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

        [[nodiscard]] Entity current() const
        {
            return m_current;
        }

    private:
        void* getComponentDataByType( zp_hash64_t componentTypeHash );

        [[nodiscard]] const void* getComponentDataReadOnlyByType( zp_hash64_t componentTypeHash ) const;

        void addTagByType( zp_hash64_t tagTypeHash );

        void removeTagByType( zp_hash64_t tagTypeHash );

    public:
        template<typename T>
        T* getComponentData()
        {
            return static_cast<T*>( getComponentDataByType( zp_type_hash<T>() ) );
        }

        template<typename T>
        const T* getComponentDataReadOnly() const
        {
            return static_cast<const T*>( getComponentDataReadOnlyByType( zp_type_hash<T>() ) );
        }

        template<typename T>
        void addTag()
        {
            addTagByType( zp_type_hash<T>() );
        }

        template<typename T>
        void removeTag()
        {
            removeTagByType( zp_type_hash<T>() );
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
