@echo off
setlocal enabledelayedexpansion

REM This Program is to Bulk Rename Multiple Files that start with "old_name" to a new name with the same ending.
REM "old_name" is the beginning of the old name being replaced, and "new_name" is the new name prefix that's replacing the old one
REM This program allows the renaming of files with "old_name" as the full name and part of the name (at the beginning)

for %%i in (old_name*.*) do (
    set "filename=%%~ni"
    set "extension=%%~xi"
    
    REM Extract the part of the filename after "old_name"
    set "postfix=!filename:*old_name=!"
    
    REM Construct the new filename with "new_name" as the prefix instead of "old_name"
    set "newfilename=new_name!postfix!!extension!"
    
    ren "%%i" "!newfilename!"
)
