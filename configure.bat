:: Copyright (C) 2016 The Qt Company Ltd.
:: Copyright (C) 2016 Intel Corporation.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

@echo off
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
set ARGS=%*
set QTSRC=%~dp0
set QTSRC=%QTSRC:~0,-1%
set QTDIR=%CD%

rem Parse command line

set TOPLEVEL=false
set TOPQTSRC=%QTSRC%
set TOPQTDIR=%QTDIR%
if /i not "%~1" == "-top-level" goto notoplevel
set ARGS=%ARGS:~10%
set TOPLEVEL=true
for %%P in ("%TOPQTSRC%") do set TOPQTSRC=%%~dpP
set TOPQTSRC=%TOPQTSRC:~0,-1%
for %%P in ("%QTDIR%") do set TOPQTDIR=%%~dpP
set TOPQTDIR=%TOPQTDIR:~0,-1%
goto wastoplevel
:notoplevel
if not exist ..\.qmake.super goto wastoplevel
echo ERROR: You cannot configure qtbase separately within a top-level build. >&2
exit /b 1
:wastoplevel

call :doargs %ARGS%
if errorlevel 1 exit /b
goto doneargs

:doargs
    if "%~1" == "" exit /b

    if "%~1" == "/?" goto help
    if "%~1" == "-?" goto help
    if /i "%~1" == "/h" goto help
    if /i "%~1" == "-h" goto help
    if /i "%~1" == "/help" goto help
    if /i "%~1" == "-help" goto help
    if /i "%~1" == "--help" goto help

    if /i "%~1" == "-redo" goto redo
    if /i "%~1" == "--redo" goto redo

:nextarg
    shift
    goto doargs

:help
    type %QTSRC%\config_help.txt
    if %TOPLEVEL% == false exit /b 1
    for /d %%p in ("%TOPQTSRC%"\qt*) do (
        if not "%%p" == "%QTSRC%" (
            if exist "%%p\config_help.txt" (
                echo.
                type "%%p\config_help.txt"
            )
        )
    )
    exit /b 1

:redo
    if not exist "%TOPQTDIR%\config.opt" goto redoerr
    echo %ARGS% > %TOPQTDIR%\config.redo.in
    set redoing=""
    goto nextarg
:redoerr
    echo No config.opt present - cannot redo configuration. >&2
    exit /b 1

:doneargs

cd "%TOPQTDIR%"

rem Write config.opt if we're not currently -redo'ing
set OPT_FILE_PATH=%TOPQTDIR%\config.opt
set OPT_TMP_FILE_PATH=%TOPQTDIR%\config.opt.in
set REDO_FILE_PATH=%TOPQTDIR%\config.redo.last
set REDO_TMP_FILE_PATH=%TOPQTDIR%\config.redo.in
set FRESH_REQUESTED_ARG=
if not defined redoing (
    echo.%*>"%OPT_TMP_FILE_PATH%"

    cmake -DIN_FILE="%OPT_TMP_FILE_PATH%" -DOUT_FILE="%OPT_FILE_PATH%" -DIGNORE_ARGS=-top-level -P "%QTSRC%\cmake\QtWriteArgsFile.cmake"
) else (
    echo. 2> "%OPT_TMP_FILE_PATH%"
    for /F "usebackq tokens=*" %%A in ("%OPT_FILE_PATH%") do echo "%%A" >> "%OPT_TMP_FILE_PATH%"

    cmake -DIN_FILE="%OPT_TMP_FILE_PATH%" -DREDO_FILE="%REDO_TMP_FILE_PATH%" -DOUT_FILE="%REDO_FILE_PATH%" -DIGNORE_ARGS="-top-level;-redo;--redo" -P "%QTSRC%\cmake\QtWriteArgsFile.cmake"

    set OPT_FILE_PATH=%REDO_FILE_PATH%
    set FRESH_REQUESTED_ARG=-DFRESH_REQUESTED=TRUE
)

rem Launch CMake-based configure
set TOP_LEVEL_ARG=
if %TOPLEVEL% == true set TOP_LEVEL_ARG=-DTOP_LEVEL=TRUE
cmake -DOPTFILE="%OPT_FILE_PATH%" %TOP_LEVEL_ARG% %FRESH_REQUESTED_ARG% -P "%QTSRC%\cmake\QtProcessConfigureArgs.cmake"
