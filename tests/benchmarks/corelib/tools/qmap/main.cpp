/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QFile>
#include <QMap>
#include <QString>
#include <QTest>
#include <qdebug.h>


class tst_QMap : public QObject
{
    Q_OBJECT

private slots:
    void insertion_int_int();
    void insertion_int_string();
    void insertion_string_int();

    void lookup_int_int();
    void lookup_int_string();
    void lookup_string_int();

    void iteration();
    void toStdMap();
    void iterator_begin();

    void ctorStdMap();

    void insertion_int_intx();
    void insertion_int_int_with_hint1();
    void insertion_int_int2();
    void insertion_int_int_with_hint2();

    void insertion_string_int2();
    void insertion_string_int2_hint();
};


void tst_QMap::insertion_int_int()
{
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            map.insert(i, i);
    }
}

void tst_QMap::insertion_int_intx()
{
    // This is the same test - but executed later.
    // The results in the beginning of the test seems to be a somewhat inaccurate.
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            map.insert(i, i);
    }
}

void tst_QMap::insertion_int_int_with_hint1()
{
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            map.insert(map.constEnd(), i, i);
    }
}

void tst_QMap::insertion_int_int2()
{
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = 100000; i >= 0; --i)
            map.insert(i, i);
    }
}

void tst_QMap::insertion_int_int_with_hint2()
{
    QMap<int, int> map;
    QBENCHMARK {
        for (int i = 100000; i >= 0; --i)
            map.insert(map.constBegin(), i, i);
    }
}

void tst_QMap::insertion_int_string()
{
    QMap<int, QString> map;
    QString str("Hello World");
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            map.insert(i, str);
    }
}

void tst_QMap::insertion_string_int()
{
    QMap<QString, int> map;
    QString str("Hello World");
    QBENCHMARK {
        for (int i = 1; i < 100000; ++i) {
            str[0] = QChar(i);
            map.insert(str, i);
        }
    }
}


void tst_QMap::lookup_int_int()
{
    QMap<int, int> map;
    for (int i = 0; i < 100000; ++i)
        map.insert(i, i);

    int sum = 0;
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
             sum += map.value(i);
    }
}

void tst_QMap::lookup_int_string()
{
    QMap<int, QString> map;
    QString str("Hello World");
    for (int i = 0; i < 100000; ++i)
        map.insert(i, str);

    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
             str += map.value(i);
    }
}

void tst_QMap::lookup_string_int()
{
    QMap<QString, int> map;
    QString str("Hello World");
    for (int i = 1; i < 100000; ++i) {
        str[0] = QChar(i);
        map.insert(str, i);
    }

    int sum = 0;
    QBENCHMARK {
        for (int i = 1; i < 100000; ++i) {
            str[0] = QChar(i);
            sum += map.value(str);
        }
    }
}

// iteration speed doesn't depend on the type of the map.
void tst_QMap::iteration()
{
    QMap<int, int> map;
    for (int i = 0; i < 100000; ++i)
        map.insert(i, i);

    int j = 0;
    QBENCHMARK {
        for (int i = 0; i < 100; ++i) {
            QMap<int, int>::const_iterator it = map.constBegin();
            QMap<int, int>::const_iterator end = map.constEnd();
            while (it != end) {
                j += *it;
                ++it;
            }
        }
    }
}

void tst_QMap::toStdMap()
{
    QMap<int, int> map;
    for (int i = 0; i < 100000; ++i)
        map.insert(i, i);

    QBENCHMARK {
        std::map<int, int> n = map.toStdMap();
        n.begin();
    }
}

void tst_QMap::iterator_begin()
{
    QMap<int, int> map;
    for (int i = 0; i < 100000; ++i)
        map.insert(i, i);

    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            QMap<int, int>::const_iterator it = map.constBegin();
            QMap<int, int>::const_iterator end = map.constEnd();
            if (it == end) // same as if (false)
                ++it;
        }
    }
}

void tst_QMap::ctorStdMap()
{
    std::map<int, int> map;
    for (int i = 0; i < 100000; ++i)
        map.insert(std::pair<int, int>(i, i));

    QBENCHMARK {
        QMap<int, int> qmap(map);
        qmap.constBegin();
    }
}

class XString : public QString
{
public:
    bool operator < (const XString& x) const // an expensive operator <
    {
        return toInt() < x.toInt();
    }
};

void tst_QMap::insertion_string_int2()
{
    QMap<XString, int> map;
    QBENCHMARK {
        for (int i = 1; i < 5000; ++i) {
            XString str;
            str.setNum(i);
            map.insert(str, i);
        }
    }
}

void tst_QMap::insertion_string_int2_hint()
{
    QMap<XString, int> map;
    QBENCHMARK {
        for (int i = 1; i < 5000; ++i) {
            XString str;
            str.setNum(i);
            map.insert(map.end(), str, i);
        }
    }
}

QTEST_MAIN(tst_QMap)

#include "main.moc"
