/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/
#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <QStringList>
#include <QMap>
#include <QVector>
#include <QIODevice>

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

