SOURCES = xcb.cpp
CONFIG -= qt

LIBS += -lxcb

!isEmpty(XCB_STATIC_LINK): LIBS += -lXau
