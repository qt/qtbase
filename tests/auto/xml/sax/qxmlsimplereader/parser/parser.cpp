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


#include <qxml.h>
#include <qregexp.h>

#include "parser.h"

class ContentHandler : public QXmlDefaultHandler
{
public:
    ContentHandler();

    // QXmlContentHandler methods
    bool startDocument();
    bool endDocument();
    bool startElement(const QString &namespaceURI,
                      const QString &localName,
                      const QString &qName,
                      const QXmlAttributes &atts);
    bool endElement(const QString &namespaceURI,
                    const QString &localName,
                    const QString &qName);
    bool characters(const QString &ch);
    void setDocumentLocator(QXmlLocator *locator);
    bool startPrefixMapping(const QString &prefix, const QString &uri);
    bool endPrefixMapping(const QString &prefix);
    bool ignorableWhitespace(const QString &ch);
    bool processingInstruction(const QString &target, const QString &data);
    bool skippedEntity(const QString &name);

    // QXmlErrorHandler methods
    bool warning(const QXmlParseException &exception);
    bool error(const QXmlParseException &exception);
    bool fatalError(const QXmlParseException &exception);

    // QXmlDTDHandler methods
    bool notationDecl(const QString &name, const QString &publicId,
                      const QString &systemId);
    bool unparsedEntityDecl(const QString &name,
                            const QString &publicId,
                            const QString &systemId,
                            const QString &notationName);

    // QXmlEntityResolver methods
    bool resolveEntity(const QString &publicId,
                       const QString &systemId,
                       QXmlInputSource *&);

    // QXmlLexicalHandler methods
    bool startDTD (const QString &name, const QString &publicId, const QString &systemId);
    bool endDTD();
    bool startEntity(const QString &name);
    bool endEntity(const QString &name);
    bool startCDATA();
    bool endCDATA();
    bool comment(const QString &ch);

    // QXmlDeclHandler methods
    bool attributeDecl(const QString &eName, const QString &aName, const QString &type, const QString &valueDefault, const QString &value);
    bool internalEntityDecl(const QString &name, const QString &value);
    bool externalEntityDecl(const QString &name, const QString &publicId, const QString &systemId);


    const QString &result() const { return m_result; }
    const QString &errorMsg() const { return m_error_msg; }

private:
    QString nestPrefix() const { return QString().fill(' ', 3*m_nest); }
    QString formatAttributes(const QXmlAttributes & atts);
    QString escapeStr(const QString &s);

    unsigned m_nest;
    QString m_result, m_error_msg;
};

ContentHandler::ContentHandler()
{
    m_nest = 0;
}


bool ContentHandler::startDocument()
{
    m_result += nestPrefix();
    m_result += "startDocument()\n";
    ++m_nest;
    return true;
}

bool ContentHandler::endDocument()
{
    --m_nest;
    m_result += nestPrefix();
    m_result += "endDocument()\n";
    return true;
}

bool ContentHandler::startElement(const QString &namespaceURI,
                                  const QString &localName,
                                  const QString &qName,
                                  const QXmlAttributes &atts)
{
    m_result += nestPrefix();
    m_result += "startElement(namespaceURI=\"" + escapeStr(namespaceURI)
                + "\", localName=\"" + escapeStr(localName)
                + "\", qName=\"" + escapeStr(qName)
                + "\", atts=[" + formatAttributes(atts) + "])\n";
    ++m_nest;
    return true;
}

QString ContentHandler::escapeStr(const QString &s)
{
    QString result = s;
    result.replace(QRegExp("\""), "\\\"");
    result.replace(QRegExp("\\"), "\\\\");
    result.replace(QRegExp("\n"), "\\n");
    result.replace(QRegExp("\r"), "\\r");
    result.replace(QRegExp("\t"), "\\t");
    return result;
}

QString ContentHandler::formatAttributes(const QXmlAttributes &atts)
{
    QString result;
    for (int i = 0, cnt = atts.count(); i < cnt; ++i) {
        if (i != 0) result += ", ";
        result += "{localName=\"" + escapeStr(atts.localName(i))
                    + "\", qName=\"" + escapeStr(atts.qName(i))
                    + "\", uri=\"" + escapeStr(atts.uri(i))
                    + "\", type=\"" + escapeStr(atts.type(i))
                    + "\", value=\"" + escapeStr(atts.value(i)) + "\"}";
    }
    return result;
}

bool ContentHandler::endElement(const QString &namespaceURI,
                                const QString &localName,
                                const QString &qName)
{
    --m_nest;
    m_result += nestPrefix();
    m_result += "endElement(namespaceURI=\"" + escapeStr(namespaceURI)
                + "\", localName=\"" + escapeStr(localName)
                + "\", qName=\"" + escapeStr(qName) + "\")\n";
    return true;
}

bool ContentHandler::characters(const QString &ch)
{
    m_result += nestPrefix();
    m_result += "characters(ch=\"" + escapeStr(ch) + "\")\n";
    return true;
}

void ContentHandler::setDocumentLocator(QXmlLocator *locator)
{
    m_result += nestPrefix();
    m_result += "setDocumentLocator(locator={columnNumber="
                + QString::number(locator->columnNumber())
                + ", lineNumber=" + QString::number(locator->lineNumber())
                + "})\n";
}

bool ContentHandler::startPrefixMapping (const QString &prefix, const QString & uri)
{
    m_result += nestPrefix();
    m_result += "startPrefixMapping(prefix=\"" + escapeStr(prefix)
                    + "\", uri=\"" + escapeStr(uri) + "\")\n";
    ++m_nest;
    return true;
}

bool ContentHandler::endPrefixMapping(const QString &prefix)
{
    --m_nest;
    m_result += nestPrefix();
    m_result += "endPrefixMapping(prefix=\"" + escapeStr(prefix) + "\")\n";
    return true;
}

bool ContentHandler::ignorableWhitespace(const QString & ch)
{
    m_result += nestPrefix();
    m_result += "ignorableWhitespace(ch=\"" + escapeStr(ch) + "\")\n";
    return true;
}

bool ContentHandler::processingInstruction(const QString &target, const QString &data)
{
    m_result += nestPrefix();
    m_result += "processingInstruction(target=\"" + escapeStr(target)
                + "\", data=\"" + escapeStr(data) + "\")\n";
    return true;
}

bool ContentHandler::skippedEntity (const QString & name)
{
    m_result += nestPrefix();
    m_result += "skippedEntity(name=\"" + escapeStr(name) + "\")\n";
    return true;
}

bool ContentHandler::warning(const QXmlParseException & exception)
{
    m_error_msg = QString("Warning %1:%2: %3")
                    .arg(exception.columnNumber())
                    .arg(exception.lineNumber())
                    .arg(exception.message());
    m_result += nestPrefix();
    m_result += "warning(exception={columnNumber="
                    + QString::number(exception.columnNumber())
                    + ", lineNumber="
                    + QString::number(exception.lineNumber())
                    + ", publicId=\"" + escapeStr(exception.publicId())
                    + "\", systemId=\"" + escapeStr(exception.systemId())
                    + "\", message=\"" + escapeStr(exception.message())
                    + "\"})\n";
    return true;
}

bool ContentHandler::error(const QXmlParseException & exception)
{
    m_error_msg = QString("Error %1:%2: %3")
                    .arg(exception.columnNumber())
                    .arg(exception.lineNumber())
                    .arg(exception.message());
    m_result += nestPrefix();
    m_result += "error(exception={columnNumber="
                    + QString::number(exception.columnNumber())
                    + ", lineNumber="
                    + QString::number(exception.lineNumber())
                    + ", publicId=\"" + escapeStr(exception.publicId())
                    + "\", systemId=\"" + escapeStr(exception.systemId())
                    + "\", message=\"" + escapeStr(exception.message())
                    + "\"})\n";
    return true;
}

bool ContentHandler::fatalError(const QXmlParseException & exception)
{
    m_error_msg = QString("Fatal error %1:%2: %3")
                    .arg(exception.columnNumber())
                    .arg(exception.lineNumber())
                    .arg(exception.message());
    m_result += nestPrefix();
    m_result += "fatalError(exception={columnNumber="
                    + QString::number(exception.columnNumber())
                    + ", lineNumber="
                    + QString::number(exception.lineNumber())
                    + ", publicId=\"" + escapeStr(exception.publicId())
                    + "\", systemId=\"" + escapeStr(exception.systemId())
                    + "\", message=\"" + escapeStr(exception.message())
                    + "\"})\n";
    return true;
}

bool ContentHandler::notationDecl(const QString &name,
                                  const QString &publicId,
                                  const QString &systemId )
{
    m_result += nestPrefix();
    m_result += "notationDecl(name=\"" + escapeStr(name) + "\", publicId=\""
                    + escapeStr(publicId) + "\", systemId=\""
                    + escapeStr(systemId) + "\")\n";
    return true;
}

bool ContentHandler::unparsedEntityDecl(const QString &name,
                                        const QString &publicId,
                                        const QString &systemId,
                                        const QString &notationName )
{
    m_result += nestPrefix();
    m_result += "unparsedEntityDecl(name=\"" + escapeStr(name)
                    + "\", publicId=\"" + escapeStr(publicId)
                    + "\", systemId=\"" + escapeStr(systemId)
                    + "\", notationName=\"" + escapeStr(notationName)
                    + "\")\n";
    return true;
}

bool ContentHandler::resolveEntity(const QString &publicId,
                                   const QString &systemId,
                                   QXmlInputSource *&)
{
    m_result += nestPrefix();
    m_result += "resolveEntity(publicId=\"" + escapeStr(publicId)
                    + "\", systemId=\"" + escapeStr(systemId)
                    + "\", ret={})\n";
    return true;
}

bool ContentHandler::startDTD ( const QString & name, const QString & publicId, const QString & systemId )
{
    m_result += nestPrefix();
    m_result += "startDTD(name=\"" + escapeStr(name)
                    + "\", publicId=\"" + escapeStr(publicId)
                    + "\", systemId=\"" + escapeStr(systemId) + "\")\n";
    ++m_nest;
    return true;
}

bool ContentHandler::endDTD ()
{
    --m_nest;
    m_result += nestPrefix();
    m_result += "endDTD()\n";
    return true;
}

bool ContentHandler::startEntity ( const QString & name )
{
    m_result += nestPrefix();
    m_result += "startEntity(name=\"" + escapeStr(name) + "\")\n";
    ++m_nest;
    return true;
}

bool ContentHandler::endEntity ( const QString & name )
{
    --m_nest;
    m_result += nestPrefix();
    m_result += "endEntity(name=\"" + escapeStr(name) + "\")\n";
    return true;
}

bool ContentHandler::startCDATA ()
{
    m_result += nestPrefix();
    m_result += "startCDATA()\n";
    ++m_nest;
    return true;
}

bool ContentHandler::endCDATA ()
{
    --m_nest;
    m_result += nestPrefix();
    m_result += "endCDATA()\n";
    return true;
}

bool ContentHandler::comment ( const QString & ch )
{
    m_result += nestPrefix();
    m_result += "comment(ch=\"" + escapeStr(ch) + "\")\n";
    return true;
}

bool ContentHandler::attributeDecl(const QString &eName,
                                   const QString &aName,
                                   const QString &type,
                                   const QString &valueDefault,
                                   const QString &value)
{
    m_result += nestPrefix();
    m_result += "attributeDecl(eName=\"" + escapeStr(eName) + "\", aName=\""
                + escapeStr(aName) + "\", type=\"" + escapeStr(type)
                + "\", valueDefault=\"" + escapeStr(valueDefault)
                + "\", value=\"" + escapeStr(value) + "\")\n";
    return true;
}

bool ContentHandler::internalEntityDecl(const QString &name,
                                        const QString &value)
{
    m_result += nestPrefix();
    m_result += "internatlEntityDecl(name=\"" + escapeStr(name)
                    + "\", value=\"" + escapeStr(value) + "\")\n";
    return true;
}

bool ContentHandler::externalEntityDecl(const QString &name,
                                        const QString &publicId,
                                        const QString &systemId)
{
    m_result += nestPrefix();
    m_result += "externalEntityDecl(name=\"" + escapeStr(name)
                    + "\", publicId=\"" + escapeStr(publicId)
                    + "\", systemId=\"" + escapeStr(systemId) + "\")\n";
    return true;
}

Parser::Parser()
{
    handler = new ContentHandler;
    setContentHandler(handler);
    setDTDHandler(handler);
    setDeclHandler(handler);
    setEntityResolver(handler);
    setErrorHandler(handler);
    setLexicalHandler(handler);
}

Parser::~Parser()
{
    delete handler;
}

bool Parser::parseFile(QFile *file)
{
    QXmlInputSource source(file);
    return parse(source);
}

QString Parser::result() const
{
    return handler->result();
}

QString Parser::errorMsg() const
{
    return handler->errorMsg();
}
