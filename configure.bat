:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Copyright (C) 2016 The Qt Company Ltd.
:: Copyright (C) 2016 Intel Corporation.
:: Contact: https://www.qt.io/licensing/
::
:: This file is part of the tools applications of the Qt Toolkit.
::
:: $QT_BEGIN_LICENSE:GPL-EXCEPT$
:: Commercial License Usage
:: Licensees holding valid commercial Qt licenses may use this file in
:: accordance with the commercial license agreement provided with the
:: Software or, alternatively, in accordance with the terms contained in
:: a written agreement between you and The Qt Company. For licensing terms
:: and conditions see https://www.qt.io/terms-conditions. For further
:: information use the contact form at https://www.qt.io/contact-us.
::
:: GNU General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU
:: General Public License version 3 as published by the Free Software
:: Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
:: included in the packaging of this file. Please review the following
:: information to ensure the GNU General Public License requirements will
:: be met: https://www.gnu.org/licenses/gpl-3.0.html.
::
:: $QT_END_LICENSE$
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

@echo off
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
set ARGS=%*
set QTSRC=%~dp0
set QTSRC=%QTSRC:~0,-1%
set QTDIR=%CD%

rem Make sure qmake is not confused by these. Recursion via Makefiles would
rem be still affected, so just unsetting them here is not an option.
if not "%QMAKESPEC%" == "" goto envfail
if not "%XQMAKESPEC%" == "" goto envfail
if not "%QMAKEPATH%" == "" goto envfail
if not "%QMAKEFEATURES%" == "" goto envfail
goto envok
:envfail
echo >&2 Please make sure to unset the QMAKESPEC, XQMAKESPEC, QMAKEPATH,
echo >&2 and QMAKEFEATURES environment variables prior to building Qt.
exit /b 1
:envok

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

set SYNCQT=
set PLATFORM=
set MAKE=
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

    if /i "%~1" == "-platform" goto platform
    if /i "%~1" == "--platform" goto platform

    if /i "%~1" == "-no-syncqt" goto nosyncqt
    if /i "%~1" == "--no-syncqt" goto nosyncqt

    if /i "%~1" == "-make-tool" goto maketool
    if /i "%~1" == "--make-tool" goto maketool

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
    set rargs=
    for /f "usebackq delims=" %%i in ("%TOPQTDIR%\config.opt") do set rargs=!rargs! "%%i"
    call :doargs %rargs%
    goto nextarg
:redoerr
    echo No config.opt present - cannot redo configuration. >&2
    exit /b 1

:platform
    shift
    if "%~1" == "win32-msvc2012" goto msvc
    if "%~1" == "win32-msvc2013" goto msvc
    if "%~1" == "win32-msvc2015" goto msvc
    if "%~1" == "win32-msvc2017" goto msvc
    set PLATFORM=%~1
    goto nextarg
:msvc
    echo. >&2
    echo Notice: re-mapping requested qmake spec to unified 'win32-msvc'. >&2
    echo. >&2
    set PLATFORM=win32-msvc
    goto nextarg

:nosyncqt
    set SYNCQT=false
    goto nextarg

:maketool
    shift
    set MAKE=%~1
    goto nextarg

:doneargs

rem Find various executables
for %%C in (clang-cl.exe cl.exe icl.exe g++.exe perl.exe jom.exe) do set %%C=%%~$PATH:C

rem Determine host spec

if "%PLATFORM%" == "" (
    if not "%icl.exe%" == "" (
        set PLATFORM=win32-icc
    ) else if not "%cl.exe%" == "" (
        set PLATFORM=win32-msvc
    ) else if not "%clang-cl.exe%" == "" (
        set PLATFORM=win32-clang-msvc
    ) else if not "%g++.exe%" == "" (
        set PLATFORM=win32-g++
    ) else (
        echo Cannot detect host toolchain. Please use -platform. Aborting. >&2
        exit /b 1
    )
)
if not exist "%QTSRC%\mkspecs\%PLATFORM%\qmake.conf" (
    echo Host platform '%PLATFORM%' is invalid. Aborting. >&2
    exit /b 1
)
if "%PLATFORM:win32-g++=%" == "%PLATFORM%" (
    if "%MAKE%" == "" (
        if not "%jom.exe%" == "" (
            set MAKE=jom
        ) else (
            set MAKE=nmake
        )
    )
    set tmpl=win32
) else (
    if "%MAKE%" == "" (
        set MAKE=mingw32-make
    )
    set tmpl=unix
)

rem Prepare build dir

if not exist mkspecs (
    md mkspecs
    if errorlevel 1 exit /b
)
if not exist bin (
    md bin
    if errorlevel 1 exit /b
)
if not exist qmake (
    md qmake
    if errorlevel 1 exit /b
)

rem Extract Qt's version from .qmake.conf
for /f "eol=# tokens=1,2,3,4 delims=.= " %%i in (%QTSRC%\.qmake.conf) do (
    if %%i == MODULE_VERSION (
        set QTVERMAJ=%%j
        set QTVERMIN=%%k
        set QTVERPAT=%%l
    )
)
set QTVERSION=%QTVERMAJ%.%QTVERMIN%.%QTVERPAT%

rem Create forwarding headers

if "%SYNCQT%" == "" (
    if exist "%QTSRC%\.git" (
        set SYNCQT=true
    ) else (
        set SYNCQT=false
    )
)
if "%SYNCQT%" == "true" (
    if not "%perl.exe%" == "" (
        echo Running syncqt ...
        "%perl.exe%" -w "%QTSRC%\bin\syncqt.pl" -minimal -version %QTVERSION% -module QtCore -outdir "%QTDIR%" %QTSRC%
        if errorlevel 1 exit /b
    ) else (
        echo Perl not found in PATH. Aborting. >&2
        exit /b 1
    )
)

rem Build qmake

echo Bootstrapping qmake ...

cd qmake
if errorlevel 1 exit /b

echo #### Generated by configure.bat - DO NOT EDIT! ####> Makefile
echo/>> Makefile
echo BUILD_PATH = ..>> Makefile
if "%tmpl%" == "win32" (
    echo SOURCE_PATH = %QTSRC%>> Makefile
) else (
    echo SOURCE_PATH = %QTSRC:\=/%>> Makefile
)
if exist "%QTSRC%\.git" (
    echo INC_PATH = ../include>> Makefile
) else (
    echo INC_PATH = $^(SOURCE_PATH^)/include>> Makefile
)
echo QT_VERSION = %QTVERSION%>> Makefile
rem These must have trailing spaces to avoid misinterpretation as 5>>, etc.
echo QT_MAJOR_VERSION = %QTVERMAJ% >> Makefile
echo QT_MINOR_VERSION = %QTVERMIN% >> Makefile
echo QT_PATCH_VERSION = %QTVERPAT% >> Makefile
if "%tmpl%" == "win32" (
    echo QMAKESPEC = %PLATFORM%>> Makefile
) else (
    echo QMAKESPEC = $^(SOURCE_PATH^)/mkspecs/%PLATFORM%>> Makefile
    echo CONFIG_CXXFLAGS = -std=c++11 -ffunction-sections>> Makefile
    echo CONFIG_LFLAGS = -Wl,--gc-sections>> Makefile
    type "%QTSRC%\qmake\Makefile.unix.win32" >> Makefile
    type "%QTSRC%\qmake\Makefile.unix.mingw" >> Makefile
)
echo/>> Makefile
type "%QTSRC%\qmake\Makefile.%tmpl%" >> Makefile

%MAKE%
if errorlevel 1 (cd .. & exit /b 1)

cd ..

rem Generate qt.conf

> "%QTDIR%\bin\qt.conf" (
    @echo [EffectivePaths]
    @echo Prefix=..
    @echo [Paths]
    @echo TargetSpec=dummy
    @echo HostSpec=%PLATFORM%
)
if not "%QTDIR%" == "%QTSRC%" (
    >> "%QTDIR%\bin\qt.conf" (
        @echo [EffectiveSourcePaths]
        @echo Prefix=%QTSRC:\=/%
    )
)

rem Launch qmake-based configure

cd "%TOPQTDIR%"
"%QTDIR%\bin\qmake.exe" "%TOPQTSRC%" -- %ARGS%
