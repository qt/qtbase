QMAKE_SUBSTITUTES += test.in sub/test2.in indirect

indirect.input = $$PWD/test3.txt
indirect.output = $$OUT_PWD/sub/indirect_test.txt

