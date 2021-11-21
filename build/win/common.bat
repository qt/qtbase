:: MIT License
::
:: Copyright (C) 2021 by wangwenx190 (Yuhang Zhao)
::
:: Permission is hereby granted, free of charge, to any person obtaining a copy
:: of this software and associated documentation files (the "Software"), to deal
:: in the Software without restriction, including without limitation the rights
:: to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
:: copies of the Software, and to permit persons to whom the Software is
:: furnished to do so, subject to the following conditions:
::
:: The above copyright notice and this permission notice shall be included in
:: all copies or substantial portions of the Software.
::
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
:: IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
:: FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
:: AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
:: LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
:: OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
:: SOFTWARE.

@echo off
title Building Qt ...
setlocal enabledelayedexpansion
set __repo_root_dir=%~dp0..\..
set __qt_source_dir=%__repo_root_dir%
set __qt_build_dir=%__repo_root_dir%\..\__qt_build_cache_dir__
set __qt_install_dir=%__repo_root_dir%\..\Qt
set __cmake_config_params=%* -DCMAKE_CONFIGURATION_TYPES=Release;Debug -DCMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE=ON -DCMAKE_INSTALL_PREFIX="%__qt_install_dir%" -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF -DFEATURE_relocatable=ON -DFEATURE_system_zlib=OFF -G "Ninja Multi-Config" "%__qt_source_dir%"
set __cmake_build_params=--build "%__qt_build_dir%" --parallel
set __cmake_install_params=--install "%__qt_build_dir%"
set __vs_install_dir=%ProgramFiles%\Microsoft Visual Studio\2022\Community
for /f "delims=" %%a in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -property installationPath -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64') do set __vs_install_dir=%%a
call "%__vs_install_dir%\VC\Auxiliary\Build\vcvars64.bat"
cd /d "%~dp0"
cmake --version
echo Ninja build version:
ninja --version
if exist "%__qt_install_dir%" rd /s /q "%__qt_install_dir%"
if exist "%__qt_build_dir%" rd /s /q "%__qt_build_dir%"
md "%__qt_build_dir%" && cd "%__qt_build_dir%"
cmake %__cmake_config_params%
cmake %__cmake_build_params%
::cmake %__cmake_install_params%
ninja install
rd /s /q "%__qt_build_dir%"
:: TODO: Compress the installed artifacts into a 7-Zip package.
endlocal
cd /d "%~dp0"
pause
exit /b
