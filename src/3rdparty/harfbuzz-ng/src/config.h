#ifndef HB_CONFIG_H
#define HB_CONFIG_H

#define HAVE_OT

#define HB_NO_MT
#define HB_NO_UNICODE_FUNCS

#define HB_DISABLE_DEPRECATED

#include <QtCore/qglobal.h>

#ifndef HB_INTERNAL
#  define HB_INTERNAL Q_DECL_HIDDEN
#endif

#if !defined(QT_NO_DEBUG)
#  define NDEBUG
#endif

// because strdup() is not part of strict Posix, declare it here
extern "C" char *strdup(const char *src);

#ifndef HAVE_ATEXIT
#  define HAVE_ATEXIT 1
#  include <QtCore/qcoreapplication.h>
#  define atexit qAddPostRoutine
#endif

#endif /* HB_CONFIG_H */
