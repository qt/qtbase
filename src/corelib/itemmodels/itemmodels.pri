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

    qtConfig(concatenatetablesproxymodel) {
        HEADERS += \
            itemmodels/qconcatenatetablesproxymodel.h

        SOURCES += \
            itemmodels/qconcatenatetablesproxymodel.cpp
    }

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

    qtConfig(transposeproxymodel) {
        HEADERS += \
            itemmodels/qtransposeproxymodel.h \
            itemmodels/qtransposeproxymodel_p.h

        SOURCES += \
            itemmodels/qtransposeproxymodel.cpp
    }
}

qtConfig(stringlistmodel) {
    HEADERS += \
        itemmodels/qstringlistmodel.h

    SOURCES += \
        itemmodels/qstringlistmodel.cpp
}
