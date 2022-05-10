:: Copyright (C) 2019 Intel Corporation.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
