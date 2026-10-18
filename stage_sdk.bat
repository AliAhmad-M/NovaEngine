@echo off
setlocal

set CONFIG=%1
set SDK=%~2
set ROOT=%~dp0

if "%CONFIG%"=="" set CONFIG=Release
if "%SDK%"=="" set SDK=%ROOT%Editor\bin\x64\%CONFIG%\Output\sdk

echo ============================================
echo [stage_sdk] Config : %CONFIG%
echo [stage_sdk] Root   : %ROOT%
echo [stage_sdk] Target : %SDK%
echo ============================================

mkdir "%SDK%\include" 2>nul
mkdir "%SDK%\lib\%CONFIG%" 2>nul

rem ---- Nova's own modules ----
for %%M in (Core Platform Graphics Physics World Runtime) do (
    echo [stage_sdk] %%M: copying headers...
    robocopy "%ROOT%%%M\include\%%M" "%SDK%\include\%%M" /E /NFL /NDL /NJH /NJS >nul
    if errorlevel 8 echo [stage_sdk] WARNING: header copy failed for %%M

    if exist "%ROOT%%%M\bin\x64\%CONFIG%\Output\%%M.lib" (
        echo [stage_sdk] %%M: copying %%M.lib
        copy /Y "%ROOT%%%M\bin\x64\%CONFIG%\Output\%%M.lib" "%SDK%\lib\%CONFIG%\" >nul
    ) else (
        echo [stage_sdk] WARNING: %%M.lib not found, has %%M been built for %CONFIG%?
    )
)

rem ---- vcpkg packages ----
echo [stage_sdk] Copying vcpkg_installed...
robocopy "%ROOT%vcpkg_installed" "%SDK%\vcpkg_installed" /E /NFL /NDL /NJH /NJS >nul
if errorlevel 8 echo [stage_sdk] WARNING: vcpkg_installed copy failed

rem ---- CMake config ----
echo [stage_sdk] Copying NovaConfig.cmake...
copy /Y "%ROOT%NovaConfig.cmake" "%SDK%\NovaConfig.cmake" >nul

echo [stage_sdk] Done.
exit /b 0