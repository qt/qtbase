qtConfig(standarditemmodel) {
    HEADERS += \
        itemmodels/qstandarditemmodel.h \
        itemmodels/qstandarditemmodel_p.h \

    SOURCES += \
        itemmodels/qstandarditemmodel.cpp \
}

qtConfig(filesystemmodel) {
    HEADERS += \
        itemmodels/qfilesystemmodel.h \
        itemmodels/qfilesystemmodel_p.h \
        itemmodels/qfileinfogatherer_p.h

    SOURCES += \
        itemmodels/qfilesystemmodel.cpp \
        itemmodels/qfileinfogatherer.cpp
}
