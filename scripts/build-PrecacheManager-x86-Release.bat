@echo off
setlocal
set "Configuration=Release"
call "%~dp0build-PrecacheManager-x86.bat" %*
exit /b %errorlevel%
