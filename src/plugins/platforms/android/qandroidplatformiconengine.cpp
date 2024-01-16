// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformiconengine.h"
#include "androidjnimain.h"

#include <QtCore/qdebug.h>
#include <QtCore/qjniarray.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qfile.h>
#include <QtCore/qset.h>

#include <QtGui/qfontdatabase.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
Q_LOGGING_CATEGORY(lcIconEngineFontDownload, "qt.qpa.iconengine.fontdownload")

// the primary types to work with the FontRequest API
Q_DECLARE_JNI_CLASS(FontRequest, "androidx/core/provider/FontRequest")
Q_DECLARE_JNI_CLASS(FontsContractCompat, "androidx/core/provider/FontsContractCompat")
Q_DECLARE_JNI_CLASS(FontFamilyResult, "androidx/core/provider/FontsContractCompat$FontFamilyResult")
Q_DECLARE_JNI_CLASS(FontInfo, "androidx/core/provider/FontsContractCompat$FontInfo")

// various utility types
Q_DECLARE_JNI_CLASS(List, "java/util/List"); // List is just an Interface
Q_DECLARE_JNI_CLASS(ArrayList, "java/util/ArrayList");
Q_DECLARE_JNI_CLASS(HashSet, "java/util/HashSet");
Q_DECLARE_JNI_CLASS(Uri, "android/net/Uri")
Q_DECLARE_JNI_CLASS(CancellationSignal, "android/os/CancellationSignal")
Q_DECLARE_JNI_CLASS(ParcelFileDescriptor, "android/os/ParcelFileDescriptor")
Q_DECLARE_JNI_CLASS(ContentResolver, "android/content/ContentResolver")
Q_DECLARE_JNI_CLASS(PackageManager, "android/content/pm/PackageManager")
Q_DECLARE_JNI_CLASS(ProviderInfo, "android/content/pm/ProviderInfo")
Q_DECLARE_JNI_CLASS(PackageInfo, "android/content/pm/PackageInfo")
Q_DECLARE_JNI_CLASS(Signature, "android/content/pm/Signature")

namespace FontProvider {

static QString fetchFont(const QString &query)
{
    using namespace QtJniTypes;

    static QMap<QString, QString> triedFonts;
    const auto it = triedFonts.find(query);
    if (it != triedFonts.constEnd())
        return it.value();

    QString fontFamily;
    triedFonts[query] = fontFamily; // mark as tried

    QStringList loadedFamilies;
    if (QFile file(query); file.open(QIODevice::ReadOnly)) {
        qCDebug(lcIconEngineFontDownload) << "Loading font from resource" << query;
        const QByteArray fontData = file.readAll();
        int fontId = QFontDatabase::addApplicationFontFromData(fontData);
        loadedFamilies << QFontDatabase::applicationFontFamilies(fontId);
    } else if (!query.startsWith(u":/"_s)) {
        const QString package = u"com.google.android.gms"_s;
        const QString authority = u"com.google.android.gms.fonts"_s;

        // First we access the content provider to get the signatures of the authority for the package
        const auto context = QtAndroidPrivate::context();

        auto packageManager = context.callMethod<PackageManager>("getPackageManager");
        if (!packageManager.isValid()) {
            qCWarning(lcIconEngineFontDownload, "Failed to instantiate PackageManager");
            return fontFamily;
        }
        const int signaturesField = PackageManager::getStaticField<int>("GET_SIGNATURES");
        auto providerInfo = packageManager.callMethod<ProviderInfo>("resolveContentProvider",
                                                                    authority, 0);
        if (!providerInfo.isValid()) {
            qCWarning(lcIconEngineFontDownload, "Failed to resolve content provider");
            return fontFamily;
        }
        const QString packageName = providerInfo.getField<QString>("packageName");
        if (packageName != package) {
            qCWarning(lcIconEngineFontDownload, "Mismatched provider package - expected '%s', got '%s'",
                                                package.toUtf8().constData(), packageName.toUtf8().constData());
            return fontFamily;
        }
        auto packageInfo = packageManager.callMethod<PackageInfo>("getPackageInfo",
                                                        package, signaturesField);
        if (!packageInfo.isValid()) {
            qCWarning(lcIconEngineFontDownload, "Failed to get package info with signature field %d",
                                                signaturesField);
            return fontFamily;
        }
        const auto signatures = packageInfo.getField<Signature[]>("signatures");
        if (!signatures.isValid()) {
            qCWarning(lcIconEngineFontDownload, "Failed to get signature array from package info");
            return fontFamily;
        }

        // FontRequest wants a list of sets for the certificates
        ArrayList outerList;
        HashSet innerSet;
        Q_ASSERT(outerList.isValid() && innerSet.isValid());

        for (QJniObject signature : signatures) {
            const QJniArray<jbyte> byteArray = signature.callMethod<jbyte[]>("toByteArray");

            // add takes an Object, not an Array
            if (!innerSet.callMethod<jboolean>("add", byteArray.object<jobject>()))
                qCWarning(lcIconEngineFontDownload, "Failed to add signature to set");
        }
        // Add the set to the list
        if (!outerList.callMethod<jboolean>("add", innerSet.object()))
            qCWarning(lcIconEngineFontDownload, "Failed to add set to certificate list");

        // FontRequest constructor wants a List interface, not an ArrayList
        FontRequest fontRequest(authority, package, query, outerList.object<List>());
        if (!fontRequest.isValid()) {
            qCWarning(lcIconEngineFontDownload, "Failed to create font request for '%s'",
                                                query.toUtf8().constData());
            return fontFamily;
        }

        // Call FontsContractCompat::fetchFonts with the FontRequest object
        auto fontFamilyResult = FontsContractCompat::callStaticMethod<FontFamilyResult>(
                                                        "fetchFonts",
                                                        context,
                                                        CancellationSignal(nullptr),
                                                        fontRequest);
        if (!fontFamilyResult.isValid()) {
            qCWarning(lcIconEngineFontDownload, "Failed to fetch fonts for query '%s'",
                                                query.toUtf8().constData());
            return fontFamily;
        }

        enum class StatusCode {
            OK                          = 0,
            UNEXPECTED_DATA_PROVIDED    = 1,
            WRONG_CERTIFICATES          = 2,
        };

        const StatusCode statusCode = fontFamilyResult.callMethod<StatusCode>("getStatusCode");
        switch (statusCode) {
        case StatusCode::OK:
            break;
        case StatusCode::UNEXPECTED_DATA_PROVIDED:
            qCWarning(lcIconEngineFontDownload, "Provider returned unexpected data for query '%s'",
                                                query.toUtf8().constData());
            return fontFamily;
        case StatusCode::WRONG_CERTIFICATES:
            qCWarning(lcIconEngineFontDownload, "Wrong Certificates provided in query '%s'",
                                                query.toUtf8().constData());
            return fontFamily;
        }

        const auto fontInfos = fontFamilyResult.callMethod<FontInfo[]>("getFonts");
        if (!fontInfos.isValid()) {
            qCWarning(lcIconEngineFontDownload, "FontFamilyResult::getFonts returned null object for '%s'",
                                                query.toUtf8().constData());
            return fontFamily;
        }

        auto contentResolver = context.callMethod<ContentResolver>("getContentResolver");

        for (QJniObject fontInfo : fontInfos) {
            if (!fontInfo.isValid()) {
                qCDebug(lcIconEngineFontDownload, "Received null-fontInfo object, skipping");
                continue;
            }
            enum class ResultCode {
                OK                  = 0,
                FONT_NOT_FOUND      = 1,
                FONT_UNAVAILABLE    = 2,
                MALFORMED_QUERY     = 3,
            };
            const ResultCode resultCode = fontInfo.callMethod<ResultCode>("getResultCode");
            switch (resultCode) {
            case ResultCode::OK:
                break;
            case ResultCode::FONT_NOT_FOUND:
                qCWarning(lcIconEngineFontDownload, "Font '%s' could not be found",
                                                    query.toUtf8().constData());
                return fontFamily;
            case ResultCode::FONT_UNAVAILABLE:
                qCWarning(lcIconEngineFontDownload, "Font '%s' is unavailable at",
                                                    query.toUtf8().constData());
                return fontFamily;
            case ResultCode::MALFORMED_QUERY:
                qCWarning(lcIconEngineFontDownload, "Query string '%s' is malformed",
                                                    query.toUtf8().constData());
                return fontFamily;
            }
            auto fontUri = fontInfo.callMethod<Uri>("getUri");
            // in this case the Font URI is always a content scheme file, made
            // so the app requesting it has permissions to open
            auto fileDescriptor = contentResolver.callMethod<ParcelFileDescriptor>("openFileDescriptor",
                                                        fontUri, u"r"_s);
            if (!fileDescriptor.isValid()) {
                qCWarning(lcIconEngineFontDownload, "Font file '%s' not accessible",
                                                    fontUri.toString().toUtf8().constData());
                continue;
            }

            int fd = fileDescriptor.callMethod<int>("detachFd");
            QFile file;
            file.open(fd, QFile::OpenModeFlag::ReadOnly, QFile::FileHandleFlag::AutoCloseHandle);
            const QByteArray fontData = file.readAll();
            qCDebug(lcIconEngineFontDownload) << "Font file read:" << fontData.size() << "bytes";
            int fontId = QFontDatabase::addApplicationFontFromData(fontData);
            loadedFamilies << QFontDatabase::applicationFontFamilies(fontId);
        }
    }

    qCDebug(lcIconEngineFontDownload) << "Query '" << query << "' added families" << loadedFamilies;
    if (!loadedFamilies.isEmpty())
        fontFamily = loadedFamilies.first();
    triedFonts[query] = fontFamily;
    return fontFamily;
}
}

QString QAndroidPlatformIconEngine::glyphs() const
{
    if (!QFontInfo(m_iconFont).exactMatch())
        return {};

    static constexpr std::pair<QLatin1StringView, QStringView> glyphMap[] = {
        {"edit-clear"_L1, u"\ue872"},
        {"edit-copy"_L1, u"\ue14d"},
        {"edit-cut"_L1, u"\ue14e"},
        {"edit-delete"_L1, u"\ue14a"},
        {"edit-find"_L1, u"\ue8b6"},
        {"edit-find-replace"_L1, u"\ue881"},
        {"edit-paste"_L1, u"\ue14f"},
        {"edit-redo"_L1, u"\ue15a"},
        {"edit-select-all"_L1, u"\ue162"},
        {"edit-undo"_L1, u"\ue166"},
        {"printer"_L1, u"\ue8ad"},
        {"banana"_L1, u"🍌"},
    };

    const auto it = std::find_if(std::begin(glyphMap), std::end(glyphMap), [this](const auto &c){
        return c.first == m_iconName;
    });
    return it != std::end(glyphMap) ? it->second.toString()
                                    : (m_iconName.length() == 1 ? m_iconName : QString());
}

QAndroidPlatformIconEngine::QAndroidPlatformIconEngine(const QString &iconName)
    : m_iconName(iconName)
    , m_glyphs(glyphs())
{
    QString fontFamily;
    // The MaterialIcons-*.ttf and MaterialSymbols* font files are available from
    // https://github.com/google/material-design-icons/tree/master. If one of them is
    // packaged as a resource with the application, then we use it. We prioritize
    // a variable font.
    const QStringList fontCandidates = {
        "MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf",
        "MaterialSymbolsRounded[FILL,GRAD,opsz,wght].ttf",
        "MaterialSymbolsSharp[FILL,GRAD,opsz,wght].ttf",
        "MaterialIcons-Regular.ttf",
        "MaterialIconsOutlined-Regular.otf",
        "MaterialIconsRound-Regular.otf",
        "MaterialIconsSharp-Regular.otf",
        "MaterialIconsTwoTone-Regular.otf",
    };
    for (const auto &fontCandidate : fontCandidates) {
        fontFamily = FontProvider::fetchFont(u":/qt-project.org/icons/%1"_s.arg(fontCandidate));
        if (!fontFamily.isEmpty())
            break;
    }

    // Otherwise we try to download the Outlined version of Material Symbols
    const QString key = qEnvironmentVariable("QT_GOOGLE_FONTS_KEY");
    if (fontFamily.isEmpty() && !key.isEmpty())
        fontFamily = FontProvider::fetchFont(u"key=%1&name=Material+Symbols+Outlined"_s.arg(key));

    // last resort - use any Material Icons
    if (fontFamily.isEmpty())
        fontFamily = u"Material Icons"_s;
    m_iconFont = QFont(fontFamily);
}

QAndroidPlatformIconEngine::~QAndroidPlatformIconEngine()
{}

QIconEngine *QAndroidPlatformIconEngine::clone() const
{
    return new QAndroidPlatformIconEngine(m_iconName);
}

QString QAndroidPlatformIconEngine::key() const
{
    return u"QAndroidPlatformIconEngine"_s;
}

QString QAndroidPlatformIconEngine::iconName()
{
    return m_iconName;
}

bool QAndroidPlatformIconEngine::isNull()
{
    if (m_glyphs.isEmpty())
        return true;
    const QChar c0 = m_glyphs.at(0);
    const QFontMetrics fontMetrics(m_iconFont);
    if (c0.category() == QChar::Other_Surrogate && m_glyphs.size() > 1)
        return !fontMetrics.inFontUcs4(QChar::surrogateToUcs4(c0, m_glyphs.at(1)));
    return !fontMetrics.inFont(c0);
}

QList<QSize> QAndroidPlatformIconEngine::availableSizes(QIcon::Mode, QIcon::State)
{
    return {{16, 16}, {24, 24}, {48, 48}, {128, 128}};
}

QSize QAndroidPlatformIconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return QIconEngine::actualSize(size, mode, state);
}

QPixmap QAndroidPlatformIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

QPixmap QAndroidPlatformIconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    const quint64 cacheKey = calculateCacheKey(mode, state);
    if (cacheKey != m_cacheKey || m_pixmap.size() != size || m_pixmap.devicePixelRatio() != scale) {
        m_pixmap = QPixmap(size * scale);
        m_pixmap.fill(QColor(0, 0, 0, 0));
        m_pixmap.setDevicePixelRatio(scale);

        QPainter painter(&m_pixmap);
        QFont renderFont(m_iconFont);
        renderFont.setPixelSize(size.height());
        painter.setFont(renderFont);

        QPalette palette;
        switch (mode) {
        case QIcon::Active:
            painter.setPen(palette.color(QPalette::Active, QPalette::Accent));
            break;
        case QIcon::Normal:
            painter.setPen(palette.color(QPalette::Active, QPalette::Text));
            break;
        case QIcon::Disabled:
            painter.setPen(palette.color(QPalette::Disabled, QPalette::Accent));
            break;
        case QIcon::Selected:
            painter.setPen(palette.color(QPalette::Active, QPalette::Accent));
            break;
        }

        const QRect rect({0, 0}, size);
        painter.drawText(rect, Qt::AlignCenter, m_glyphs);

        m_cacheKey = cacheKey;
    }

    return m_pixmap;
}

void QAndroidPlatformIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    const qreal scale = painter->device()->devicePixelRatio();
    painter->drawPixmap(rect, scaledPixmap(rect.size(), mode, state, scale));
}

QT_END_NAMESPACE
