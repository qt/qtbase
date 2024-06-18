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

static QByteArray nameToByteArray(QStringView view)
{
    return view.toUtf8();
}

static QByteArray nameToByteArray(QLatin1StringView view)
{
    if (!QtPrivate::isAscii(view))
        return view.toString().toUtf8(); // ### optimize

    return QByteArray::fromRawData(view.data(), view.size());
}

static QByteArray nameToByteArray(QUtf8StringView view)
{
    return QByteArray::fromRawData(view.data(), view.size());
}

static void escapeNameAndAppend(QByteArray &dst, QByteArrayView src)
{
    for (auto c : src) {
        if (c == '"' || c == '\\')
            dst += '\\';
        dst += c;
    }
}

/*!
    Constructs a QFormDataPartBuilder object and sets \a name as the name
    parameter of the form-data.
*/
QFormDataPartBuilder::QFormDataPartBuilder(QAnyStringView name, PrivateConstructor /*unused*/)
{
    static_assert(std::is_nothrow_move_constructible_v<decltype(m_body)>);
    static_assert(std::is_nothrow_move_assignable_v<decltype(m_body)>);

    const auto enc = name.visit([](auto name) { return nameToByteArray(name); });

    m_headerValue += "form-data; name=\"";
    escapeNameAndAppend(m_headerValue, enc);
    m_headerValue += "\"";
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
    Destroys the QFormDataPartBuilder object.
*/

QFormDataPartBuilder::~QFormDataPartBuilder()
    = default;

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

QFormDataPartBuilder &QFormDataPartBuilder::setBodyHelper(const QByteArray &data,
                                                          QAnyStringView name,
                                                          QAnyStringView mimeType)
{
    m_originalBodyName = name.toString();
    convertInto(m_mimeType, mimeType);
    m_body = data;
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

QFormDataPartBuilder &QFormDataPartBuilder::setBody(QByteArrayView data,
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

    \sa setBody(), QHttpPart::setBodyDevice()
  */

QFormDataPartBuilder &QFormDataPartBuilder::setBodyDevice(QIODevice *body, QAnyStringView fileName,
                                                          QAnyStringView mimeType)
{
    m_originalBodyName = fileName.toString();
    convertInto(m_mimeType, mimeType);
    m_body = body;
    return *this;
}

/*!
    Sets the headers specified in \a headers.

    \note The "content-type" and "content-disposition" headers, if any are
    specified in \a headers, will be overwritten by the class.
*/

QFormDataPartBuilder &QFormDataPartBuilder::setHeaders(const QHttpHeaders &headers)
{
    m_httpHeaders = headers;
    return *this;
}

/*!
    \internal

    Generates a QHttpPart and sets the content disposition header as form-data.

    When this function called, it uses the MIME database to deduce the type the
    body based on its name and then sets the deduced type as the content type
    header.
*/

QHttpPart QFormDataPartBuilder::build()
{
    QHttpPart httpPart;

    if (!m_originalBodyName.isNull()) {
        const bool utf8 = !QtPrivate::isAscii(m_originalBodyName);
        const auto enc = utf8 ? m_originalBodyName.toUtf8() : m_originalBodyName.toLatin1();
        m_headerValue += "; filename=\"";
        escapeNameAndAppend(m_headerValue, enc);
        m_headerValue += "\"";
        if (utf8) {
            // For 'filename*' production see
            // https://datatracker.ietf.org/doc/html/rfc5987#section-3.2.1
            // For providing both filename and filename* parameters see
            // https://datatracker.ietf.org/doc/html/rfc6266#section-4.3 and
            // https://datatracker.ietf.org/doc/html/rfc8187#section-4.2
            m_headerValue += "; filename*=UTF-8''" + enc.toPercentEncoding();
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

    httpPart.setHeader(QNetworkRequest::ContentDispositionHeader, m_headerValue);

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

        std::unique_ptr<QHttpMultiPart> = builder.buildMultiPart();
    \endcode

    \sa QHttpPart, QHttpMultiPart, QFormDataPartBuilder
*/

class QFormDataBuilderPrivate
{
public:
    std::vector<QFormDataPartBuilder> parts;
};

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
    Constructs and returns a reference to a QFormDataPartBuilder object and sets
    \a name as the name parameter of the form-data. The returned reference is
    valid until the next call to this function.

    Limiting \a name characters to US-ASCII is
    \l {https://datatracker.ietf.org/doc/html/rfc7578#section-5.1.1}{strongly recommended}
    for interoperability reasons.

    \sa QFormDataPartBuilder, QHttpPart
*/

QFormDataPartBuilder &QFormDataBuilder::part(QAnyStringView name)
{
    Q_D(QFormDataBuilder);

    return d->parts.emplace_back(name, QFormDataPartBuilder::PrivateConstructor());
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
