//
// Created by phosg on 7/16/2023.
//

#ifndef ZP_COMMANDLINE_H
#define ZP_COMMANDLINE_H

#include "Core/Common.h"
#include "Core/Types.h"
#include "Core/String.h"
#include "Core/Vector.h"
#include "Core/Map.h"

namespace zp
{
    struct CommandLineOperationDesc
    {
        const char* shortName;
        const char* longName;
        const char* description;
        zp_size_t minParameterCount;
        zp_size_t maxParameterCount;
    };

    struct CommandLineOperation
    {
        zp_size_t id;
    };

    class CommandLine
    {
    public:
        explicit CommandLine( MemoryLabel memoryLabel );

        CommandLineOperation addOperation( const CommandLineOperationDesc& desc );

        zp_bool_t parse( const char* cmdLine );

        zp_bool_t hasFlag( const CommandLineOperation& operation ) const;

        zp_bool_t hasParameter( const CommandLineOperation& operation, Vector<AllocString>& outParameters ) const;

        void printHelp() const;

    private:
        Vector<CommandLineOperationDesc> m_commandLineOperations;
        Vector<AllocString> m_commandLineTokens;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_COMMANDLINE_H
