@echo off

IF NOT EXIST "external/libyuv" (
    cd external
    7z\7za.exe x -aos *.7z
)
