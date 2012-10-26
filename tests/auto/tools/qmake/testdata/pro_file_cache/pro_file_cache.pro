TEMPLATE = app
CONFIG -= debug_and_release_target

SOURCES =

content = ""
write_file("include.pri", content)
include(include.pri)

content = "SOURCES = main.cpp"
write_file("include.pri", content)
include(include.pri)

# Make sure that including the .pri file a second time will reload it properly
# off disk with the new content.
isEmpty(SOURCES): error(No sources defined)

# Empty it again to silence qmake about non-existance sources
SOURCES =
