@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

:: Check if cl.exe is already available in PATH
where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 goto :compile

:: Locate Visual Studio via vswhere if available
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist %VSWHERE% (
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_DIR=%%i"
    )
    if exist "!VS_DIR!\VC\Auxiliary\Build\vcvarsall.bat" (
        call "!VS_DIR!\VC\Auxiliary\Build\vcvarsall.bat" x86
        goto :compile
    )
)

:: Fallback paths for VS2019 / VS2022 Community / Professional / Enterprise
set KNOWN_PATHS[0]="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
set KNOWN_PATHS[1]="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
set KNOWN_PATHS[2]="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
set KNOWN_PATHS[3]="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
set KNOWN_PATHS[4]="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
set KNOWN_PATHS[5]="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

for /L %%i in (0,1,5) do (
    set "P=!KNOWN_PATHS[%%i]!"
    if exist !P! (
        call !P! x86
        goto :compile
    )
)

echo [Error] Microsoft Visual C++ Compiler (x86) not found.
echo Please install Visual Studio with C++ tools or run from Developer Command Prompt.
exit /b 1

:compile
echo [Build] Compiling C++ source files...
cl.exe /O2 /W3 /MD /EHsc /c main.cpp blueprint.cpp overlay.cpp
if %ERRORLEVEL% NEQ 0 (
    echo [Build] Compilation failed!
    exit /b %ERRORLEVEL%
)

echo [Build] Linking shfolder.dll...
link.exe /DLL /OUT:shfolder.dll /EXPORT:SHGetFolderPathA=_SHGetFolderPathA@20 /EXPORT:SHGetFolderPathW=_SHGetFolderPathW@20 user32.lib shell32.lib gdi32.lib main.obj blueprint.obj overlay.obj
if %ERRORLEVEL% EQU 0 (
    echo [Build] Compilation and linking successful!
    if exist "..\Stronghold Crusader.exe" (
        copy /Y shfolder.dll ..\shfolder.dll >nul 2>&1
        if !ERRORLEVEL! NEQ 0 (
            del /f /q ..\shfolder.dll.old >nul 2>&1
            ren ..\shfolder.dll shfolder.dll.old >nul 2>&1
            copy /Y shfolder.dll ..\shfolder.dll >nul 2>&1
        )
        echo [Build] Copied shfolder.dll to game root folder.
    ) else (
        echo [Build] shfolder.dll is ready. Copy it to your Stronghold Crusader game directory.
    )
) else (
    echo [Build] Linking failed!
    exit /b %ERRORLEVEL%
)
