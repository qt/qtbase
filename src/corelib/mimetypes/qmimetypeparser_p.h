/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef MIMETYPEPARSER_P_H
#define MIMETYPEPARSER_P_H

#include "qmimedatabase_p.h"
#include "qmimeprovider_p.h"

QT_BEGIN_NAMESPACE

class QIODevice;

// XML tags in MIME files
extern const char *const mimeInfoTagC;
extern const char *const mimeTypeTagC;
extern const char *const mimeTypeAttributeC;
extern const char *const subClassTagC;
extern const char *const commentTagC;
extern const char *const genericIconTagC;
extern const char *const nameAttributeC;
extern const char *const globTagC;
extern const char *const aliasTagC;
extern const char *const patternAttributeC;
extern const char *const weightAttributeC;
extern const char *const caseSensitiveAttributeC;
extern const char *const localeAttributeC;

extern const char *const magicTagC;
extern const char *const priorityAttributeC;

extern const char *const matchTagC;
extern const char *const matchValueAttributeC;
extern const char *const matchTypeAttributeC;
extern const char *const matchOffsetAttributeC;
extern const char *const matchMaskAttributeC;

class QMimeTypeParserBase
{
    Q_DISABLE_COPY(QMimeTypeParserBase)

public:
    QMimeTypeParserBase() {}
    virtual ~QMimeTypeParserBase() {}

    bool parse(QIODevice *dev, const QString &fileName, QString *errorMessage);

protected:
    virtual bool process(const QMimeType &t, QString *errorMessage) = 0;
    virtual bool process(const QMimeGlobPattern &t, QString *errorMessage) = 0;
    virtual void processParent(const QString &child, const QString &parent) = 0;
    virtual void processAlias(const QString &alias, const QString &name) = 0;
    virtual void processMagicMatcher(const QMimeMagicRuleMatcher &matcher) = 0;

private:
    enum ParseState {
        ParseBeginning,
        ParseMimeInfo,
        ParseMimeType,
        ParseComment,
        ParseGenericIcon,
        ParseIcon,
        ParseGlobPattern,
        ParseSubClass,
        ParseAlias,
        ParseMagic,
        ParseMagicMatchRule,
        ParseOtherMimeTypeSubTag,
        ParseError
    };

    static ParseState nextState(ParseState currentState, const QStringRef &startElement);
};


class QMimeTypeParser : public QMimeTypeParserBase
{
public:
    explicit QMimeTypeParser(QMimeXMLProvider &provider) : m_provider(provider) {}

protected:
    inline bool process(const QMimeType &t, QString *)
    { m_provider.addMimeType(t); return true; }

    inline bool process(const QMimeGlobPattern &glob, QString *)
    { m_provider.addGlobPattern(glob); return true; }

    inline void processParent(const QString &child, const QString &parent)
    { m_provider.addParent(child, parent); }

    inline void processAlias(const QString &alias, const QString &name)
    { m_provider.addAlias(alias, name); }

    inline void processMagicMatcher(const QMimeMagicRuleMatcher &matcher)
    { m_provider.addMagicMatcher(matcher); }

private:
    QMimeXMLProvider &m_provider;
};

QT_END_NAMESPACE

#endif // MIMETYPEPARSER_P_H
