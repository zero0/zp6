//
// Created by phosg on 2/9/2022.
//

#ifndef ZP_GRAPHICSRESOURCE_H
#define ZP_GRAPHICSRESOURCE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"

namespace zp
{
    ZP_DECLSPEC_NOVTABLE class BaseGraphicsResource
    {
    public:
        void addRef();

        void removeRef();

    protected:
        zp_size_t m_refCount;
    };

    template<typename T>
    class GraphicsResource final : public BaseGraphicsResource
    {
    public:
        const T& get() const
        {
            return m_resource;
        }

        T& get()
        {
            return m_resource;
        }

        const T* data() const
        {
            return &m_resource;
        }

        T* data()
        {
            return &m_resource;
        }

    private:
        T m_resource;
    };

    template<typename T>
    class GraphicsResourceHandle
    {
    public:
        typedef T* type_pointer;
        typedef const T* const_type_pointer;
        typedef GraphicsResource<T>* resource_pointer;
        typedef const GraphicsResource<T>& const_ref_resource;
        typedef const GraphicsResourceHandle<T>& const_ref_handle;
        typedef GraphicsResourceHandle<T>& ref_handle;
        typedef GraphicsResourceHandle<T>&& move_handle;

        GraphicsResourceHandle()
            : m_resource( nullptr )
        {
        }

        explicit GraphicsResourceHandle( resource_pointer resource )
            : m_resource( resource )
        {
            if( m_resource ) m_resource->addRef();
        }

        GraphicsResourceHandle( const_ref_handle other )
            : m_resource( other.m_resource )
        {
            if( m_resource ) m_resource->addRef();
        }

        GraphicsResourceHandle( move_handle other ) noexcept
            : m_resource( other.m_resource )
        {
            other.m_resource = nullptr;
        }

        ~GraphicsResourceHandle()
        {
            if( m_resource ) m_resource->removeRef();
            m_resource = nullptr;
        }

        GraphicsResourceHandle& operator=( const_ref_handle other )
        {
            if( m_resource ) m_resource->removeRef();

            m_resource = other.m_resource;

            if( m_resource ) m_resource->addRef();

            return *this;
        }

        GraphicsResourceHandle& operator=( move_handle other )
        {
            if( m_resource ) m_resource->removeRef();

            m_resource = other.m_resource;
            other.m_resource = nullptr;

            return *this;
        }

        const_type_pointer data() const
        {
            return m_resource ? m_resource->data() : nullptr;
        }

        type_pointer data()
        {
            return m_resource ? m_resource->data() : nullptr;
        }

        type_pointer operator->()
        {
            return m_resource ? m_resource->data() : nullptr;
        }

        const_type_pointer operator->() const
        {
            return m_resource ? m_resource->data() : nullptr;
        }

        void release()
        {
            if( m_resource ) m_resource->removeRef();
            m_resource = nullptr;
        }

        [[nodiscard]] zp_hash64_t hash() const
        {
            return static_cast<zp_hash64_t>( m_resource );
        }

        [[nodiscard]] zp_bool_t isValid() const
        {
            return m_resource != nullptr;
        }

    private:
        resource_pointer m_resource;
    };
}

#endif //ZP_GRAPHICSRESOURCE_H
