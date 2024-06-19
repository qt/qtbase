// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qformdatabuilder.h"

#include <QtCore/private/qstringconverter_p.h>
#if QT_CONFIG(mimetype)
#include "QtCore/qmimedatabase.h"
#endif

#include <vector>

QT_BEGIN_NAMESPACE

/*!
    \class QFormDataPartBuilder
    \brief The QFormDataPartBuilder class is a convenience class to simplify
    the construction of QHttpPart objects.
    \since 6.8

    \ingroup network
    \ingroup shared
    \inmodule QtNetwork

    The QFormDataPartBuilder class can be used to build a QHttpPart object with
    the content disposition header set to be form-data by default. Then the
    generated object can be used as part of a multipart message (which is
    represented by the QHttpMultiPart class).

    \sa QHttpPart, QHttpMultiPart, QFormDataBuilder
*/

class QFormDataPartBuilderPrivate
{
public:
    explicit QFormDataPartBuilderPrivate(QAnyStringView name);
    QHttpPart build();

    QString m_name;
    QByteArray m_mimeType;
    QString m_originalBodyName;
    QHttpHeaders m_httpHeaders;
    std::variant<QIODevice*, QByteArray> m_body;
};

QFormDataPartBuilderPrivate::QFormDataPartBuilderPrivate(QAnyStringView name)
    : m_name{name.toString()}
{

}


static void escapeNameAndAppend(QByteArray &dst, QByteArrayView src)
{
    for (auto c : src) {
        if (c == '"' || c == '\\')
            dst += '\\';
        dst += c;
    }
}

static void escapeNameAndAppend(QByteArray &dst, QStringView src)
{
    qsizetype last = 0;

    // equivalent to for (auto chunk : qTokenize(src, any_of("\\\""))), if there was such a thing
    for (qsizetype i = 0, end = src.size(); i != end; ++i) {
        const auto c = src[i];
        if (c == u'"' || c == u'\\') {
            const auto chunk = src.sliced(last, i - last);
            dst += QUtf8::convertFromUnicode(chunk); // ### optimize
            dst += '\\';
            last = i;
        }
    }
    dst += QUtf8::convertFromUnicode(src.sliced(last));
}

/*!
    \fn QFormDataPartBuilder::QFormDataPartBuilder(QFormDataPartBuilder &&other) noexcept

    Move-constructs a QFormDataPartBuilder instance, making it point at the same
    object that \a other was pointing to.
*/

/*!
    \fn QFormDataPartBuilder &QFormDataPartBuilder::operator=(QFormDataPartBuilder &&other)

    Move-assigns \a other to this QFormDataPartBuilder instance.
*/

/*!
    \fn QFormDataPartBuilder::QFormDataPartBuilder(const QFormDataPartBuilder &other)

    Constructs a copy of \a other. The object is valid for as long as the associated
    QFormDataBuilder has not been destroyed.

    The data of the copy is shared (shallow copy): modifying one part will also change
    the other.

    \code
        QFormDataPartBuilder foo()
        {
            QFormDataBuilder builder;
            auto qfdpb1 = builder.part("First"_L1);
            auto qfdpb2 = qfdpb1; // this creates a shallow copy

            qfdpb2.setBodyDevice(&image, "cutecat.jpg"); // qfdpb1 is also modified

            return qfdbp2;  // invalid, builder is destroyed at the end of the scope
        }
    \endcode
*/

/*!
    \fn QFormDataPartBuilder& QFormDataPartBuilder::operator=(const QFormDataPartBuilder &other)

    Assigns \a other to QFormDataPartBuilder and returns a reference to this
    QFormDataPartBuilder. The object is valid for as long as the associated QFormDataBuilder
    has not been destroyed.

    The data of the copy is shared (shallow copy): modifying one part will also change the other.

    \code
        QFormDataPartBuilder foo()
        {
            QFormDataBuilder builder;
            auto qfdpb1 = builder.part("First"_L1);
            auto qfdpb2 = qfdpb1; // this creates a shallow copy

            qfdpb2.setBodyDevice(&image, "cutecat.jpg"); // qfdpb1 is also modified

            return qfdbp2;  // invalid, builder is destroyed at the end of the scope
        }
    \endcode
*/

/*!
    \fn QFormDataPartBuilder::~QFormDataPartBuilder()

    Destroys the QFormDataPartBuilder object.
*/

static void convertInto_impl(QByteArray &dst, QUtf8StringView in)
{
    dst.clear();
    dst += QByteArrayView{in}; // it's ASCII, anyway
}

static void convertInto_impl(QByteArray &dst, QLatin1StringView in)
{
    dst.clear();
    dst += QByteArrayView{in}; // it's ASCII, anyway
}

static void convertInto_impl(QByteArray &dst, QStringView in)
{
    dst.resize(in.size());
    (void)QLatin1::convertFromUnicode(dst.data(), in);
}

static void convertInto(QByteArray &dst, QAnyStringView in)
{
    in.visit([&dst](auto in) { convertInto_impl(dst, in); });
}

QFormDataPartBuilder QFormDataPartBuilder::setBodyHelper(const QByteArray &data,
                                                         QAnyStringView name,
                                                         QAnyStringView mimeType)
{
    Q_D(QFormDataPartBuilder);

    d->m_originalBodyName = name.toString();
    convertInto(d->m_mimeType, mimeType);
    d->m_body = data;
    return *this;
}

/*!
    Sets \a data as the body of this MIME part and, if given, \a fileName as the
    file name parameter in the content disposition header.

    If \a mimeType is not given (is empty), then QFormDataPartBuilder tries to
    auto-detect the mime-type of \a data using QMimeDatabase.

    A subsequent call to setBodyDevice() discards the body and the device will
    be used instead.

    For a large amount of data (e.g. an image), setBodyDevice() is preferred,
    which will not copy the data internally.

    \sa setBodyDevice()
*/

QFormDataPartBuilder QFormDataPartBuilder::setBody(QByteArrayView data,
                                                   QAnyStringView fileName,
                                                   QAnyStringView mimeType)
{
    return setBody(data.toByteArray(), fileName, mimeType);
}

/*!
    Sets \a body as the body device of this part and \a fileName as the file
    name parameter in the content disposition header.

    If \a mimeType is not given (is empty), then QFormDataPartBuilder tries to
    auto-detect the mime-type of \a body using QMimeDatabase.

    A subsequent call to setBody() discards the body device and the data set by
    setBody() will be used instead.

    For large amounts of data this method should be preferred over setBody(),
    because the content is not copied when using this method, but read
    directly from the device.

    \a body must be open and readable. QFormDataPartBuilder does not take
    ownership of \a body, i.e. the device must be closed and destroyed if
    necessary.

    \note If \a body is sequential (e.g. sockets, but not files),
    QNetworkAccessManager::post() should be called after \a body has emitted
    finished().

    \sa setBody(), QHttpPart::setBodyDevice()
  */

QFormDataPartBuilder QFormDataPartBuilder::setBodyDevice(QIODevice *body, QAnyStringView fileName,
                                                         QAnyStringView mimeType)
{
    Q_D(QFormDataPartBuilder);

    d->m_originalBodyName = fileName.toString();
    convertInto(d->m_mimeType, mimeType);
    d->m_body = body;
    return *this;
}

/*!
    Sets the headers specified in \a headers.

    \note The "content-type" and "content-disposition" headers, if any are
    specified in \a headers, will be overwritten by the class.
*/

QFormDataPartBuilder QFormDataPartBuilder::setHeaders(const QHttpHeaders &headers)
{
    Q_D(QFormDataPartBuilder);

    d->m_httpHeaders = headers;
    return *this;
}

/*!
    \internal

    Generates a QHttpPart and sets the content disposition header as form-data.

    When this function called, it uses the MIME database to deduce the type the
    body based on its name and then sets the deduced type as the content type
    header.
*/

QHttpPart QFormDataPartBuilderPrivate::build()
{
    QHttpPart httpPart;

    QByteArray headerValue;

    headerValue += "form-data; name=\"";
    escapeNameAndAppend(headerValue, m_name);
    headerValue += "\"";

    if (!m_originalBodyName.isNull()) {
        const bool utf8 = !QtPrivate::isAscii(m_originalBodyName);
        const auto enc = utf8 ? m_originalBodyName.toUtf8() : m_originalBodyName.toLatin1();
        headerValue += "; filename=\"";
        escapeNameAndAppend(headerValue, enc);
        headerValue += "\"";
        if (utf8) {
            // For 'filename*' production see
            // https://datatracker.ietf.org/doc/html/rfc5987#section-3.2.1
            // For providing both filename and filename* parameters see
            // https://datatracker.ietf.org/doc/html/rfc6266#section-4.3 and
            // https://datatracker.ietf.org/doc/html/rfc8187#section-4.2
            headerValue += "; filename*=UTF-8''" + enc.toPercentEncoding();
        }
    }

#if QT_CONFIG(mimetype)
    if (m_mimeType.isEmpty()) {
        // auto-detect
        QMimeDatabase db;
        convertInto(m_mimeType, std::visit([&](auto &arg) {
                return db.mimeTypeForFileNameAndData(m_originalBodyName, arg);
            }, m_body).name());
    }
#endif

    for (qsizetype i = 0; i < m_httpHeaders.size(); i++) {
        httpPart.setRawHeader(QByteArrayView(m_httpHeaders.nameAt(i)).toByteArray(),
                                             m_httpHeaders.valueAt(i).toByteArray());
    }

    if (!m_mimeType.isEmpty())
        httpPart.setHeader(QNetworkRequest::ContentTypeHeader, m_mimeType);

    httpPart.setHeader(QNetworkRequest::ContentDispositionHeader, std::move(headerValue));

    if (auto d = std::get_if<QIODevice*>(&m_body))
        httpPart.setBodyDevice(*d);
    else if (auto b = std::get_if<QByteArray>(&m_body))
        httpPart.setBody(*b);
    else
        Q_UNREACHABLE();

    return httpPart;
}

/*!
    \class QFormDataBuilder
    \brief The QFormDataBuilder class is a convenience class to simplify
    the construction of QHttpMultiPart objects.
    \since 6.8

    \ingroup network
    \ingroup shared
    \inmodule QtNetwork

    The QFormDataBuilder class can be used to build a QHttpMultiPart object
    with the content type set to be FormDataType by default.

    The snippet below demonstrates how to build a multipart message with
    QFormDataBuilder:

    \code
        QFormDataBuilder builder;
        QFile image(u"../../pic.png"_s); image.open(QFile::ReadOnly);
        QFile mask(u"../../mask.png"_s); mask.open(QFile::ReadOnly);

        builder.part("image"_L1).setBodyDevice(&image, "the actual image");
        builder.part("mask"_L1).setBodyDevice(&mask, "the mask image");
        builder.part("prompt"_L1).setBody("Lobster wearing a beret");
        builder.part("n"_L1).setBody("2");
        builder.part("size"_L1).setBody("512x512");

        std::unique_ptr<QHttpMultiPart> mp = builder.buildMultiPart();
    \endcode

    \sa QHttpPart, QHttpMultiPart, QFormDataPartBuilder
*/

class QFormDataBuilderPrivate
{
public:
    std::vector<QFormDataPartBuilderPrivate> parts;
};

QFormDataPartBuilderPrivate* QFormDataPartBuilder::d_func()
{
    return const_cast<QFormDataPartBuilderPrivate*>(std::as_const(*this).d_func());
}

const QFormDataPartBuilderPrivate* QFormDataPartBuilder::d_func() const
{
    Q_ASSERT(m_index < d->parts.size());
    return &d->parts[m_index];
}

/*!
    Constructs an empty QFormDataBuilder object.
*/

QFormDataBuilder::QFormDataBuilder()
    : d_ptr(new QFormDataBuilderPrivate())
{

}

/*!
    Destroys the QFormDataBuilder object.
*/

QFormDataBuilder::~QFormDataBuilder()
{
    delete d_ptr;
}

/*!
    \fn QFormDataBuilder::QFormDataBuilder(QFormDataBuilder &&other) noexcept

    Move-constructs a QFormDataBuilder instance, making it point at the same
    object that \a other was pointing to.
*/

/*!
    \fn QFormDataBuilder &QFormDataBuilder::operator=(QFormDataBuilder &&other) noexcept

    Move-assigns \a other to this QFormDataBuilder instance.
*/

/*!
    Returns a newly-constructed QFormDataPartBuilder object using \a name as the
    form-data's \c name parameter. The object is valid for as long as the
    associated QFormDataBuilder has not been destroyed.

    Limiting \a name characters to US-ASCII is
    \l {https://datatracker.ietf.org/doc/html/rfc7578#section-5.1.1}{strongly recommended}
    for interoperability reasons.

    \sa QFormDataPartBuilder, QHttpPart
*/

QFormDataPartBuilder QFormDataBuilder::part(QAnyStringView name)
{
    Q_D(QFormDataBuilder);

    d->parts.emplace_back(name);
    return QFormDataPartBuilder(d, d->parts.size() - 1);
}

/*!
    Constructs and returns a pointer to a QHttpMultipart object.

    \sa QHttpMultiPart
*/

std::unique_ptr<QHttpMultiPart> QFormDataBuilder::buildMultiPart()
{
    Q_D(QFormDataBuilder);

    auto multiPart = std::make_unique<QHttpMultiPart>(QHttpMultiPart::FormDataType);

    for (auto &part : d->parts)
        multiPart->append(part.build());

    return multiPart;
}

QT_END_NAMESPACE
