TEMPLATE		= app
HEADERS		= test_file.h
SOURCES		= test_file.cpp \
		  	main.cpp
RESOURCES = test.qrc
TARGET	= "simple app"
DESTDIR	= "dest dir"

target.path = $$OUT_PWD/dist
INSTALLS += target

!build_pass:msvc:CONFIG(debug, debug|release):message("check for pdb, please")
