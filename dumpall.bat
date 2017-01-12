:: Dumps all CGO and DGOs from Jak & Daxter 1 out into assembly.

@echo off
setlocal
if "%1"=="" goto usage
if "%2"=="" goto usage
set OUT=%1
set IN=%2
if not exist "%IN%\SYSTEM.CNF" goto missing
if not exist %OUT%\cgo mkdir %OUT%\cgo
if not exist %OUT%\dgo mkdir %OUT%\dgo

echo Disassembling '%IN%' into '%OUT%'...

for %%a in (%IN%\CGO\*.cgo) do (
	goaldis -asm %OUT%\CGO\%%~na %%a
)

for %%a in (%IN%\DGO\*.dgo) do (
	goaldis -asm %OUT%\DGO\%%~na %%a
)

exit /b 0

:usage
echo Usage: dumpall outdir isodir
echo e.g.   dumpall myasm C:\jakiso\
exit /b 1

:missing
echo Invalid ISO: cannot find %IN%\SYSTEM.CNF
exit /b 1
