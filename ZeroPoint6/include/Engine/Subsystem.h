//
// Created by phosg on 3/25/2023.
//

#ifndef ZP_SUBSYSTEM_H
#define ZP_SUBSYSTEM_H

namespace zp
{
    class ISubsystem
    {
    };

    template<typename T>
    struct SubsystemHandle
    {
    public:
        zp_bool_t isValid() const;

        T* operator->() { return m_subsystem; }

        const T* operator->() const { return m_subsystem; }

    private:
        T* m_subsystem;
    };

    class SubsystemEngine
    {
    public:
        template<typename T>
        void registerSubsystem();

        template<typename T>
        zp_bool_t tryGetSubsystem( SubsystemHandle<T>& handle ) const;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_SUBSYSTEM_H
