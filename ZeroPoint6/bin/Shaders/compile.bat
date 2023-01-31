@echo off
goto :init

:help
    echo %BATCH_FILE% [-d] [-?]
    echo   -d, --debug      enable debug symbols
    echo   -c, --clean      clean before build
    echo   -?, --help       help
    goto :end

:init
    SETLOCAL
    set "BATCH_FILE=%~nx0"
    set "UseDebug=-g0"
    set "Clean="

:parse
    if "%~1"=="" goto :validate

    if "%~1"=="-d"      set "UseDebug=-g" & shift & goto :parse
    if "%~1"=="--debug" set "UseDebug=-g" & shift & goto :parse

    if "%~1"=="-c"      set "Clean=yes" & shift & goto :parse
    if "%~1"=="--clean" set "Clean=yes" & shift & goto :parse

    if "%~1"=="-?"      shift & goto :help
    if "%~1"=="--help"  shift & goto :help

    shift
    goto :parse

:validate

:main
    if defined Clean for /r %%f in (*.spv) do del "%%f"

    for /r %%f in (*.vert;*.frag;*.hlsl) do glslangValidator %UseDebug% -Os -V -o "%%f.spv" "%%f"

:end
    ENDLOCAL
