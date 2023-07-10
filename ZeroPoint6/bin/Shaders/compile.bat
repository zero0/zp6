@echo off
goto :init

:help
    echo %BATCH_FILE% [-e <name>] [-h] [-d] [-?]
    echo   -e, --entry      entry point name
    echo   -h, --human      human readable
    echo   -d, --debug      enable debug symbols
    echo   -c, --clean      clean before build
    echo   -?, --help       help
    goto :end

:init
    SETLOCAL EnableDelayedExpansion
    set "Compiler=dxc"
    set "EntryPoint=main"
    set "BATCH_FILE=%~nx0"
    set "UseDebug=-g0 --undef-macro DEBUG"
    set "Clean="
    set "VkVersion=100"
    set "GlobalDefines=--define-macro SHADER_VULKAN=1 --spirv-val"
    set "HumanReadable="
    set "OutputExtension=spv"

:parse
    if "%~1"=="" goto :validate

    if "%~1"=="-e"      shift & set "EntryPoint=%~1" & shift & goto :parse
    if "%~1"=="--entry" shift & set "EntryPoint=%~1" & shift & goto :parse

    if "%~1"=="-h"      set "HumanReadable=-H" & shift & goto :parse
    if "%~1"=="--human" set "HumanReadable=-H" & shift & goto :parse

    if "%~1"=="-d"      set "UseDebug=-g --define-macro DEBUG=1" & shift & goto :parse
    if "%~1"=="--debug" set "UseDebug=-g --define-macro DEBUG=1" & shift & goto :parse

    if "%~1"=="-c"      set "Clean=yes" & shift & goto :parse
    if "%~1"=="--clean" set "Clean=yes" & shift & goto :parse

    if "%~1"=="-?"      shift & goto :help
    if "%~1"=="--help"  shift & goto :help

    shift
    goto :parse

:validate
    if "%Compiler%" == "dxc"  goto :main-dxc
    if "%Compiler%" == "glsl" goto :main-glsl
    goto :end

:main-glsl
    if defined Clean for /r %%f in (*.spv) do del "%%f"

    rem Vertex and Fragment split shaders
    for /r %%f in (*.vert;*.frag) do (
        glslangValidator %UseDebug% -e %EntryPoint% -Os %HumanReadable% -V%VkVersion% %GlobalDefines% -l -s -o "%%f.%OutputExtension%" "%%f"
    )

    rem Shader file parsing
    for /r %%f in (*.shader) do (
        glslangValidator %UseDebug% -e %EntryPoint% -Os %HumanReadable% -V%VkVersion% %GlobalDefines% -D -l -S vert --define-macro SHADER_STAGE_VERTEX=1   -o "%%~df%%~pf%%~nf.vert.%OutputExtension%" "%%f"
        glslangValidator %UseDebug% -e %EntryPoint% -Os %HumanReadable% -V%VkVersion% %GlobalDefines% -D -l -S frag --define-macro SHADER_STAGE_FRAGMENT=1 -o "%%~df%%~pf%%~nf.frag.%OutputExtension%" "%%f"
    )

    goto :end

:main-dxc
    if defined Clean for /r %%f in (*.spv) do del "%%f"

    rem Vertex and Fragment split shaders
    for /r %%f in (*.vert) do (
    rem    dxc -spirv -E %EntryPoint% -D SHADER_STAGE_VERTEX=1   -T vs_6_0 -Fo "%%f.%OutputExtension%" "%%f"
    )

    for /r %%f in (*.frag) do (
    rem    dxc -spirv -E %EntryPoint% -D SHADER_STAGE_FRAGMENT=1 -T ps_6_0 -Fo "%%f.%OutputExtension%" "%%f"
    )

    rem Shader file parsing
    for /r %%f in (*.shader) do (
        dxc -spirv -E "%%~nfVertex"   -fspv-entrypoint-name="%EntryPoint%" -I "../../Assets/ShaderLibrary/" -D SHADER_PLATFORM_D3D=1 -D SHADER_STAGE_VERTEX=1   -T vs_6_0 -Fo "%%~df%%~pf%%~nf.vert.%OutputExtension%" "%%f"
        dxc -spirv -E "%%~nfFragment" -fspv-entrypoint-name="%EntryPoint%" -I "../../Assets/ShaderLibrary/" -D SHADER_PLATFORM_D3D=1 -D SHADER_STAGE_FRAGMENT=1 -T ps_6_0 -Fo "%%~df%%~pf%%~nf.frag.%OutputExtension%" "%%f"
    )

    goto :end

:end
    ENDLOCAL
