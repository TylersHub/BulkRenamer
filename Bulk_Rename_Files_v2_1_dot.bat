@echo off
setlocal enabledelayedexpansion

REM This Program is to Bulk Rename Multiple Files that share the same name to another name that they will all share. 
REM (Compatible with files that have "." in them)
REM "old_name" is the old name being replaced and "new_name" is the new name that's replacing the old one

for %%i in ("old_name.*") do (
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
