module;

// Replace with the commented-out block below once qobject_impl.h has been fixed.
#include <QString>

// #include <QtCore>
// #ifdef HAS_QTCONCURRENT
// #include <QtConcurrent>
// #endif
// #ifdef HAS_QTDBUS
// #include <QtDBus>
// #endif
// #ifdef HAS_QTGUI
// #include <QtGui>
// #endif
// #ifdef HAS_QTNETWORK
// #include <QtNetwork>
// #endif
// #ifdef HAS_QTOPENGL
// #include <QtOpenGL>
// #endif
// #ifdef HAS_QTOPENGLWIDGETS
// #include <QtOpenGLWidgets>
// #endif
// #ifdef HAS_QTPRINTSUPPORT
// #include <QtPrintSupport>
// #endif
// #ifdef HAS_QTSQL
// #include <QtSql>
// #endif
// #ifdef HAS_QTTEST
// #include <QtTest>
// #endif
// #ifdef HAS_QTWIDGETS
// #include <QtWidgets>
// #endif
// #ifdef HAS_QTXML
// #include <QtXml>
// #endif

export module Mod;

export QString modFn() { return QString{"ok"}; }
