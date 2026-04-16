@echo off
setlocal

set GLSLC=glslc
where %GLSLC% >nul 2>nul
if %errorlevel% neq 0 (
	if defined VULKAN_SDK (
		set GLSLC=%VULKAN_SDK%\Bin\glslc.exe
	)
)

"%GLSLC%" simple_shader.vert -o simple_shader.vert.spv
if %errorlevel% neq 0 exit /b %errorlevel%

"%GLSLC%" simple_shader.frag -o simple_shader.frag.spv
exit /b %errorlevel%