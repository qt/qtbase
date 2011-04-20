include(../qpluginbase.pri)
QT  = core sql-private
DESTDIR = $$QT.sql.plugins/sqldrivers

target.path     += $$[QT_INSTALL_PLUGINS]/sqldrivers
INSTALLS        += target

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
