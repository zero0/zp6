//
// Created by phosg on 2/2/2022.
//

#ifndef ZP_ENGINEPIPELINE_H
#define ZP_ENGINEPIPELINE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"

#include "Engine/JobSystem.h"

namespace zp
{
    class Engine;

    ZP_DECLSPEC_NOVTABLE class EnginePipeline
    {
    public:

        virtual void onActivated()
        {
        };

        virtual void onDeactivated()
        {
        };

        virtual PreparedJobHandle onProcessPipeline( Engine* engine, const PreparedJobHandle& inputHandle ) = 0;

    private:
    };
}

#endif //ZP_ENGINEPIPELINE_H
