/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTEST_H
#define QTEST_H

#include <QtTest/qtest_global.h>
#include <QtTest/qtestcase.h>
#include <QtTest/qtestdata.h>
#include <QtTest/qtestdata.h>
#include <QtTest/qbenchmark.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qurl.h>

#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qrect.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Test)

namespace QTest
{

template<> inline char *toString(const QLatin1String &str)
{
    return qstrdup(str.latin1());
}

template<> inline char *toString(const QString &str)
{
    return qstrdup(str.toLatin1().constData());
}

template<> inline char *toString(const QByteArray &ba)
{
    return QTest::toHexRepresentation(ba.constData(), ba.length());
}

template<> inline char *toString(const QTime &time)
{
    return time.isValid()
        ? qstrdup(time.toString(QLatin1String("hh:mm:ss.zzz")).toLatin1().constData())
        : qstrdup("Invalid QTime");
}

template<> inline char *toString(const QDate &date)
{
    return date.isValid()
        ? qstrdup(date.toString(QLatin1String("yyyy/MM/dd")).toLatin1().constData())
        : qstrdup("Invalid QDate");
}

template<> inline char *toString(const QDateTime &dateTime)
{
    return dateTime.isValid()
        ? qstrdup((dateTime.toString(QLatin1String("yyyy/MM/dd hh:mm:ss.zzz")) +
                  (dateTime.timeSpec() == Qt::LocalTime ? QLatin1String("[local time]") : QLatin1String("[UTC]"))).toLatin1().constData())
        : qstrdup("Invalid QDateTime");
}

template<> inline char *toString(const QChar &c)
{
    return qstrdup(QString::fromLatin1("QChar: '%1' (0x%2)").arg(c).arg(QString::number(static_cast<int>(c.unicode()), 16)).toLatin1().constData());
}

template<> inline char *toString(const QPoint &p)
{
    return qstrdup(QString::fromLatin1("QPoint(%1,%2)").arg(p.x()).arg(p.y()).toLatin1().constData());
}

template<> inline char *toString(const QSize &s)
{
    return qstrdup(QString::fromLatin1("QSize(%1x%2)").arg(s.width()).arg(s.height()).toLatin1().constData());
}

template<> inline char *toString(const QRect &s)
{
    return qstrdup(QString::fromLatin1("QRect(%1,%2 %5x%6) (bottomright %3,%4)").arg(s.left()).arg(s.top()).arg(s.right()).arg(s.bottom()).arg(s.width()).arg(s.height()).toLatin1().constData());
}

template<> inline char *toString(const QPointF &p)
{
    return qstrdup(QString::fromLatin1("QPointF(%1,%2)").arg(p.x()).arg(p.y()).toLatin1().constData());
}

template<> inline char *toString(const QSizeF &s)
{
    return qstrdup(QString::fromLatin1("QSizeF(%1x%2)").arg(s.width()).arg(s.height()).toLatin1().constData());
}

template<> inline char *toString(const QRectF &s)
{
    return qstrdup(QString::fromLatin1("QRectF(%1,%2 %5x%6) (bottomright %3,%4)").arg(s.left()).arg(s.top()).arg(s.right()).arg(s.bottom()).arg(s.width()).arg(s.height()).toLatin1().constData());
}

template<> inline char *toString(const QUrl &uri)
{
    return qstrdup(uri.toEncoded().constData());
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
                vstring.append(qvariant_cast<QString>(v).toLatin1());
            }
            else {
                vstring.append("<value not representable as string>");
            }
        }
    }
    vstring.append(')');

    return qstrdup(vstring.constData());
}

#ifndef QTEST_NO_SPECIALIZATIONS
template<>
#endif
inline bool qCompare(QString const &t1, QLatin1String const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare<QString>(t1, QString(t2), actual, expected, file, line);
}
#ifndef QTEST_NO_SPECIALIZATIONS
template<>
#endif
inline bool qCompare(QLatin1String const &t1, QString const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare<QString>(QString(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(QStringList const &t1, QStringList const &t2,
                    const char *actual, const char *expected, const char *file, int line)
{
    char msg[1024];
    msg[0] = '\0';
    bool isOk = true;
    if (t1.count() != t2.count()) {
        qt_snprintf(msg, 1024, "Compared QStringLists have different sizes.\n"
                    "   Actual (%s) size  : '%d'\n"
                    "   Expected (%s) size: '%d'", actual, t1.count(), expected, t2.count());
        isOk = false;
    }
    const int min = qMin(t1.count(), t2.count());
    for (int i = 0; isOk && i < min; ++i) {
        if (t1.at(i) != t2.at(i)) {
            qt_snprintf(msg, 1024, "Compared QStringLists differ at index %d.\n"
                        "   Actual (%s) : '%s'\n"
                        "   Expected (%s) : '%s'", i, actual, t1.at(i).toLatin1().constData(),
                        expected, t2.at(i).toLatin1().constData());
            isOk = false;
        }
    }
    return compare_helper(isOk, msg, file, line);
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

}
QT_END_NAMESPACE

#define QTEST_APPLESS_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#define QTEST_NOOP_MAIN \
int main(int argc, char *argv[]) \
{ \
    QObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#include <QtTest/qtestsystem.h>

#ifdef QT_GUI_LIB

#include <QtTest/qtest_gui.h>

#ifdef QT_KEYPAD_NAVIGATION
#  define QTEST_DISABLE_KEYPAD_NAVIGATION QApplication::setNavigationMode(Qt::NavigationModeNone);
#else
#  define QTEST_DISABLE_KEYPAD_NAVIGATION
#endif

#define QTEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    QApplication app(argc, argv); \
    QTEST_DISABLE_KEYPAD_NAVIGATION \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#else

#define QTEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    QCoreApplication app(argc, argv); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#endif // QT_GUI_LIB

QT_END_HEADER

#endif
