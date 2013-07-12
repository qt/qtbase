TEMPLATE=app
TARGET=foo
CONFIG -= debug_and_release_target

HEADERS=test_file.h
SOURCES=\
    test_file.cpp\
    main.cpp

test1.files=test1
test1.path=dist
INSTALLS+=test1

test2.files=test2
test2.path=dist
INSTALLS+=test2

target.path=dist
target.depends=install_test1 install_test2
INSTALLS+=target
