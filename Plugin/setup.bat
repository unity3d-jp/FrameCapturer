@echo off

IF NOT EXIST "external/ispc.exe" (
    cd external
    7z\7za.exe x -aos *.7z
)

IF NOT EXIST "external\ispc.exe" (
    IF NOT EXIST "external\external.7z" (
        echo "downloading external libararies..."
        powershell.exe -Command "(new-object System.Net.WebClient).DownloadFile('https://github.com/unity3d-jp/FrameCapturer/releases/download/20170508/external.7z', 'external/external.7z')"
    )
    cd external
    7z\7za.exe x -aos external.7z
    cd ..
)
