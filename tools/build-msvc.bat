@echo off
setlocal

set "VS_ROOT=C:\Program Files\Microsoft Visual Studio\18\Community"
set "VS_CMAKE=%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "VS_CTEST=%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"

call "%VS_ROOT%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

"%VS_CMAKE%" --preset msvc-debug --fresh
if errorlevel 1 exit /b %errorlevel%

"%VS_CMAKE%" --build --preset msvc-debug
if errorlevel 1 exit /b %errorlevel%

"%VS_CTEST%" --preset msvc-debug
if not errorlevel 1 exit /b 0

echo CTest metadata could not be read; running the single test binary directly.
"%TEMP%\cad-class-design\msvc-debug\cadstudy_tests.exe"
exit /b %errorlevel%
