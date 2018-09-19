/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTEST_H
#define QTEST_H

#include <QtTest/qttestglobal.h>
#include <QtTest/qtestcase.h>
#include <QtTest/qtestdata.h>
#include <QtTest/qbenchmark.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qurl.h>
#include <QtCore/quuid.h>

#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE


namespace QTest
{

template <> inline char *toString(const QStringView &str)
{
    return QTest::toPrettyUnicode(str);
}

template<> inline char *toString(const QString &str)
{
    return toString(QStringView(str));
}

template<> inline char *toString(const QLatin1String &str)
{
    return toString(QString(str));
}

template<> inline char *toString(const QByteArray &ba)
{
    return QTest::toPrettyCString(ba.constData(), ba.length());
}

#if QT_CONFIG(datestring)
template<> inline char *toString(const QTime &time)
{
    return time.isValid()
        ? qstrdup(qPrintable(time.toString(QStringViewLiteral("hh:mm:ss.zzz"))))
        : qstrdup("Invalid QTime");
}

template<> inline char *toString(const QDate &date)
{
    return date.isValid()
        ? qstrdup(qPrintable(date.toString(QStringViewLiteral("yyyy/MM/dd"))))
        : qstrdup("Invalid QDate");
}

template<> inline char *toString(const QDateTime &dateTime)
{
    return dateTime.isValid()
        ? qstrdup(qPrintable(dateTime.toString(QStringViewLiteral("yyyy/MM/dd hh:mm:ss.zzz[t]"))))
        : qstrdup("Invalid QDateTime");
}
#endif // datestring

template<> inline char *toString(const QChar &c)
{
    const ushort uc = c.unicode();
    if (uc < 128) {
        char msg[32] = {'\0'};
        qsnprintf(msg, sizeof(msg), "QChar: '%c' (0x%x)", char(uc), unsigned(uc));
        return qstrdup(msg);
    }
    return qstrdup(qPrintable(QString::fromLatin1("QChar: '%1' (0x%2)").arg(c).arg(QString::number(static_cast<int>(c.unicode()), 16))));
}

template<> inline char *toString(const QPoint &p)
{
    char msg[128] = {'\0'};
    qsnprintf(msg, sizeof(msg), "QPoint(%d,%d)", p.x(), p.y());
    return qstrdup(msg);
}

template<> inline char *toString(const QSize &s)
{
    char msg[128] = {'\0'};
    qsnprintf(msg, sizeof(msg), "QSize(%dx%d)", s.width(), s.height());
    return qstrdup(msg);
}

template<> inline char *toString(const QRect &s)
{
    char msg[256] = {'\0'};
    qsnprintf(msg, sizeof(msg), "QRect(%d,%d %dx%d) (bottomright %d,%d)",
              s.left(), s.top(), s.width(), s.height(), s.right(), s.bottom());
    return qstrdup(msg);
}

template<> inline char *toString(const QPointF &p)
{
    char msg[64] = {'\0'};
    qsnprintf(msg, sizeof(msg), "QPointF(%g,%g)", p.x(), p.y());
    return qstrdup(msg);
}

template<> inline char *toString(const QSizeF &s)
{
    char msg[64] = {'\0'};
    qsnprintf(msg, sizeof(msg), "QSizeF(%gx%g)", s.width(), s.height());
    return qstrdup(msg);
}

template<> inline char *toString(const QRectF &s)
{
    char msg[256] = {'\0'};
    qsnprintf(msg, sizeof(msg), "QRectF(%g,%g %gx%g) (bottomright %g,%g)",
              s.left(), s.top(), s.width(), s.height(), s.right(), s.bottom());
    return qstrdup(msg);
}

template<> inline char *toString(const QUrl &uri)
{
    if (!uri.isValid())
        return qstrdup(qPrintable(QLatin1String("Invalid URL: ") + uri.errorString()));
    return qstrdup(uri.toEncoded().constData());
}

template <> inline char *toString(const QUuid &uuid)
{
    return qstrdup(uuid.toByteArray().constData());
}

template<> inline char *toString(const QVariant &v)
{
    QByteArray vstring("QVariant(");
    if (v.isValid()) {
        QByteArray type(v.typeName());
        if (type.isEmpty()) {
            type = QByteArray::number(v.userType());
        }
        vstring.append(type);
        if (!v.isNull()) {
            vstring.append(',');
            if (v.canConvert(QVariant::String)) {
                vstring.append(v.toString().toLocal8Bit());
            }
            else {
                vstring.append("<value not representable as string>");
            }
        }
    }
    vstring.append(')');

    return qstrdup(vstring.constData());
}

template <typename T1, typename T2>
inline char *toString(const QPair<T1, T2> &pair)
{
    const QScopedArrayPointer<char> first(toString(pair.first));
    const QScopedArrayPointer<char> second(toString(pair.second));
    return toString(QString::asprintf("QPair(%s,%s)", first.data(), second.data()));
}

template <typename T1, typename T2>
inline char *toString(const std::pair<T1, T2> &pair)
{
    const QScopedArrayPointer<char> first(toString(pair.first));
    const QScopedArrayPointer<char> second(toString(pair.second));
    return toString(QString::asprintf("std::pair(%s,%s)", first.data(), second.data()));
}

inline char *toString(std::nullptr_t)
{
    return toString(QLatin1String("nullptr"));
}

template<>
inline bool qCompare(QString const &t1, QLatin1String const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, QString(t2), actual, expected, file, line);
}
template<>
inline bool qCompare(QLatin1String const &t1, QString const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(QString(t1), t2, actual, expected, file, line);
}

template <typename T>
inline bool qCompare(QList<T> const &t1, QList<T> const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    char msg[1024];
    msg[0] = '\0';
    bool isOk = true;
    const int actualSize = t1.count();
    const int expectedSize = t2.count();
    if (actualSize != expectedSize) {
        qsnprintf(msg, sizeof(msg), "Compared lists have different sizes.\n"
                  "   Actual   (%s) size: %d\n"
                  "   Expected (%s) size: %d", actual, actualSize, expected, expectedSize);
        isOk = false;
    }
    for (int i = 0; isOk && i < actualSize; ++i) {
        if (!(t1.at(i) == t2.at(i))) {
            char *val1 = toString(t1.at(i));
            char *val2 = toString(t2.at(i));

            qsnprintf(msg, sizeof(msg), "Compared lists differ at index %d.\n"
                      "   Actual   (%s): %s\n"
                      "   Expected (%s): %s", i, actual, val1 ? val1 : "<null>",
                      expected, val2 ? val2 : "<null>");
            isOk = false;

            delete [] val1;
            delete [] val2;
        }
    }
    return compare_helper(isOk, msg, nullptr, nullptr, actual, expected, file, line);
}

template <>
inline bool qCompare(QStringList const &t1, QStringList const &t2, const char *actual, const char *expected,
                            const char *file, int line)
{
    return qCompare<QString>(t1, t2, actual, expected, file, line);
}

template <typename T>
inline bool qCompare(QFlags<T> const &t1, T const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return qCompare(int(t1), int(t2), actual, expected, file, line);
}

template <typename T>
inline bool qCompare(QFlags<T> const &t1, int const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return qCompare(int(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(qint64 const &t1, qint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<qint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(qint64 const &t1, quint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<qint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(quint64 const &t1, quint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<quint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(qint32 const &t1, qint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<qint64>(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(quint32 const &t1, qint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<qint64>(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(quint32 const &t1, quint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<quint64>(t1), t2, actual, expected, file, line);
}

}
QT_END_NAMESPACE

#ifdef QT_TESTCASE_BUILDDIR
#  define QTEST_SET_MAIN_SOURCE_PATH  QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR);
#else
#  define QTEST_SET_MAIN_SOURCE_PATH  QTest::setMainSourcePath(__FILE__);
#endif

#define QTEST_APPLESS_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    TestObject tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv); \
}

#include <QtTest/qtestsystem.h>
#include <set>

#ifndef QT_NO_OPENGL
#  define QTEST_ADD_GPU_BLACKLIST_SUPPORT_DEFS \
    extern Q_TESTLIB_EXPORT std::set<QByteArray> *(*qgpu_features_ptr)(const QString &); \
    extern Q_GUI_EXPORT std::set<QByteArray> *qgpu_features(const QString &);
#  define QTEST_ADD_GPU_BLACKLIST_SUPPORT \
    qgpu_features_ptr = qgpu_features;
#else
#  define QTEST_ADD_GPU_BLACKLIST_SUPPORT_DEFS
#  define QTEST_ADD_GPU_BLACKLIST_SUPPORT
#endif

#if defined(QT_NETWORK_LIB)
#  include <QtTest/qtest_network.h>
#endif

#if defined(QT_WIDGETS_LIB)

#include <QtTest/qtest_widgets.h>

#ifdef QT_KEYPAD_NAVIGATION
#  define QTEST_DISABLE_KEYPAD_NAVIGATION QApplication::setNavigationMode(Qt::NavigationModeNone);
#else
#  define QTEST_DISABLE_KEYPAD_NAVIGATION
#endif

#define QTEST_MAIN(TestObject) \
QT_BEGIN_NAMESPACE \
QTEST_ADD_GPU_BLACKLIST_SUPPORT_DEFS \
QT_END_NAMESPACE \
int main(int argc, char *argv[]) \
{ \
    QApplication app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    QTEST_DISABLE_KEYPAD_NAVIGATION \
    QTEST_ADD_GPU_BLACKLIST_SUPPORT \
    TestObject tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv); \
}

#elif defined(QT_GUI_LIB)

#include <QtTest/qtest_gui.h>

#define QTEST_MAIN(TestObject) \
QT_BEGIN_NAMESPACE \
QTEST_ADD_GPU_BLACKLIST_SUPPORT_DEFS \
QT_END_NAMESPACE \
int main(int argc, char *argv[]) \
{ \
    QGuiApplication app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    QTEST_ADD_GPU_BLACKLIST_SUPPORT \
    TestObject tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv); \
}

#else

#define QTEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    QCoreApplication app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    TestObject tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv); \
}

#endif // QT_GUI_LIB

#define QTEST_GUILESS_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    QCoreApplication app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    TestObject tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv); \
}

#endif
