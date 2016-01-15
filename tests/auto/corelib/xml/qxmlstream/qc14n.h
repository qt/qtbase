/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

QT_FORWARD_DECLARE_CLASS(QIODevice)
QT_FORWARD_DECLARE_CLASS(QString)

#include <QtCore/QFlags>

class QC14N
{
public:
    static bool isEqual(QIODevice *const firstDocument,
                        QIODevice *const secondDocument,
                        QString *const message = 0);

private:
    static bool isDifferent(const QXmlStreamReader &r1,
                            const QXmlStreamReader &r2,
                            QString *const message);
    static bool isAttributesEqual(const QXmlStreamReader &r1,
                                  const QXmlStreamReader &r2,
                                  QString *const message);
};

#include <QXmlStreamReader>

/*! \internal

  \a firstDocument and \a secondDocument must be pointers to opened devices.
 */
bool QC14N::isEqual(QIODevice *const firstDocument,
                    QIODevice *const secondDocument,
                    QString *const message)
{
    qDebug() << Q_FUNC_INFO;
    if (!firstDocument)
        qFatal("%s: A valid firstDocument QIODevice pointer must be supplied", Q_FUNC_INFO);
    if (!secondDocument)
        qFatal("%s: A valid secondDocument QIODevice pointer must be supplied", Q_FUNC_INFO);
    if (!firstDocument->isReadable())
        qFatal("%s: The firstDocument device must be readable.", Q_FUNC_INFO);
    if (!secondDocument->isReadable())
        qFatal("%s: The secondDocument device must be readable.", Q_FUNC_INFO);

    QXmlStreamReader r1(firstDocument);
    QXmlStreamReader r2(secondDocument);

    while(!r1.atEnd())
    {
        if(r1.error())
        {
            if(message)
                *message = r1.errorString();

            return false;
        }
        else if(r2.error())
        {
            if(message)
                *message = r1.errorString();

            return false;
        }
        else
        {
            if(isDifferent(r1, r2, message))
                return true;
        }

        r1.readNext();
        r2.readNext();
    }

    if(!r2.atEnd())
    {
        if(message)
            *message = QLatin1String("Reached the end of the first document, while there was still content left in the second");

        return false;
    }

    /* And they lived happily ever after. */
    return true;
}

/*! \internal
 */
bool QC14N::isAttributesEqual(const QXmlStreamReader &r1,
                              const QXmlStreamReader &r2,
                              QString *const message)
{
    Q_UNUSED(message);

    const QXmlStreamAttributes &attrs1 = r1.attributes();
    const QXmlStreamAttributes &attrs2 = r2.attributes();
    const int len = attrs1.size();

    if(len != attrs2.size())
        return false;

    for(int i = 0; i < len; ++i)
    {
        if(!attrs2.contains(attrs1.at(i)))
            return false;
    }

    return true;
}

bool QC14N::isDifferent(const QXmlStreamReader &r1,
                        const QXmlStreamReader &r2,
                        QString *const message)
{
    // TODO error reporting can be a lot better here.
    if(r1.tokenType() != r2.tokenType())
        return false;

    switch(r1.tokenType())
    {
        case QXmlStreamReader::NoToken:
        /* Fallthrough. */
        case QXmlStreamReader::StartDocument:
        /* Fallthrough. */
        case QXmlStreamReader::EndDocument:
        /* Fallthrough. */
        case QXmlStreamReader::DTD:
            return true;
        case QXmlStreamReader::Invalid:
            return false;
        case QXmlStreamReader::StartElement:
        {
            return r1.qualifiedName() == r2.qualifiedName()
                   /* Yes, the namespace test below should be redundant, but with it we
                    * trap namespace bugs in QXmlStreamReader, if any. */
                   && r1.namespaceUri() == r2.namespaceUri()
                   && isAttributesEqual(r1, r2, message);

        }
        case QXmlStreamReader::EndElement:
        {
            return r1.qualifiedName() == r2.qualifiedName()
                   && r1.namespaceUri() == r2.namespaceUri()
                   && r1.name() == r2.name();
        }
        case QXmlStreamReader::Characters:
        /* Fallthrough. */
        case QXmlStreamReader::Comment:
            return r1.text() == r2.text();
        case QXmlStreamReader::EntityReference:
        case QXmlStreamReader::ProcessingInstruction:
        {
            return r1.processingInstructionTarget() == r2.processingInstructionTarget() &&
                   r2.processingInstructionData() == r2.processingInstructionData();

        }
        default:
            qFatal("%s: Unknown tokenType: %d", Q_FUNC_INFO, static_cast<int>(r1.tokenType()));
            return false;
    }
}

