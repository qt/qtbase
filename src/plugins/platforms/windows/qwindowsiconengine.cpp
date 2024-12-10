// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsiconengine.h"

#ifndef QT_NO_ICON

#include <QtCore/qoperatingsystemversion.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QString getGlyphs(QStringView iconName)
{
    static constexpr std::pair<QLatin1StringView, QStringView> glyphMap[] = {
        {"address-book-new"_L1, u"\ue780"},
        {"application-exit"_L1, u"\ue8bb"},
        {"appointment-new"_L1, u"\ue878"},
        {"call-start"_L1, u"\uf715"},
        {"call-stop"_L1, u"\uf405"},
        {"contact-new"_L1, u"\ue8fa"},
        {"document-new"_L1, u"\ue8a5"},
        {"document-open"_L1, u"\ue8e5"},
        {"document-open-recent"_L1, u"\ue823"},
        {"document-page-setup"_L1, u"\ue7c3"},
        {"document-print"_L1, u"\ue749"},
        {"document-print-preview"_L1, u"\ue956"},
        {"document-properties"_L1, u"\ue90f"},
        {"document-revert"_L1, u"\ue7a7"}, // ?
        {"document-save"_L1, u"\ue74e"}, // or e78c?
        {"document-save-as"_L1, u"\ue792"},
        {"document-send"_L1, u"\ue724"},
        {"edit-clear"_L1, u"\ue894"},
        {"edit-copy"_L1, u"\ue8c8"},
        {"edit-cut"_L1, u"\ue8c6"},
        {"edit-delete"_L1, u"\ue74d"},
        {"edit-find"_L1, u"\ue721"},
        //{"edit-find-replace"_L1, u"\u"},
        {"edit-paste"_L1, u"\ue77f"},
        {"edit-redo"_L1, u"\ue7a6"},
        {"edit-select-all"_L1, u"\ue8b3"},
        {"edit-undo"_L1, u"\ue7a7"},
        {"folder-new"_L1, u"\ue8f4"},
        //{"format-indent-less"_L1, u"\u"},
        //{"format-indent-more"_L1, u"\u"},
        {"format-justify-center"_L1, u"\ue8e3"},
        //{"format-justify-fill"_L1, u"\ue235"},
        {"format-justify-left"_L1, u"\ue8e4"},
        {"format-justify-right"_L1, u"\ue8e2"},
        {"format-text-direction-ltr"_L1, u"\ue9aa"},
        {"format-text-direction-rtl"_L1, u"\ue9ab"},
        {"format-text-bold"_L1, u"\ue8dd"},
        {"format-text-italic"_L1, u"\ue8db"},
        {"format-text-underline"_L1, u"\ue8dc"},
        {"format-text-strikethrough"_L1, u"\uede0"},
        //{"go-bottom"_L1,u"\ue258"},
        {"go-down"_L1,u"\ue74b"},
        //{"go-first"_L1, u"\ue5dc"},
        {"go-home"_L1, u"\ue80f"},
        // {"go-jump"_L1, u"\uf719"},
        //{"go-last"_L1, u"\ue5dd"},
        {"go-next"_L1, u"\ue893"},
        {"go-previous"_L1, u"\ue892"},
        //{"go-top"_L1, u"\ue25a"},
        {"go-up"_L1, u"\ue74a"},
        {"help-about"_L1, u"\ue946"},
        //{"help-contents"_L1, u"\ue8de"},
        {"help-faq"_L1, u"\ue897"},
        {"insert-image"_L1, u"\ue946"},
        {"insert-link"_L1, u"\ue71b"},
        //{"insert-object"_L1, u"\u"},
        //{"insert-text"_L1, u"\uf827"},
        {"list-add"_L1, u"\ue710"},
        {"list-remove"_L1, u"\ue738"},
        {"mail-forward"_L1, u"\ue89c"},
        //{"mail-mark-important"_L1, u"\ue937"},
        //{"mail-mark-junk"_L1, u"\u"},
        //{"mail-mark-notjunk"_L1, u"\u"},
        {"mail-mark-read"_L1, u"\ue8c3"},
        //{"mail-mark-unread"_L1, u"\ue9bc"},
        {"mail-message-new"_L1, u"\ue70f"},
        {"mail-reply-all"_L1, u"\ue8c2"},
        {"mail-reply-sender"_L1, u"\ue8ca"},
        {"mail-send"_L1, u"\ue724"},
        //{"mail-send-receive"_L1, u"\u"},
        {"media-eject"_L1, u"\uf847"},
        {"media-playback-pause"_L1, u"\ue769"},
        {"media-playback-start"_L1, u"\ue768"},
        {"media-playback-stop"_L1, u"\ue71a"},
        {"media-record"_L1, u"\ue7c8"},
        {"media-seek-backward"_L1, u"\ueb9e"},
        {"media-seek-forward"_L1, u"\ueb9d"},
        {"media-skip-backward"_L1, u"\ue892"},
        {"media-skip-forward"_L1, u"\ue893"},
        //{"object-flip-horizontal"_L1, u"\u"},
        //{"object-flip-vertical"_L1, u"\u"},
        {"object-rotate-left"_L1, u"\ue80c"},
        {"object-rotate-right"_L1, u"\ue80d"},
        //{"process-stop"_L1, u"\ue5c9"},
        {"system-lock-screen"_L1, u"\uee3f"},
        {"system-log-out"_L1, u"\uf3b1"},
        //{"system-run"_L1, u"\u"},
        {"system-search"_L1, u"\ue721"},
        {"system-reboot"_L1, u"\ue777"}, // unsure?
        {"system-shutdown"_L1, u"\ue7e8"},
        {"tools-check-spelling"_L1, u"\uf87b"},
        {"view-fullscreen"_L1, u"\ue740"},
        {"view-refresh"_L1, u"\ue72c"},
        {"view-restore"_L1, u"\ue777"},
        //{"view-sort-ascending"_L1, u"\ue25a"},
        //{"view-sort-descending"_L1, u"\ue258"},
        {"window-close"_L1, u"\ue8bb"},
        {"window-new"_L1, u"\ue78b"},
        {"zoom-fit-best"_L1, u"\ue9a6"},
        {"zoom-in"_L1, u"\ue8a3"},
        {"zoom-original"_L1, u"\ue71e"},
        {"zoom-out"_L1, u"\ue71f"},

        {"process-working"_L1, u"\ue9f3"},

        {"accessories-calculator"_L1, u"\ue8ef"},
        {"accessories-character-map"_L1, u"\uf2b7"},
        {"accessories-dictionary"_L1, u"\ue82d"},
        //{"accessories-text-editor"_L1, u"\ue262"},
        {"help-browser"_L1, u"\ue897"},
        {"multimedia-volume-control"_L1, u"\ue767"},
        {"preferences-desktop-accessibility"_L1, u"\ue776"},
        {"preferences-desktop-font"_L1, u"\ue8d2"},
        {"preferences-desktop-keyboard"_L1, u"\ue765"},
        {"preferences-desktop-locale"_L1, u"\uf2b7"},
        //{"preferences-desktop-multimedia"_L1, u"\uea75"},
        {"preferences-desktop-screensaver"_L1, u"\uf182"},
        //{"preferences-desktop-theme"_L1, u"\uf560"},
        //{"preferences-desktop-wallpaper"_L1, u"\ue1bc"},
        {"system-file-manager"_L1, u"\uec50"},
        //{"system-software-install"_L1, u"\ueb71"},
        {"system-software-update"_L1, u"\uecc5"},
        {"utilities-system-monitor"_L1, u"\ue7f4"},
        {"utilities-terminal"_L1, u"\ue756"},

        //{"applications-accessories"_L1, u"\u"},
        {"applications-development"_L1, u"\uec7a"},
        //{"applications-engineering"_L1, u"\uea3d"},
        {"applications-games"_L1, u"\ue7fc"},
        //{"applications-graphics"_L1, u"\u"},
        {"applications-internet"_L1, u"\ue774"},
        {"applications-multimedia"_L1, u"\uea69"},
        //{"applications-office"_L1, u"\u"},
        //{"applications-other"_L1, u"\u"},
        //{"applications-science"_L1, u"\uea4b"},
        {"applications-system"_L1, u"\ue770"},
        //{"applications-utilities"_L1, u"\u"},
        //{"preferences-desktop"_L1, u"\ueb97"},
        //{"preferences-desktop-peripherals"_L1, u"\u"},
        //{"preferences-desktop-personal"_L1, u"\uf835"},
        //{"preferences-other"_L1, u"\u"},
        //{"preferences-system"_L1, u"\ue8b8"},
        //{"preferences-system-network"_L1, u"\ue894"},
        {"system-help"_L1, u"\ue946"},

        {"audio-card"_L1, u"\ue8d6"},
        {"audio-input-microphone"_L1, u"\ue720"},
        {"battery"_L1, u"\ue83f"},
        {"camera-photo"_L1, u"\ue722"},
        {"camera-video"_L1, u"\ue714"},
        {"camera-web"_L1, u"\ue8b8"},
        {"computer"_L1, u"\ue7f8"}, // or e7fb?
        {"drive-harddisk"_L1, u"\ueda2"},
        {"drive-optical"_L1, u"\ue958"},
        //{"drive-removable-media"_L1, u"\u"},
        //{"input-gaming"_L1, u"\u"},
        {"input-keyboard"_L1, u"\ue92e"},
        {"input-mouse"_L1, u"\ue962"},
        {"input-tablet"_L1, u"\ue70a"},
        {"media-flash"_L1, u"\ue88e"},
        //{"media-floppy"_L1, u"\u"},
        {"media-optical"_L1, u"\ue958"},
        {"media-tape"_L1, u"\ue96a"},
        //{"modem"_L1, u"\u"},
        //{"multimedia-player"_L1, u"\u"},
        {"network-wired"_L1, u"\ue968"},
        {"network-wireless"_L1, u"\ue701"},
        //{"pda"_L1, u"\u"},
        {"phone"_L1, u"\ue717"},
        {"printer"_L1, u"\ue749"},
        {"scanner"_L1, u"\ue8fe"},
        //{"video-display"_L1, u"\uf06a"},

        {"emblem-default"_L1, u"\uf56d"},
        {"emblem-documents"_L1, u"\ue8a5"},
        {"emblem-downloads"_L1, u"\ue896"},
        {"emblem-favorite"_L1, u"\ue734"},
        {"emblem-important"_L1, u"\ue8c9"},
        {"emblem-mail"_L1, u"\ue715"},
        {"emblem-photos"_L1, u"\ue91b"},
        //{"emblem-readonly"_L1, u"\u"},
        {"emblem-shared"_L1, u"\ue902"},
        {"emblem-symbolic-link"_L1, u"\ue71b"},
        {"emblem-synchronized"_L1, u"\uedab"},
        {"emblem-system"_L1, u"\ue770"},
        //{"emblem-unreadable"_L1, u"\u"},

        {"folder"_L1, u"\ue8b7"},
        //{"folder-remote"_L1, u"\u"},
        //{"network-server"_L1, u"\ue875"},
        //{"network-workgroup"_L1, u"\ue1a0"},
        {"start-here"_L1, u"\ue8fc"}, // unsure
        {"user-bookmarks"_L1, u"\ue8a4"},
        //{"user-desktop"_L1, u"\ue30a"},
        {"user-home"_L1, u"\ue80f"},
        {"user-trash"_L1, u"\ue74d"},

        //{"appointment-missed"_L1, u"\ue615"},
        //{"appointment-soon"_L1, u"\uf540"},
        {"audio-volume-high"_L1, u"\ue995"},
        {"audio-volume-low"_L1, u"\ue993"},
        {"audio-volume-medium"_L1, u"\ue994"},
        {"audio-volume-muted"_L1, u"\ue992"},
        //{"battery-caution"_L1, u"\ue19c"},
        {"battery-low"_L1, u"\ue851"}, // ?
        {"dialog-error"_L1, u"\ue783"},
        {"dialog-information"_L1, u"\ue946"},
        //{"dialog-password"_L1, u"\uf042"},
        {"dialog-question"_L1, u"\uf142"}, // unsure
        {"dialog-warning"_L1, u"\ue7ba"},
        //{"folder-drag-accept"_L1, @u"\ue9a3"},
        {"folder-open"_L1, u"\ue838"},
        //{"folder-visiting"_L1, u"\ue8a7"},
        //{"image-loading"_L1, u"\ue41a"},
        //{"image-missing"_L1, u"\ue3ad"},
        {"mail-attachment"_L1, u"\ue723"},
        //{"mail-unread"_L1, u"\uf18a"},
        //{"mail-read"_L1, u"\uf18c"},
        {"mail-replied"_L1, u"\ue8ca"},
        //{"mail-signed"_L1, u"\u"},
        //{"mail-signed-verified"_L1, u"\u"},
        {"media-playlist-repeat"_L1, u"\ue8ee"},
        {"media-playlist-shuffle"_L1, u"\ue8b1"},
        //{"network-error"_L1, u"\uead9"},
        //{"network-idle"_L1, u"\ue51f"},
        {"network-offline"_L1, u"\uf384"},
        //{"network-receive"_L1, u"\ue2c0"},
        //{"network-transmit"_L1, u"\ue2c3"},
        //{"network-transmit-receive"_L1, u"\uca18"},
        //{"printer-error"_L1, u"\uf7a0"},
        //{"printer-printing"_L1, u"\uf7a1"},
        //{"security-high"_L1, u"\ue32a"},
        //{"security-medium"_L1, u"\ue9e0"},
        //{"security-low"_L1, u"\uf012"},
        //{"software-update-available"_L1, u"\ue923"},
        //{"software-update-urgent"_L1, u"\uf05a"},
        {"sync-error"_L1, u"\uea6a"},
        {"sync-synchronizing"_L1, u"\ue895"},
        //{"task-due"_L1, u"\u"},
        //{"task-past-due"_L1, u"\u"},
        {"user-available"_L1, u"\ue8cf"},
        //{"user-away"_L1, u"\ue510"},
        //{"user-idle"_L1, u"\u"},
        //{"user-offline"_L1, u"\uf7b3"},
        //{"user-trash-full"_L1, u"\ue872"}, //delete
        //{"user-trash-full"_L1, u"\ue92b"}, //delete_forever
        {"weather-clear"_L1, u"\ue706"},
        //{"weather-clear-night"_L1, u"\uf159"},
        //{"weather-few-clouds"_L1, u"\uf172"},
        //{"weather-few-clouds-night"_L1, u"\uf174"},
        //{"weather-fog"_L1, u"\ue818"},
        {"weather-overcast"_L1, u"\ue753"},
        //{"weather-severe-alert"_L1, u"\ue002"}, //warning
        //{"weather-showers"_L1, u"\uf176"},
        //{"weather-showers-scattered"_L1, u"\u"},
        //{"weather-snow"_L1, u"\ue80f"}, //snowing
        //{"weather-storm"_L1, u"\uf070"},
    };

    const auto it = std::find_if(std::begin(glyphMap),
                                 std::end(glyphMap), [iconName](const auto &c){
        return c.first == iconName;
    });

    return it != std::end(glyphMap) ? it->second.toString()
                                    : (iconName.length() == 1 ? iconName.toString() : QString());
}

namespace {
static auto iconFont()
{
    static const bool isWindows11 = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11;
    QFont font(isWindows11 ? u"Segoe Fluent Icons"_s
                           : u"Segoe MDL2 Assets"_s);
    font.setStyleStrategy(QFont::NoFontMerging);
    return font;
}
}

QWindowsIconEngine::QWindowsIconEngine(const QString &iconName)
    : QFontIconEngine(iconName, iconFont())
    , m_glyphs(getGlyphs(iconName))
{
}

QWindowsIconEngine::~QWindowsIconEngine()
{}

QString QWindowsIconEngine::key() const
{
    return u"QWindowsIconEngine"_s;
}

QIconEngine *QWindowsIconEngine::clone() const
{
    QWindowsIconEngine *that = const_cast<QWindowsIconEngine *>(this);
    return new QWindowsIconEngine(that->iconName());
}

QString QWindowsIconEngine::string() const
{
    return m_glyphs;
}

QT_END_NAMESPACE

#endif //QT_NO_ICON
