/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <QStringList>
#include <QMap>
#include <QVector>

struct ConfigFile
{
    struct Entry
    {
        inline Entry() : lineNumber(-1) {}
        int lineNumber;
        QString key;
        QString value;
    };
    struct Section : public QVector<Entry>
    {
        inline bool contains(const QString &key) const
        {
            for (int i = 0; i < count(); ++i)
                if (at(i).key == key)
                    return true;
            return false;
        }
        inline QString value(const QString &key, const QString &defaultValue = QString()) const
        {
            for (int i = 0; i < count(); ++i)
                if (at(i).key == key)
                    return at(i).value;
            return defaultValue;
        }
    };
    typedef QMap<QString, Section> SectionMap;

    static SectionMap parse(const QString &fileName);
    static SectionMap parse(QIODevice *dev);
};

#endif // CONFIGFILE_H

