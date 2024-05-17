// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QDebug>
#include <QtCore/QFlags>
#include <QtCore/QXmlStreamReader>

#include <algorithm>

class QC14N
{
public:
    static bool isEqual(QIODevice *const firstDocument,
                        QIODevice *const secondDocument,
                        QString *const message = nullptr);

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
    if (attrs1.size() != attrs2.size())
        return false;

    auto existsInOtherList = [&attrs2](const auto &attr) { return attrs2.contains(attr); };
    return std::all_of(attrs1.cbegin(), attrs1.cend(), existsInOtherList);
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

