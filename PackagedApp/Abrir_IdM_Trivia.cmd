@echo off
setlocal
set "APP=%~dp0NewApp\IdM_Trivia2.html"
start "IdM Trivia" msedge.exe "%APP%"
if errorlevel 1 start "IdM Trivia" "%APP%"
endlocal
