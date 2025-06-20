// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformiconengine.h"

#ifndef QT_NO_ICON

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
Q_STATIC_LOGGING_CATEGORY(lcIconEngineFontDownload, "qt.qpa.iconengine.fontdownload")

// the primary types to work with the FontRequest API
Q_DECLARE_JNI_CLASS(FontRequest, "androidx/core/provider/FontRequest")
Q_DECLARE_JNI_CLASS(FontsContractCompat, "androidx/core/provider/FontsContractCompat")
Q_DECLARE_JNI_CLASS(FontFamilyResult, "androidx/core/provider/FontsContractCompat$FontFamilyResult")
Q_DECLARE_JNI_CLASS(FontInfo, "androidx/core/provider/FontsContractCompat$FontInfo")

// various utility types
Q_DECLARE_JNI_CLASS(List, "java/util/List"); // List is just an Interface
Q_DECLARE_JNI_CLASS(HashSet, "java/util/HashSet");
Q_DECLARE_JNI_CLASS(CancellationSignal, "android/os/CancellationSignal")
Q_DECLARE_JNI_CLASS(ParcelFileDescriptor, "android/os/ParcelFileDescriptor")
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

        for (const auto &signature : signatures) {
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
            if (!file.open(fd, QFile::OpenModeFlag::ReadOnly,
                           QFile::FileHandleFlag::AutoCloseHandle)) {
                qCWarning(lcIconEngineFontDownload, "Font file '%ls' cannot be opened: %ls",
                          qUtf16Printable(fontUri.toString()),
                          qUtf16Printable(file.errorString()));
                continue;
            }
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

static QFont selectFont()
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
    QFont font(fontFamily);
    font.setStyleStrategy(QFont::NoFontMerging);
    return font;
}

} // namespace FontProvider

static QString getGlyphs(QStringView iconName)
{
    static constexpr std::pair<QLatin1StringView, QStringView> glyphMap[] = {
        {"address-book-new"_L1, u"\ue0e0"},
        {"application-exit"_L1, u"\ue5cd"},
        {"appointment-new"_L1, u"\ue878"},
        {"call-start"_L1, u"\ue0b0"},
        {"call-stop"_L1, u"\ue0b1"},
        {"contact-new"_L1, u"\uf22e"},
        {"document-new"_L1, u"\ue89c"},
        {"document-open"_L1, u"\ue2c8"},
        {"document-open-recent"_L1, u"\ue4a7"},
        {"document-page-setup"_L1, u"\uf88c"},
        {"document-print"_L1, u"\ue8ad"},
        {"document-print-preview"_L1, u"\uefb2"},
        {"document-properties"_L1, u"\uf775"},
        {"document-revert"_L1, u"\ue929"},
        {"document-save"_L1, u"\ue161"},
        {"document-save-as"_L1, u"\ueb60"},
        {"document-send"_L1, u"\uf09b"},
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
        {"folder-new"_L1, u"\ue2cc"},
        {"format-indent-less"_L1, u"\ue23d"},
        {"format-indent-more"_L1, u"\ue23e"},
        {"format-justify-center"_L1, u"\ue234"},
        {"format-justify-fill"_L1, u"\ue235"},
        {"format-justify-left"_L1, u"\ue236"},
        {"format-justify-right"_L1, u"\ue237"},
        {"format-text-direction-ltr"_L1, u"\ue247"},
        {"format-text-direction-rtl"_L1, u"\ue248"},
        {"format-text-bold"_L1, u"\ue238"},
        {"format-text-italic"_L1, u"\ue23f"},
        {"format-text-underline"_L1, u"\ue249"},
        {"format-text-strikethrough"_L1, u"\ue246"},
        {"go-bottom"_L1,u"\ue258"},
        {"go-down"_L1,u"\uf1e3"},
        {"go-first"_L1, u"\ue5dc"},
        {"go-home"_L1, u"\ue88a"},
        {"go-jump"_L1, u"\uf719"},
        {"go-last"_L1, u"\ue5dd"},
        {"go-next"_L1, u"\ue5c8"},
        {"go-previous"_L1, u"\ue5c4"},
        {"go-top"_L1, u"\ue25a"},
        {"go-up"_L1, u"\uf1e0"},
        {"help-about"_L1, u"\ue88e"},
        {"help-contents"_L1, u"\ue8de"},
        {"help-faq"_L1, u"\uf04c"},
        {"insert-image"_L1, u"\ue43e"},
        {"insert-link"_L1, u"\ue178"},
        //{"insert-object"_L1, u"\u"},
        {"insert-text"_L1, u"\uf827"},
        {"list-add"_L1, u"\ue145"},
        {"list-remove"_L1, u"\ue15b"},
        {"mail-forward"_L1, u"\ue154"},
        {"mail-mark-important"_L1, u"\ue937"},
        //{"mail-mark-junk"_L1, u"\u"},
        //{"mail-mark-notjunk"_L1, u"\u"},
        {"mail-mark-read"_L1, u"\uf18c"},
        {"mail-mark-unread"_L1, u"\ue9bc"},
        {"mail-message-new"_L1, u"\ue3c9"},
        {"mail-reply-all"_L1, u"\ue15f"},
        {"mail-reply-sender"_L1, u"\ue15e"},
        {"mail-send"_L1, u"\ue163"},
        //{"mail-send-receive"_L1, u"\u"},
        {"media-eject"_L1, u"\ue8fb"},
        {"media-playback-pause"_L1, u"\ue034"},
        {"media-playback-start"_L1, u"\ue037"},
        {"media-playback-stop"_L1, u"\ue047"},
        {"media-record"_L1, u"\uf679"},
        {"media-seek-backward"_L1, u"\ue020"},
        {"media-seek-forward"_L1, u"\ue01f"},
        {"media-skip-backward"_L1, u"\ue045"},
        {"media-skip-forward"_L1, u"\ue044"},
        //{"object-flip-horizontal"_L1, u"\u"},
        //{"object-flip-vertical"_L1, u"\u"},
        {"object-rotate-left"_L1, u"\ue419"},
        {"object-rotate-right"_L1, u"\ue41a"},
        {"process-stop"_L1, u"\ue5c9"},
        {"system-lock-screen"_L1, u"\ue897"},
        {"system-log-out"_L1, u"\ue9ba"},
        //{"system-run"_L1, u"\u"},
        {"system-search"_L1, u"\uef70"},
        {"system-reboot"_L1, u"\uf053"},
        {"system-shutdown"_L1, u"\ue8ac"},
        {"tools-check-spelling"_L1, u"\ue8ce"},
        {"view-fullscreen"_L1, u"\ue5d0"},
        {"view-refresh"_L1, u"\ue5d5"},
        {"view-restore"_L1, u"\uf1cf"},
        {"view-sort-ascending"_L1, u"\ue25a"},
        {"view-sort-descending"_L1, u"\ue258"},
        {"window-close"_L1, u"\ue5cd"},
        {"window-new"_L1, u"\uf710"},
        {"zoom-fit-best"_L1, u"\uea10"},
        {"zoom-in"_L1, u"\ue8ff"},
        {"zoom-original"_L1, u"\ue5d1"},
        {"zoom-out"_L1, u"\ue900"},
        {"process-working"_L1, u"\uef64"},
        {"accessories-calculator"_L1, u"\uea5f"},
        {"accessories-character-map"_L1, u"\uf8a3"},
        {"accessories-dictionary"_L1, u"\uf539"},
        {"accessories-text-editor"_L1, u"\ue262"},
        {"help-browser"_L1, u"\ue887"},
        {"multimedia-volume-control"_L1, u"\ue050"},
        {"preferences-desktop-accessibility"_L1, u"\uf05d"},
        {"preferences-desktop-font"_L1, u"\ue165"},
        {"preferences-desktop-keyboard"_L1, u"\ue312"},
        //{"preferences-desktop-locale"_L1, u"\u"},
        {"preferences-desktop-multimedia"_L1, u"\uea75"},
        //{"preferences-desktop-screensaver"_L1, u"\u"},
        {"preferences-desktop-theme"_L1, u"\uf560"},
        {"preferences-desktop-wallpaper"_L1, u"\ue1bc"},
        {"system-file-manager"_L1, u"\ue2c7"},
        {"system-software-install"_L1, u"\ueb71"},
        {"system-software-update"_L1, u"\ue8d7"},
        {"utilities-system-monitor"_L1, u"\uef5b"},
        {"utilities-terminal"_L1, u"\ueb8e"},
        //{"applications-accessories"_L1, u"\u"},
        {"applications-development"_L1, u"\ue720"},
        {"applications-engineering"_L1, u"\uea3d"},
        {"applications-games"_L1, u"\uf135"},
        //{"applications-graphics"_L1, u"\u"},
        {"applications-internet"_L1, u"\ue80b"},
        {"applications-multimedia"_L1, u"\uf06a"},
        //{"applications-office"_L1, u"\u"},
        //{"applications-other"_L1, u"\u"},
        {"applications-science"_L1, u"\uea4b"},
        //{"applications-system"_L1, u"\u"},
        //{"applications-utilities"_L1, u"\u"},
        {"preferences-desktop"_L1, u"\ueb97"},
        //{"preferences-desktop-peripherals"_L1, u"\u"},
        {"preferences-desktop-personal"_L1, u"\uf835"},
        //{"preferences-other"_L1, u"\u"},
        {"preferences-system"_L1, u"\ue8b8"},
        {"preferences-system-network"_L1, u"\ue894"},
        {"system-help"_L1, u"\ue887"},
        //{"audio-card"_L1, u"\u"},
        {"audio-input-microphone"_L1, u"\ue029"},
        {"battery"_L1, u"\ue1a4"},
        {"camera-photo"_L1, u"\ue412"},
        {"camera-video"_L1, u"\ue04b"},
        {"camera-web"_L1, u"\uf7a6"},
        {"computer"_L1, u"\ue30a"},
        {"drive-harddisk"_L1, u"\uf80e"},
        {"drive-optical"_L1, u"\ue019"}, // same as media-optical
        //{"drive-removable-media"_L1, u"\u"},
        {"input-gaming"_L1, u"\uf5ee"},
        {"input-keyboard"_L1, u"\ue312"},
        {"input-mouse"_L1, u"\ue323"},
        //{"input-tablet"_L1, u"\u"},
        //{"media-flash"_L1, u"\u"},
        //{"media-floppy"_L1, u"\u"},
        {"media-optical"_L1, u"\ue019"},
        //{"media-tape"_L1, u"\u"},
        //{"modem"_L1, u"\u"},
        //{"multimedia-player"_L1, u"\u"},
        //{"network-wired"_L1, u"\u"},
        {"network-wireless"_L1, u"\ue63e"},
        //{"pda"_L1, u"\u"},
        {"phone"_L1, u"\ue32c"},
        {"printer"_L1, u"\ue8ad"},
        {"scanner"_L1, u"\ue329"},
        {"video-display"_L1, u"\uf06a"},
        //{"emblem-default"_L1, u"\u"},
        {"emblem-documents"_L1, u"\ue873"},
        {"emblem-downloads"_L1, u"\uf090"},
        {"emblem-favorite"_L1, u"\uf090"},
        {"emblem-important"_L1, u"\ue645"},
        {"emblem-mail"_L1, u"\ue158"},
        {"emblem-photos"_L1, u"\ue413"},
        //{"emblem-readonly"_L1, u"\u"},
        {"emblem-shared"_L1, u"\ue413"},
        //{"emblem-symbolic-link"_L1, u"\u"},
        //{"emblem-synchronized"_L1, u"\u"},
        {"emblem-system"_L1, u"\ue8b8"},
        //{"emblem-unreadable"_L1, u"\u"},
        {"folder"_L1, u"\ue2c7"},
        {"text-x-generic"_L1, u"\ue66d"},
        //{"folder-remote"_L1, u"\u"},
        {"network-server"_L1, u"\ue875"},
        {"network-workgroup"_L1, u"\ue1a0"},
        {"start-here"_L1, u"\ue089"},
        {"user-bookmarks"_L1, u"\ue98b"},
        {"user-desktop"_L1, u"\ue30a"},
        {"user-home"_L1, u"\ue88a"},
        {"user-trash"_L1, u"\ue872"},
        {"appointment-missed"_L1, u"\ue615"},
        {"appointment-soon"_L1, u"\uf540"},
        {"audio-volume-high"_L1, u"\ue050"},
        {"audio-volume-low"_L1, u"\ue04d"},
        //{"audio-volume-medium"_L1, u"\u"},
        {"audio-volume-muted"_L1, u"\ue04e"},
        {"battery-caution"_L1, u"\ue19c"},
        {"battery-low"_L1, u"\uf147"},
        {"dialog-error"_L1, u"\ue000"},
        {"dialog-information"_L1, u"\ue88e"},
        {"dialog-password"_L1, u"\uf042"},
        {"dialog-question"_L1, u"\ueb8b"},
        {"dialog-warning"_L1, u"\ue002"},
        {"folder-drag-accept"_L1, u"\ue9a3"},
        {"folder-open"_L1, u"\ue2c8"},
        {"folder-visiting"_L1, u"\ue8a7"},
        {"image-loading"_L1, u"\ue41a"},
        {"image-missing"_L1, u"\ue3ad"},
        {"mail-attachment"_L1, u"\ue2bc"},
        {"mail-unread"_L1, u"\uf18a"},
        {"mail-read"_L1, u"\uf18c"},
        //{"mail-replied"_L1, u"\u"},
        //{"mail-signed"_L1, u"\u"},
        //{"mail-signed-verified"_L1, u"\u"},
        {"media-playlist-repeat"_L1, u"\ue040"},
        {"media-playlist-shuffle"_L1, u"\ue043"},
        {"network-error"_L1, u"\uead9"},
        {"network-idle"_L1, u"\ue51f"},
        {"network-offline"_L1, u"\uf239"},
        {"network-receive"_L1, u"\ue2c0"},
        {"network-transmit"_L1, u"\ue2c3"},
        {"network-transmit-receive"_L1, u"\uea18"},
        {"printer-error"_L1, u"\uf7a0"},
        {"printer-printing"_L1, u"\uf7a1"},
        {"security-high"_L1, u"\ue32a"},
        {"security-medium"_L1, u"\ue9e0"},
        {"security-low"_L1, u"\uf012"},
        {"software-update-available"_L1, u"\ue923"},
        {"software-update-urgent"_L1, u"\uf05a"},
        {"sync-error"_L1, u"\ue629"},
        {"sync-synchronizing"_L1, u"\ue627"},
        //{"task-due"_L1, u"\u"},
        //{"task-past-due"_L1, u"\u"},
        {"user-available"_L1, u"\uf565"},
        {"user-away"_L1, u"\ue510"},
        //{"user-idle"_L1, u"\u"},
        {"user-offline"_L1, u"\uf7b3"},
        {"user-trash-full"_L1, u"\ue872"}, //delete
        //{"user-trash-full"_L1, u"\ue92b"}, //delete_forever
        {"weather-clear"_L1, u"\uf157"},
        {"weather-clear-night"_L1, u"\uf159"},
        {"weather-few-clouds"_L1, u"\uf172"},
        {"weather-few-clouds-night"_L1, u"\uf174"},
        {"weather-fog"_L1, u"\ue818"},
        //{"weather-overcast"_L1, u"\u"},
        {"weather-severe-alert"_L1, u"\ue002"}, //warning
        //{"weather-severe-alert"_L1, u"\uebd3"},//severe_cold
        {"weather-showers"_L1, u"\uf176"},
        //{"weather-showers-scattered"_L1, u"\u"},
        {"weather-snow"_L1, u"\ue80f"}, //snowing
        //{"weather-snow"_L1, u"\ue2cd"}, //weather_snowy
        //{"weather-snow"_L1, u"\ue810"},//cloudy_snowing
        {"weather-storm"_L1, u"\uf070"},
    };

    const auto it = std::find_if(std::begin(glyphMap),
                                 std::end(glyphMap), [iconName](const auto &c){
        return c.first == iconName;
    });
    return it != std::end(glyphMap) ? it->second.toString()
                                    : (iconName.length() == 1 ? iconName.toString() : QString());
}

QAndroidPlatformIconEngine::QAndroidPlatformIconEngine(const QString &iconName)
    : QFontIconEngine(iconName, FontProvider::selectFont())
    , m_glyphs(getGlyphs(iconName))
{
}

QAndroidPlatformIconEngine::~QAndroidPlatformIconEngine() = default;

QString QAndroidPlatformIconEngine::key() const
{
    return u"QAndroidPlatformIconEngine"_s;
}

QIconEngine *QAndroidPlatformIconEngine::clone() const
{
    QAndroidPlatformIconEngine *that = const_cast<QAndroidPlatformIconEngine *>(this);
    return new QAndroidPlatformIconEngine(that->iconName());
}

QString QAndroidPlatformIconEngine::string() const
{
    return m_glyphs;
}

QT_END_NAMESPACE

#endif // QT_NO_ICON
