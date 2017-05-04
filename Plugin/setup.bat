@echo off

IF NOT EXIST "external/ispc.exe" (
    cd external
    7z\7za.exe x -aos *.7z
)
