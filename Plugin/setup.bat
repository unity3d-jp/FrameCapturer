@echo off

IF NOT EXIST "external\ispc.exe" (
    IF NOT EXIST "external\external.7z" (
        echo "downloading external libararies..."
        powershell.exe -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol=[System.Net.SecurityProtocolType]::Tls12; wget https://github.com/unity3d-jp/FrameCapturer/releases/download/20170510/external.7z -OutFile external/external.7z"
    )
    cd external
    7z\7za.exe x -aos external.7z
    cd ..
)
