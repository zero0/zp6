
#include "Engine/Engine.h"
#include "EntryPoint/EntryPoint.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    using namespace zp;

    EntryPointDesc desc;
    int ret = EntryPointMain<Engine>( String::As( lpCmdLine ), desc );
    return ret;
}
