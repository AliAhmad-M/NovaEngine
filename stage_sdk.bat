@echo off
setlocal

set CONFIG=%1
set SDK=%~2
set ROOT=%~dp0

if "%CONFIG%"=="" set CONFIG=Release
if "%SDK%"=="" (
    echo Usage: stage_sdk.bat Release "path\to\folder\containing\Editor.exe\sdk"
    exit /b 1
)

mkdir "%SDK%\include" 2>nul
mkdir "%SDK%\lib\%CONFIG%" 2>nul

rem ---- Nova's own modules ----
for %%M in (Core Platform Graphics Physics World Runtime) do (
    robocopy "%ROOT%%%M\include\%%M" "%SDK%\include\%%M" /E /NFL /NDL /NJH /NJS >nul
    copy /Y "%ROOT%%%M\bin\x64\%CONFIG%\Output\%%M.lib" "%SDK%\lib\%CONFIG%\" >nul
)

rem ---- vcpkg packages (the folder vcpkg created next to your vcpkg.json) ----
robocopy "%ROOT%vcpkg_installed" "%SDK%\vcpkg_installed" /E /NFL /NDL /NJH /NJS >nul

copy /Y "%ROOT%NovaConfig.cmake" "%SDK%\NovaConfig.cmake" >nul

exit /b 0