
#include "Engine/EntryPoint.h"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    using namespace zp;

    EntryPointDesc desc;
    int ret = EntryPointMain( String::As( lpCmdLine ), desc );
    return ret;
}
