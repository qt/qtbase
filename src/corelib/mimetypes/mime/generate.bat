:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Copyright (C) 2019 Intel Corporation.
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
set me=%~dp0

:: Check if certain tools are in PATH
for %%C in (gzip.exe zstd.exe perl.exe) do set %%C=%%~$PATH:C

:: If perl is in PATH, just let it do everything
if not "%perl.exe%" == "" goto PuntToPerl

set COMPRESS=
set MACRO=MIME_DATABASE_IS_UNCOMPRESSED
if not "%gzip.exe%" == "" (
    set COMPRESS=gzip -9
    set MACRO=MIME_DATABASE_IS_GZIP
)

:: Check if zstd support was enabled
if /i "%~1" == "--zstd" (
    shift
    if not "%zstd.exe%" == "" (
        set COMPRESS=zstd -19
        set MACRO=MIME_DATABASE_IS_ZSTD
    )
)

if not "%COMPRESS%" == "" goto CompressedCommon

:: No Compression and no Perl
:: Just hex-dump with Powershell
powershell -ExecutionPolicy Bypass %me%hexdump.ps1 %1 %1
exit /b %errorlevel%

:CompressedCommon
:: Compress to a temporary file, then hex-dump using Powershell
echo #define %MACRO%
set tempfile=generate%RANDOM%.tmp
%COMPRESS% < %1 > %tempfile%
powershell -ExecutionPolicy Bypass %me%hexdump.ps1 %tempfile% %1
del %tempfile%
exit /b %errorlevel%

:PuntToPerl
perl %me%generate.pl %*
exit /b %errorlevel%
