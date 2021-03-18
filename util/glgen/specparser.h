/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utilities of the Qt Toolkit.
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

#ifndef SPECPARSER_H
#define SPECPARSER_H

#include <QStringList>
#include <QVariant>

class QTextStream;
class QXmlStreamReader;

struct Version {
    int major;
    int minor;
};

inline bool operator == (const Version &lhs, const Version &rhs)
{
    return (lhs.major == rhs.major && lhs.minor == rhs.minor);
}

inline bool operator != (const Version &lhs, const Version &rhs)
{
    return !(lhs == rhs);
}

inline bool operator < (const Version &lhs, const Version &rhs)
{
    if (lhs.major != rhs.major)
        return (lhs.major < rhs.major);
    else
        return (lhs.minor < rhs.minor);
}

inline bool operator > (const Version &lhs, const Version &rhs)
{
    if (lhs.major != rhs.major)
        return (lhs.major > rhs.major);
    else
        return (lhs.minor > rhs.minor);
}

inline bool operator >= (const Version &lhs, const Version &rhs)
{
    return !(lhs < rhs);
}


inline bool operator <= (const Version &lhs, const Version &rhs)
{
    return !(lhs > rhs);
}

inline uint qHash(const Version &v)
{
    return qHash(v.major * 100 + v.minor * 10);
}

struct VersionProfile
{
    enum OpenGLProfile {
        CoreProfile = 0,
        CompatibilityProfile
    };

    inline bool hasProfiles() const
    {
        return ( version.major > 3
                 || (version.major == 3 && version.minor > 1));
    }

    Version version;
    OpenGLProfile profile;
};

inline bool operator == (const VersionProfile &lhs, const VersionProfile &rhs)
{
    if (lhs.profile != rhs.profile)
        return false;
    return lhs.version == rhs.version;
}

inline bool operator != (const VersionProfile &lhs, const VersionProfile &rhs)
{
    return !(lhs == rhs);
}

inline bool operator < (const VersionProfile &lhs, const VersionProfile &rhs)
{
    if (lhs.profile != rhs.profile)
        return (lhs.profile < rhs.profile);
    return (lhs.version < rhs.version);
}

inline uint qHash(const VersionProfile &v)
{
    return qHash(static_cast<int>(v.profile * 1000) + v.version.major * 100 + v.version.minor * 10);
}

struct Argument
{
    enum Direction {
        In = 0,
        Out
    };

    enum Mode {
        Value = 0,
        Array,
        Reference
    };

    QString type;
    QString name;
    Direction direction;
    Mode mode;
};

struct Function
{
    QString returnType;
    QString name;
    QList<Argument> arguments;
};

inline bool operator== (const Argument &lhs, const Argument &rhs)
{
    if ((lhs.type != rhs.type) || (lhs.name != rhs.name) || (lhs.direction != rhs.direction)) {
        return false;
    }

    return (lhs.mode != rhs.mode);
}

inline bool operator!= (const Argument &lhs, const Argument &rhs)
{
    return !(lhs == rhs);
}

inline bool operator== (const Function &lhs, const Function &rhs)
{
    if ((lhs.returnType != rhs.returnType) || (lhs.name != rhs.name)) {
        return false;
    }

    return (lhs.arguments == rhs.arguments);
}

inline bool operator!= (const Function &lhs, const Function &rhs)
{
    return !(lhs == rhs);
}

typedef QList<Function> FunctionList;
typedef QMap<VersionProfile, FunctionList> FunctionCollection;

struct FunctionProfile
{
    VersionProfile::OpenGLProfile profile;
    Function function;
};

class SpecParser
{
public:
    virtual ~SpecParser() {}

    QString specFileName() const
    {
        return m_specFileName;
    }

    QString typeMapFileName() const
    {
        return m_typeMapFileName;
    }

    virtual QList<Version> versions() const = 0;

    QList<VersionProfile> versionProfiles() const {return versionFunctions().uniqueKeys();}

    QList<Function> functionsForVersion(const VersionProfile &v) const
    {
        return versionFunctions().values(v);
    }

    QStringList extensions() const
    {
        return QStringList(extensionFunctions().uniqueKeys());
    }

    QList<Function> functionsForExtension(const QString &extension)
    {
        QList<Function> func;

        Q_FOREACH (const FunctionProfile &f, extensionFunctions().values(extension))
            func.append(f.function);

        return func;
    }

    void setSpecFileName(QString arg)
    {
        m_specFileName = arg;
    }

    void setTypeMapFileName(QString arg)
    {
        m_typeMapFileName = arg;
    }

    virtual bool parse() = 0;

protected:
    virtual const QMultiHash<VersionProfile, Function> &versionFunctions() const = 0;
    virtual const QMultiMap<QString, FunctionProfile> &extensionFunctions() const = 0;

private:
    QString m_specFileName;
    QString m_typeMapFileName;
};

#endif // SPECPARSER_H
