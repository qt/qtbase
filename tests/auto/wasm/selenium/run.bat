:: Copyright (C) 2024 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
::
::  The highest version of python that can be used is 3.11
:: Download from here: https://www.python.org/downloads/release/python-3118/
::
start "qtwasmserver.py" python qtwasmserver.py --cross-origin-isolation -p 8001
python qwasmwindow.py
taskkill /FI "WINDOWTITLE eq qtwasmserver.py"
