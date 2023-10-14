@echo off
goto :init

:help
    echo %BATCH_FILE% [-e <name>] [-h] [-d] [-?]
    echo   -e, --entry      entry point name
    echo   -h, --human      human readable
    echo   -d, --debug      enable debug symbols
    echo   -c, --clean      clean before build
    echo   -p, --prod       generate production code
    echo   -?, --help       help
    goto :end

:init
    SETLOCAL EnableDelayedExpansion
    set "Compiler=dxc"
    set "EntryPoint=main"
    set "BATCH_FILE=%~nx0"
    set "UseDebug="
    set "UseProd="
    set "Clean="
    set "VkVersion=100"
    set "GlobalDefines="
    set "HumanReadable="
    set "OutputExtension=spv"

:parse
    if "%~1"=="" goto :validate

    if "%~1"=="-e"      shift & set "EntryPoint=%~1" & shift & goto :parse
    if "%~1"=="--entry" shift & set "EntryPoint=%~1" & shift & goto :parse

    if "%~1"=="-h"      set "HumanReadable=-H" & shift & goto :parse
    if "%~1"=="--human" set "HumanReadable=-H" & shift & goto :parse

    if "%~1"=="-d"      set "UseDebug=yes" & shift & goto :parse
    if "%~1"=="--debug" set "UseDebug=yes" & shift & goto :parse

    if "%~1"=="-c"      set "Clean=yes" & shift & goto :parse
    if "%~1"=="--clean" set "Clean=yes" & shift & goto :parse

    if "%~1"=="-p"      set "UseProd=yes" & shift & goto :parse
    if "%~1"=="--prod"  set "UseProd=yes" & shift & goto :parse

    if "%~1"=="-?"      shift & goto :help
    if "%~1"=="--help"  shift & goto :help

    shift
    goto :parse

:validate
    if defined UseDebug if defined UseProd echo "Can't use debug and prod at the same time" & goto :end

    if "%Compiler%" == "dxc"  goto :main-dxc
    if "%Compiler%" == "glsl" goto :main-glsl
    goto :end

:main-glsl
    if defined Clean for /r %%f in (*.spv) do del "%%f"

    set "Debug=-g0 --undef-macro DEBUG"
    if defined UseDebug set "UseDebug=-g --define-macro DEBUG=1"

    rem Vertex and Fragment split shaders
    for /r %%f in (*.vert;*.frag) do (
        glslangValidator %Debug% -e %EntryPoint% -Os %HumanReadable% -V%VkVersion% %GlobalDefines% -l -s -o "%%f.%OutputExtension%" "%%f"
    )

    rem Shader file parsing
    for /r %%f in (*.shader) do (
        glslangValidator %Debug% -e %EntryPoint% -Os %HumanReadable% -V%VkVersion% %GlobalDefines% -D -l -S vert --define-macro SHADER_STAGE_VERTEX=1   -o "%%~df%%~pf%%~nf.vert.%OutputExtension%" "%%f"
        glslangValidator %Debug% -e %EntryPoint% -Os %HumanReadable% -V%VkVersion% %GlobalDefines% -D -l -S frag --define-macro SHADER_STAGE_FRAGMENT=1 -o "%%~df%%~pf%%~nf.frag.%OutputExtension%" "%%f"
    )

    goto :end

:main-dxc
    if defined Clean for /r %%f in (./bin/Shaders/*.spv) do del "%%f"

    set "TargetProfile=6_0"

    pushd .\Assets\Shaders\

    set "Debug="
    if defined UseDebug set "UseDebug=-Zi -Od -D DEBUG=1"

    rem Vertex and Fragment split shaders
    for /r %%f in (*.vert) do (
    rem    dxc -spirv -E %EntryPoint% -D SHADER_STAGE_VERTEX=1   -T vs_6_0 -Fo "%%f.%OutputExtension%" "%%f"
    )

    for /r %%f in (*.frag) do (
    rem    dxc -spirv -E %EntryPoint% -D SHADER_STAGE_FRAGMENT=1 -T ps_6_0 -Fo "%%f.%OutputExtension%" "%%f"
    )

    rem Shader file parsing
    for /r %%f in (*.shader) do (
        dxc -spirv %Debug% -E "%%~nfVertex"   -fspv-entrypoint-name="%EntryPoint%" -I "../ShaderLibrary/" -D SHADER_API_D3D=1 -D SHADER_STAGE_VERTEX=1   -T vs_%TargetProfile% -Fo "%%~dpf..\..\bin\Shaders\%%~nf.vert.%OutputExtension%" "%%f"
        dxc -spirv %Debug% -E "%%~nfFragment" -fspv-entrypoint-name="%EntryPoint%" -I "../ShaderLibrary/" -D SHADER_API_D3D=1 -D SHADER_STAGE_FRAGMENT=1 -T ps_%TargetProfile% -Fo "%%~dpf..\..\bin\Shaders\%%~nf.frag.%OutputExtension%" "%%f"
    )

    popd

    goto :end

:end
    ENDLOCAL
