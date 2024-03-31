@echo off
setlocal enabledelayedexpansion

REM This Program is to Bulk Rename Multiple Files that share the same name to another name that they will all share.
REM This Program renames all files and folders in a directory and all of its subdirectories
REM "old_name" is the old name being replaced and "new_name" is the new name that's replacing the old one
REM (Compatible with files that have "." in them)

for /F "delims=" %%i in ('dir /S /B /A:-D old_name.png') do (
    set "filename=%%~ni"
    set "extension=%%~xi"
    
    for /f "tokens=1,* delims=." %%a in ("!filename!") do (
        if "!filename!" neq "!filename:.=!" (
            set "basename=new_name."
            set "rest=%%b"
        ) else (
            set "basename=new_name"
            set "rest=%%b"
        )
    )

    ren "%%i" "!basename!!rest!!extension!"
)

REM Renaming directories
for /D %%i in (old_name.png) do (
    set "dirname=%%~ni"
    
    for /f "tokens=1,* delims=." %%a in ("!dirname!") do (
        if "!dirname!" neq "!dirname:.=!" (
            set "basename=new_name."
            set "rest=%%b"
        ) else (
            set "basename=new_name"
            set "rest=%%b"
        )
    )

    ren "%%i" "!basename!!rest!"
)
