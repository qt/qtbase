/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtXml module of the Qt Toolkit.
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
#ifndef QDOMHELPERS_P_H
#define QDOMHELPERS_P_H

#include <qcoreapplication.h>
#include <qglobal.h>

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qxml.cpp and qdom.cpp. This header file may change from version to version without
// notice, or even be removed.
//
// We mean it.
//

class QDomDocumentPrivate;
class QDomNodePrivate;
class QXmlStreamReader;
class QXmlStreamAttributes;

/**************************************************************
 *
 * QXmlDocumentLocators
 *
 **************************************************************/

/* TODO: QXmlDocumentLocator can be removed when the SAX-based
 * implementation is removed. Right now it is needed for QDomBuilder
 * to work with both QXmlStreamReader and QXmlInputSource (SAX)
 * based implementations.
 */
class QXmlDocumentLocator
{
public:
    virtual ~QXmlDocumentLocator() = default;
    virtual int column() const = 0;
    virtual int line() const = 0;
};

class QDomDocumentLocator : public QXmlDocumentLocator
{
public:
    QDomDocumentLocator(QXmlStreamReader *r) : reader(r) {}
    ~QDomDocumentLocator() override = default;

    int column() const override;
    int line() const override;

private:
    QXmlStreamReader *reader;
};

/**************************************************************
 *
 * QDomBuilder
 *
 **************************************************************/

class QDomBuilder
{
public:
    QDomBuilder(QDomDocumentPrivate *d, QXmlDocumentLocator *l, bool namespaceProcessing);
    ~QDomBuilder();

    bool endDocument();
    bool startElement(const QString &nsURI, const QString &qName, const QXmlStreamAttributes &atts);
    bool endElement();
    bool characters(const QString &characters, bool cdata = false);
    bool processingInstruction(const QString &target, const QString &data);
    bool skippedEntity(const QString &name);
    bool startEntity(const QString &name);
    bool endEntity();
    bool startDTD(const QString &name, const QString &publicId, const QString &systemId);
    bool comment(const QString &characters);
    bool externalEntityDecl(const QString &name, const QString &publicId, const QString &systemId);
    bool notationDecl(const QString &name, const QString &publicId, const QString &systemId);
    bool unparsedEntityDecl(const QString &name, const QString &publicId, const QString &systemId,
                            const QString &notationName);

    void fatalError(const QString &message);

    using ErrorInfo = std::tuple<QString, int, int>;
    ErrorInfo error() const;

    QString errorMsg;
    int errorLine;
    int errorColumn;

private:
    QDomDocumentPrivate *doc;
    QDomNodePrivate *node;
    QXmlDocumentLocator *locator;
    QString entityName;
    bool nsProcessing;
};

/**************************************************************
 *
 * QDomParser
 *
 **************************************************************/

class QDomParser
{
    Q_DECLARE_TR_FUNCTIONS(QDomParser)
public:
    QDomParser(QDomDocumentPrivate *d, QXmlStreamReader *r, bool namespaceProcessing);

    bool parse();
    QDomBuilder::ErrorInfo errorInfo() const;

private:
    bool parseProlog();
    bool parseBody();
    bool parseMarkupDecl();

    QXmlStreamReader *reader;
    QDomDocumentLocator locator;
    QDomBuilder domBuilder;
};

QT_END_NAMESPACE

#endif // QDOMHELPERS_P_H
