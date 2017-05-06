call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
msbuild fccore.sln /t:Build /p:Configuration=Master /p:Platform=x64 /m /nologo
msbuild fccore.sln /t:Build /p:Configuration=Master /p:Platform=Win32 /m /nologo
