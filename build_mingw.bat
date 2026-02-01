@echo off
setlocal enabledelayedexpansion

REM Build script for MinGW toolchain (see directory_MinGW.txt).

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "TOOLCHAIN_BIN=%ROOT%\toolchain\MinGW\bin"
set "DEPS_DIR=%ROOT%\deps"
set "VULKAN_SDK=%DEPS_DIR%\vulkan"
set "GLSLC=%DEPS_DIR%\tools\bin\glslc.exe"

if not exist "%TOOLCHAIN_BIN%\g++.exe" (
  echo [error] MinGW toolchain not found at "%TOOLCHAIN_BIN%".
  exit /b 1
)

if not exist "%VULKAN_SDK%\include\vulkan\vulkan.h" (
  echo [error] Vulkan SDK headers not found at "%VULKAN_SDK%".
  exit /b 1
)

if not exist "%GLSLC%" (
  echo [error] glslc not found at "%GLSLC%".
  exit /b 1
)

set "PATH=%TOOLCHAIN_BIN%;%PATH%"
set "CC=%TOOLCHAIN_BIN%\gcc.exe"
set "CXX=%TOOLCHAIN_BIN%\g++.exe"

set "BUILD_DIR=%ROOT%\build\mingw"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH="%DEPS_DIR%" ^
  -DVulkan_ROOT="%VULKAN_SDK%" ^
  -DVulkan_GLSLC_EXECUTABLE="%GLSLC%"
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Release
endlocal
