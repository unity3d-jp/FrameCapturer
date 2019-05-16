setlocal
cd /d "%~dp0"
call toolchain.bat

msbuild fccore.sln /t:Build /p:Configuration=Master /p:Platform=x64 /m /nologo
IF %ERRORLEVEL% NEQ 0 (
    pause
    exit /B 1
)

msbuild fccore.sln /t:Build /p:Configuration=Master /p:Platform=Win32 /m /nologo
IF %ERRORLEVEL% NEQ 0 (
    pause
    exit /B 1
)
