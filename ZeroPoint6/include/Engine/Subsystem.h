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
        [[nodiscard]] zp_bool_t isValid() const { return m_subsystem != nullptr; }

        T* operator->() { return m_subsystem; }

        const T* operator->() const { return m_subsystem; }

    private:
        T* m_subsystem;
    };

}

#endif //ZP_SUBSYSTEM_H
