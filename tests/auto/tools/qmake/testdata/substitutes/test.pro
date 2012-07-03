QMAKE_SUBSTITUTES += test.in sub/test2.in indirect copy

indirect.input = $$PWD/test3.txt
indirect.output = $$OUT_PWD/sub/indirect_test.txt

copy.input = $$PWD/copy.txt
copy.output = $$OUT_PWD/copy_test.txt
copy.CONFIG = verbatim
