#include "Core/Defines.h"
#include "Core/Common.h"
#include "Core/Allocator.h"
#include "Core/CommandLine.h"
#include "Core/String.h"
#include "EntryPoint/EntryPoint.h"

#include "Tools/AssetCompiler.h"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    using namespace zp;

    EntryPointDesc desc;
    int ret = EntryPointMain<AssetCompilerApplication>( String::As( lpCmdLine ), desc );
    return ret;
}
