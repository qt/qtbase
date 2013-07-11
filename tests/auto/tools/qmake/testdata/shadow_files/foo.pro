TEMPLATE=app
CONFIG -= debug_and_release_target
TARGET=foo
HEADERS=test_file.h
SOURCES=\
    test_file.cpp\
    main.cpp

target.path=dist
INSTALLS+=target

test.files=test.txt foo.bar
test.path=dist
INSTALLS+=test
