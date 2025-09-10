// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlocalfileapi_p.h"
#include <private/qstdweb_p.h>
#include <QtCore/QRegularExpression>

QT_BEGIN_NAMESPACE
namespace LocalFileApi {
namespace {
std::string qtFilterListToFileInputAccept(const QStringList &filterList)
{
    QStringList transformed;
    for (const auto &filter : filterList) {
        const auto type = Type::fromQt(filter);
        if (type && type->accept()) {
            const auto &extensions = type->accept()->mimeType().extensions();
            std::transform(extensions.begin(), extensions.end(), std::back_inserter(transformed),
                           [](const Type::Accept::MimeType::Extension &extension) {
                               return extension.value().toString();
                           });
        }
    }
    return transformed.join(QStringLiteral(",")).toStdString();
}

std::optional<emscripten::val> qtFilterListToTypes(const QStringList &filterList)
{
    using namespace qstdweb;
    using namespace emscripten;
    auto types = emscripten::val::array();

    for (const auto &fileFilter : filterList) {
        auto type = Type::fromQt(fileFilter);
        if (type) {
            auto jsType = emscripten::val::object();
            jsType.set("description", type->description().toString().toStdString());
            if (type->accept()) {
                jsType.set("accept", ([&mimeType = type->accept()->mimeType()]() {
                               val acceptDict = val::object();

                               QList<emscripten::val> extensions;
                               extensions.reserve(mimeType.extensions().size());
                               std::transform(
                                       mimeType.extensions().begin(), mimeType.extensions().end(),
                                       std::back_inserter(extensions),
                                       [](const Type::Accept::MimeType::Extension &extension) {
                                           return val(extension.value().toString().toStdString());
                                       });
                               acceptDict.set("application/octet-stream",
                                              emscripten::val::array(extensions.begin(),
                                                                     extensions.end()));
                               return acceptDict;
                           })());
            }
            types.call<void>("push", std::move(jsType));
        }
    }

    return types["length"].as<int>() == 0 ? std::optional<emscripten::val>() : types;
}
} // namespace

Type::Type(QStringView description, std::optional<Accept> accept)
    : m_description(description.trimmed()), m_accept(std::move(accept))
{
}

Type::~Type() = default;

std::optional<Type> Type::fromQt(QStringView type)
{
    using namespace emscripten;

    // Accepts either a string in format:
    // GROUP3
    // or in this format:
    // GROUP1 (GROUP2)
    // Group 1 is treated as the description, whereas group 2 or 3 are treated as the filter list.
    static QRegularExpression regex(
            QString(QStringLiteral("(?:(?:([^(]*)\\(([^()]+)\\)[^)]*)|([^()]+))")));
    const auto match = regex.matchView(type);

    if (!match.hasMatch())
        return std::nullopt;

    constexpr size_t DescriptionIndex = 1;
    constexpr size_t FilterListFromParensIndex = 2;
    constexpr size_t PlainFilterListIndex = 3;

    const auto description = match.hasCaptured(DescriptionIndex)
            ? match.capturedView(DescriptionIndex)
            : QStringView();
    const auto filterList = match.capturedView(match.hasCaptured(FilterListFromParensIndex)
                                                       ? FilterListFromParensIndex
                                                       : PlainFilterListIndex);

    auto accept = Type::Accept::fromQt(filterList);
    if (!accept)
        return std::nullopt;

    return Type(description, std::move(*accept));
}

Type::Accept::Accept() = default;

Type::Accept::~Accept() = default;

std::optional<Type::Accept> Type::Accept::fromQt(QStringView qtRepresentation)
{
    Accept accept;

    // Used for accepting multiple extension specifications on a filter list.
    // The next group of non-empty characters.
    static QRegularExpression internalRegex(QString(QStringLiteral("([^\\s]+)\\s*")));
    int offset = 0;
    auto internalMatch = internalRegex.matchView(qtRepresentation, offset);
    MimeType mimeType;

    while (internalMatch.hasMatch()) {
        auto webExtension = MimeType::Extension::fromQt(internalMatch.capturedView(1));

        if (!webExtension)
            return std::nullopt;

        mimeType.addExtension(*webExtension);

        internalMatch = internalRegex.matchView(qtRepresentation, internalMatch.capturedEnd());
    }

    accept.setMimeType(mimeType);
    return accept;
}

void Type::Accept::setMimeType(MimeType mimeType)
{
    m_mimeType = std::move(mimeType);
}

Type::Accept::MimeType::MimeType() = default;

Type::Accept::MimeType::~MimeType() = default;

void Type::Accept::MimeType::addExtension(Extension extension)
{
    m_extensions.push_back(std::move(extension));
}

Type::Accept::MimeType::Extension::Extension(QStringView extension) : m_value(extension) { }

Type::Accept::MimeType::Extension::~Extension() = default;

std::optional<Type::Accept::MimeType::Extension>
Type::Accept::MimeType::Extension::fromQt(QStringView qtRepresentation)
{
    // Checks for a filter that matches everything:
    // Any number of asterisks or any number of asterisks with a '.' between them.
    // The web filter does not support wildcards.
    static QRegularExpression qtAcceptAllRegex(
            QRegularExpression::anchoredPattern(QString(QStringLiteral("[*]+|[*]+\\.[*]+"))));
    if (qtAcceptAllRegex.matchView(qtRepresentation).hasMatch())
        return std::nullopt;

    // Checks for correctness. The web filter only allows filename extensions and does not filter
    // the actual filenames, therefore we check whether the filter provided only filters for the
    // extension.
    static QRegularExpression qtFilenameMatcherRegex(
            QRegularExpression::anchoredPattern(QString(QStringLiteral("(\\*?)(\\.[^*]+)"))));

    auto extensionMatch = qtFilenameMatcherRegex.matchView(qtRepresentation);
    if (extensionMatch.hasMatch())
        return Extension(extensionMatch.capturedView(2));

    // Mapping impossible.
    return std::nullopt;
}

emscripten::val makeOpenFileOptions(const QStringList &filterList, bool acceptMultiple)
{
    auto options = emscripten::val::object();
    if (auto typeList = qtFilterListToTypes(filterList); typeList) {
        options.set("types", std::move(*typeList));
        options.set("excludeAcceptAllOption", true);
    }

    options.set("multiple", acceptMultiple);

    return options;
}

emscripten::val makeSaveFileOptions(const QStringList &filterList, const std::string& suggestedName)
{
    auto options = emscripten::val::object();

    if (!suggestedName.empty())
        options.set("suggestedName", emscripten::val(suggestedName));

    if (auto typeList = qtFilterListToTypes(filterList))
        options.set("types", emscripten::val(std::move(*typeList)));

    return options;
}

std::string makeFileInputAccept(const QStringList &filterList)
{
    return qtFilterListToFileInputAccept(filterList);
}

} // namespace LocalFileApi

QT_END_NAMESPACE
