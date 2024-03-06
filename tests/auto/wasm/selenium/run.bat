::
::  The highest version of python that can be used is 3.11
:: Download from here: https://www.python.org/downloads/release/python-3118/
::
start "qtwasmserver.py" python qtwasmserver.py -p 8001
python qwasmwindow.py
taskkill /FI "WINDOWTITLE eq qtwasmserver.py"
