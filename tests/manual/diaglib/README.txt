This is a collection of functions and classes helpful for diagnosing bugs
in Qt 4 and Qt 5. It can be included in the application's .pro file by
adding:

include([path to Qt sources]/tests/manual/diaglib/diaglib.pri)

For Qt 4, the environment variable QTDIR may be used:
include($$(QTDIR)/tests/manual/diaglib/diaglib.pri)

The .pri file adds the define QT_DIAG_LIB, so, diagnostic
code can be enlosed within #ifdef to work without it as well.

All functions and classes are in the QtDiag namespace.

function dumpText() (textdump.h)
  Returns a string containing the input text split up in characters
  listing category, script, direction etc.
  Useful for analyzing non-Latin text.

function  dumpTextAsCode() (textdump.h)
   Returns a string containing a code snippet creating a QString
   by appending the unicode value of character of the input.
   This is useful for constructing non-Latin strings with purely ASCII
   source code.

class EventFilter (eventfilter.h):
  An event filter that logs Qt events to qDebug() depending on
  configured categories (for example mouse, keyboard, etc).

class DebugProxyStyle (debugproxystyle.h)
  A proxy style that wraps around an existing style and outputs
  the parameters and return values of its function calls, useful
  for debugging QStyle.

class LogWidget (logwidget.h)
  A Log Widget inheriting QPlainTextEdit with convenience functions
  to install as a message handler.

class LogFunctionGuard
   A guard class for use with LogWidget logging messages when a function
   is entered/left (indenting the log messages).
   Can be instantiated using:
   LogFunctionGuard guard(__FUNCTION__) or
   LogFunctionGuard guard(Q_FUNC_INFO)

function glInfo() (glinfo.h):
  Returns a string describing the Open GL configuration (obtained
  by querying GL_VENDOR and GL_RENDERER). Available only
  when the QT qmake variable contains opengl.

functions dumpNativeWindows(), dumpNativeQtTopLevels():
  These functions du,p out the hierarchy of native Windows. Currently
  implemented for Windows only.

function dumpAllWidgets() (qwidgetdump.h):
  Dumps the hierarchy of QWidgets including information about flags,
  visibility, geometry, etc.

function dumpAllWindows() (qwindowdump.h):
  Dumps the hierarchy of QWindows including information about flags,
  visibility, geometry, etc.
