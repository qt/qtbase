This directory contains a generator for unit tests for QtConcurrent.

The subdirectory 'generator' contains the generator. Run the file
"generate_gui.py" therein.
Python3.8 and PySide2 are required.

The generator writes on each click a testcase into the file
tst_qtconcurrentfiltermapgenerated.cpp
and
tst_qtconcurrentfiltermapgenerated.h.

Testcases which should be preserved can be copy-pasted into
tst_qtconcurrent_selected_tests.cpp.
