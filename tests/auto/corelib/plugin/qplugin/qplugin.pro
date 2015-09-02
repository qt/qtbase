QT = core
TEMPLATE = subdirs
win32 {
    exists($$[QT_INSTALL_LIBS/get]/QtCore4.dll) {
        SUBDIRS = releaseplugin
    }
    exists($$[QT_INSTALL_LIBS/get]/QtCored4.dll) {
        SUBDIRS += debugplugin
    }
}
mac {
    CONFIG(debug, debug|release): {
         SUBDIRS += debugplugin 
         tst_qplugin_pro.depends += debugplugin
    }
    CONFIG(release, debug|release): {
        SUBDIRS += releaseplugin 
        tst_qplugin_pro.depends += releaseplugin
    }
}
!win32:!mac:{
    SUBDIRS = debugplugin releaseplugin
    tst_qplugin_pro.depends += debugplugin releaseplugin
} 
SUBDIRS += tst_qplugin.pro


