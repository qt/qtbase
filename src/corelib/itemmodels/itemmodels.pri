# Qt itemmodels core module

!qtConfig(itemmodel): return()

HEADERS += \
    itemmodels/qabstractitemmodel.h \
    itemmodels/qabstractitemmodel_p.h \
    itemmodels/qitemselectionmodel.h \
    itemmodels/qitemselectionmodel_p.h

SOURCES += \
    itemmodels/qabstractitemmodel.cpp \
    itemmodels/qitemselectionmodel.cpp

qtConfig(proxymodel) {
    HEADERS += \
        itemmodels/qabstractproxymodel.h \
        itemmodels/qabstractproxymodel_p.h

    SOURCES += \
        itemmodels/qabstractproxymodel.cpp

    qtConfig(identityproxymodel) {
        HEADERS += \
            itemmodels/qidentityproxymodel.h

        SOURCES += \
            itemmodels/qidentityproxymodel.cpp
    }

    qtConfig(sortfilterproxymodel) {
        HEADERS += \
            itemmodels/qsortfilterproxymodel.h

        SOURCES += \
            itemmodels/qsortfilterproxymodel.cpp
    }
}

qtConfig(stringlistmodel) {
    HEADERS += \
        itemmodels/qstringlistmodel.h

    SOURCES += \
        itemmodels/qstringlistmodel.cpp
}
