include(rhiwindow.pri)

QT += gui-private

SOURCES += \
    main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/gui/rhiwindow
INSTALLS += target
