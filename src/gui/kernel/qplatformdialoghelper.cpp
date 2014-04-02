/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformdialoghelper.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QVariant>
#include <QtCore/QSharedData>
#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtGui/QColor>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformDialogHelper
    \since 5.0
    \internal
    \ingroup qpa

    \brief The QPlatformDialogHelper class allows for platform-specific customization of dialogs.

*/

static const int buttonRoleLayouts[2][5][14] =
{
    // Qt::Horizontal
    {
        // WinLayout
        { QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::Stretch, QPlatformDialogHelper::YesRole, QPlatformDialogHelper::AcceptRole,
          QPlatformDialogHelper::AlternateRole, QPlatformDialogHelper::DestructiveRole, QPlatformDialogHelper::NoRole,
          QPlatformDialogHelper::ActionRole, QPlatformDialogHelper::RejectRole, QPlatformDialogHelper::ApplyRole,
          QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL },

        // MacLayout
        { QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::ApplyRole, QPlatformDialogHelper::ActionRole,
          QPlatformDialogHelper::Stretch, QPlatformDialogHelper::DestructiveRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::AlternateRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::RejectRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::AcceptRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::NoRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::YesRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL },

        // KdeLayout
        { QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::Stretch, QPlatformDialogHelper::YesRole,
          QPlatformDialogHelper::NoRole, QPlatformDialogHelper::ActionRole, QPlatformDialogHelper::AcceptRole, QPlatformDialogHelper::AlternateRole,
          QPlatformDialogHelper::ApplyRole, QPlatformDialogHelper::DestructiveRole, QPlatformDialogHelper::RejectRole, QPlatformDialogHelper::EOL },

        // GnomeLayout
        { QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::Stretch, QPlatformDialogHelper::ActionRole,
          QPlatformDialogHelper::ApplyRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::DestructiveRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::AlternateRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::RejectRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::AcceptRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::NoRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::YesRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::EOL },

        // MacModelessLayout
        { QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::ApplyRole, QPlatformDialogHelper::ActionRole, QPlatformDialogHelper::Stretch,
          QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL,
          QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL }
    },

    // Qt::Vertical
    {
        // WinLayout
        { QPlatformDialogHelper::ActionRole, QPlatformDialogHelper::YesRole, QPlatformDialogHelper::AcceptRole, QPlatformDialogHelper::AlternateRole,
          QPlatformDialogHelper::DestructiveRole, QPlatformDialogHelper::NoRole, QPlatformDialogHelper::RejectRole, QPlatformDialogHelper::ApplyRole, QPlatformDialogHelper::ResetRole,
          QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::Stretch, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL },

        // MacLayout
        { QPlatformDialogHelper::YesRole, QPlatformDialogHelper::NoRole, QPlatformDialogHelper::AcceptRole, QPlatformDialogHelper::RejectRole,
          QPlatformDialogHelper::AlternateRole, QPlatformDialogHelper::DestructiveRole, QPlatformDialogHelper::Stretch, QPlatformDialogHelper::ActionRole, QPlatformDialogHelper::ApplyRole,
          QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL },

        // KdeLayout
        { QPlatformDialogHelper::AcceptRole, QPlatformDialogHelper::AlternateRole, QPlatformDialogHelper::ApplyRole, QPlatformDialogHelper::ActionRole,
          QPlatformDialogHelper::YesRole, QPlatformDialogHelper::NoRole, QPlatformDialogHelper::Stretch, QPlatformDialogHelper::ResetRole,
          QPlatformDialogHelper::DestructiveRole, QPlatformDialogHelper::RejectRole, QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::EOL },

        // GnomeLayout
        { QPlatformDialogHelper::YesRole, QPlatformDialogHelper::NoRole, QPlatformDialogHelper::AcceptRole, QPlatformDialogHelper::RejectRole,
          QPlatformDialogHelper::AlternateRole, QPlatformDialogHelper::DestructiveRole, QPlatformDialogHelper::ApplyRole, QPlatformDialogHelper::ActionRole, QPlatformDialogHelper::Stretch,
          QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL },

        // MacModelessLayout
        { QPlatformDialogHelper::ActionRole, QPlatformDialogHelper::ApplyRole, QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::Stretch,
          QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL,
          QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL }
    }
};

QPlatformDialogHelper::QPlatformDialogHelper()
{
    qRegisterMetaType<StandardButton>();
    qRegisterMetaType<ButtonRole>();
}

QPlatformDialogHelper::~QPlatformDialogHelper()
{
}

QVariant QPlatformDialogHelper::styleHint(StyleHint hint) const
{
    return QPlatformDialogHelper::defaultStyleHint(hint);
}

QVariant  QPlatformDialogHelper::defaultStyleHint(QPlatformDialogHelper::StyleHint hint)
{
    Q_UNUSED(hint);
    return QVariant();
}

// Font dialog

class QFontDialogOptionsPrivate : public QSharedData
{
public:
    QFontDialogOptionsPrivate() : options(0) {}

    QFontDialogOptions::FontDialogOptions options;
    QString windowTitle;
};

QFontDialogOptions::QFontDialogOptions(QFontDialogOptionsPrivate *dd)
    : d(dd)
{
}

QFontDialogOptions::~QFontDialogOptions()
{
}

namespace {
    struct FontDialogCombined : QFontDialogOptionsPrivate, QFontDialogOptions
    {
        FontDialogCombined() : QFontDialogOptionsPrivate(), QFontDialogOptions(this) {}
        FontDialogCombined(const FontDialogCombined &other)
            : QFontDialogOptionsPrivate(other), QFontDialogOptions(this) {}
    };
}

// static
QSharedPointer<QFontDialogOptions> QFontDialogOptions::create()
{
    return QSharedPointer<FontDialogCombined>::create();
}

QSharedPointer<QFontDialogOptions> QFontDialogOptions::clone() const
{
    return QSharedPointer<FontDialogCombined>::create(*static_cast<const FontDialogCombined*>(this));
}

QString QFontDialogOptions::windowTitle() const
{
    return d->windowTitle;
}

void QFontDialogOptions::setWindowTitle(const QString &title)
{
    d->windowTitle = title;
}

void QFontDialogOptions::setOption(QFontDialogOptions::FontDialogOption option, bool on)
{
    if (!(d->options & option) != !on)
        setOptions(d->options ^ option);
}

bool QFontDialogOptions::testOption(QFontDialogOptions::FontDialogOption option) const
{
    return d->options & option;
}

void QFontDialogOptions::setOptions(FontDialogOptions options)
{
    if (options != d->options)
        d->options = options;
}

QFontDialogOptions::FontDialogOptions QFontDialogOptions::options() const
{
    return d->options;
}

/*!
    \class QPlatformFontDialogHelper
    \since 5.0
    \internal
    \ingroup qpa

    \brief The QPlatformFontDialogHelper class allows for platform-specific customization of font dialogs.

*/
const QSharedPointer<QFontDialogOptions> &QPlatformFontDialogHelper::options() const
{
    return m_options;
}

void QPlatformFontDialogHelper::setOptions(const QSharedPointer<QFontDialogOptions> &options)
{
    m_options = options;
}

// Color dialog

class QColorDialogStaticData
{
public:
    enum { CustomColorCount = 16, StandardColorCount = 6 * 8 };

    QColorDialogStaticData();
    inline void readSettings();
    inline void writeSettings() const;

    QRgb customRgb[CustomColorCount];
    QRgb standardRgb[StandardColorCount];
    bool customSet;
};

QColorDialogStaticData::QColorDialogStaticData() : customSet(false)
{
    int i = 0;
    for (int g = 0; g < 4; ++g)
        for (int r = 0;  r < 4; ++r)
            for (int b = 0; b < 3; ++b)
                standardRgb[i++] = qRgb(r * 255 / 3, g * 255 / 3, b * 255 / 2);
    std::fill(customRgb, customRgb + CustomColorCount, 0xffffffff);
    readSettings();
}

void QColorDialogStaticData::readSettings()
{
#ifndef QT_NO_SETTINGS
    const QSettings settings(QSettings::UserScope, QStringLiteral("QtProject"));
    for (int i = 0; i < int(CustomColorCount); ++i) {
        const QVariant v = settings.value(QLatin1String("Qt/customColors/") + QString::number(i));
        if (v.isValid())
            customRgb[i] = v.toUInt();
    }
#endif
}

void QColorDialogStaticData::writeSettings() const
{
#ifndef QT_NO_SETTINGS
    if (!customSet) {
        QSettings settings(QSettings::UserScope, QStringLiteral("QtProject"));
        for (int i = 0; i < int(CustomColorCount); ++i)
            settings.setValue(QLatin1String("Qt/customColors/") + QString::number(i), customRgb[i]);
    }
#endif
}

Q_GLOBAL_STATIC(QColorDialogStaticData, qColorDialogStaticData)

class QColorDialogOptionsPrivate : public QSharedData
{
public:
    QColorDialogOptionsPrivate() : options(0) {}
    // Write out settings around destruction of dialogs
    ~QColorDialogOptionsPrivate() { qColorDialogStaticData()->writeSettings(); }

    QColorDialogOptions::ColorDialogOptions options;
    QString windowTitle;
};

QColorDialogOptions::QColorDialogOptions(QColorDialogOptionsPrivate *dd)
    : d(dd)
{
}

QColorDialogOptions::~QColorDialogOptions()
{
}

namespace {
    struct ColorDialogCombined : QColorDialogOptionsPrivate, QColorDialogOptions
    {
        ColorDialogCombined() : QColorDialogOptionsPrivate(), QColorDialogOptions(this) {}
        ColorDialogCombined(const ColorDialogCombined &other)
            : QColorDialogOptionsPrivate(other), QColorDialogOptions(this) {}
    };
}

// static
QSharedPointer<QColorDialogOptions> QColorDialogOptions::create()
{
    return QSharedPointer<ColorDialogCombined>::create();
}

QSharedPointer<QColorDialogOptions> QColorDialogOptions::clone() const
{
    return QSharedPointer<ColorDialogCombined>::create(*static_cast<const ColorDialogCombined*>(this));
}

QString QColorDialogOptions::windowTitle() const
{
    return d->windowTitle;
}

void QColorDialogOptions::setWindowTitle(const QString &title)
{
    d->windowTitle = title;
}

void QColorDialogOptions::setOption(QColorDialogOptions::ColorDialogOption option, bool on)
{
    if (!(d->options & option) != !on)
        setOptions(d->options ^ option);
}

bool QColorDialogOptions::testOption(QColorDialogOptions::ColorDialogOption option) const
{
    return d->options & option;
}

void QColorDialogOptions::setOptions(ColorDialogOptions options)
{
    if (options != d->options)
        d->options = options;
}

QColorDialogOptions::ColorDialogOptions QColorDialogOptions::options() const
{
    return d->options;
}

int QColorDialogOptions::customColorCount()
{
    return QColorDialogStaticData::CustomColorCount;
}

QRgb QColorDialogOptions::customColor(int index)
{
    if (uint(index) >= uint(QColorDialogStaticData::CustomColorCount))
        return qRgb(255, 255, 255);
    return qColorDialogStaticData()->customRgb[index];
}

QRgb *QColorDialogOptions::customColors()
{
    return qColorDialogStaticData()->customRgb;
}

void QColorDialogOptions::setCustomColor(int index, QRgb color)
{
    if (uint(index) >= uint(QColorDialogStaticData::CustomColorCount))
        return;
    qColorDialogStaticData()->customSet = true;
    qColorDialogStaticData()->customRgb[index] = color;
}

QRgb *QColorDialogOptions::standardColors()
{
    return qColorDialogStaticData()->standardRgb;
}

QRgb QColorDialogOptions::standardColor(int index)
{
    if (uint(index) >= uint(QColorDialogStaticData::StandardColorCount))
        return qRgb(255, 255, 255);
    return qColorDialogStaticData()->standardRgb[index];
}

void QColorDialogOptions::setStandardColor(int index, QRgb color)
{
    if (uint(index) >= uint(QColorDialogStaticData::StandardColorCount))
        return;
    qColorDialogStaticData()->standardRgb[index] = color;
}

/*!
    \class QPlatformColorDialogHelper
    \since 5.0
    \internal
    \ingroup qpa

    \brief The QPlatformColorDialogHelper class allows for platform-specific customization of color dialogs.

*/
const QSharedPointer<QColorDialogOptions> &QPlatformColorDialogHelper::options() const
{
    return m_options;
}

void QPlatformColorDialogHelper::setOptions(const QSharedPointer<QColorDialogOptions> &options)
{
    m_options = options;
}

// File dialog

class QFileDialogOptionsPrivate : public QSharedData
{
public:
    QFileDialogOptionsPrivate() : options(0),
        viewMode(QFileDialogOptions::Detail),
        fileMode(QFileDialogOptions::AnyFile),
        acceptMode(QFileDialogOptions::AcceptOpen),
        filters(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs),
        useDefaultNameFilters(true)
    {}

    QFileDialogOptions::FileDialogOptions options;
    QString windowTitle;

    QFileDialogOptions::ViewMode viewMode;
    QFileDialogOptions::FileMode fileMode;
    QFileDialogOptions::AcceptMode acceptMode;
    QString labels[QFileDialogOptions::DialogLabelCount];
    QDir::Filters filters;
    QList<QUrl> sidebarUrls;
    bool useDefaultNameFilters;
    QStringList nameFilters;
    QStringList mimeTypeFilters;
    QString defaultSuffix;
    QStringList history;
    QUrl initialDirectory;
    QString initiallySelectedNameFilter;
    QList<QUrl> initiallySelectedFiles;
    QStringList supportedSchemes;
};

QFileDialogOptions::QFileDialogOptions(QFileDialogOptionsPrivate *dd)
    : d(dd)
{
}

QFileDialogOptions::~QFileDialogOptions()
{
}

namespace {
    struct FileDialogCombined : QFileDialogOptionsPrivate, QFileDialogOptions
    {
        FileDialogCombined() : QFileDialogOptionsPrivate(), QFileDialogOptions(this) {}
        FileDialogCombined(const FileDialogCombined &other) : QFileDialogOptionsPrivate(other), QFileDialogOptions(this) {}
    };
}

// static
QSharedPointer<QFileDialogOptions> QFileDialogOptions::create()
{
    return QSharedPointer<FileDialogCombined>::create();
}

QSharedPointer<QFileDialogOptions> QFileDialogOptions::clone() const
{
    return QSharedPointer<FileDialogCombined>::create(*static_cast<const FileDialogCombined*>(this));
}

QString QFileDialogOptions::windowTitle() const
{
    return d->windowTitle;
}

void QFileDialogOptions::setWindowTitle(const QString &title)
{
    d->windowTitle = title;
}

void QFileDialogOptions::setOption(QFileDialogOptions::FileDialogOption option, bool on)
{
    if (!(d->options & option) != !on)
        setOptions(d->options ^ option);
}

bool QFileDialogOptions::testOption(QFileDialogOptions::FileDialogOption option) const
{
    return d->options & option;
}

void QFileDialogOptions::setOptions(FileDialogOptions options)
{
    if (options != d->options)
        d->options = options;
}

QFileDialogOptions::FileDialogOptions QFileDialogOptions::options() const
{
    return d->options;
}

QDir::Filters QFileDialogOptions::filter() const
{
    return d->filters;
}

void QFileDialogOptions::setFilter(QDir::Filters filters)
{
    d->filters  = filters;
}

void QFileDialogOptions::setViewMode(QFileDialogOptions::ViewMode mode)
{
    d->viewMode = mode;
}

QFileDialogOptions::ViewMode QFileDialogOptions::viewMode() const
{
    return d->viewMode;
}

void QFileDialogOptions::setFileMode(QFileDialogOptions::FileMode mode)
{
    d->fileMode = mode;
}

QFileDialogOptions::FileMode QFileDialogOptions::fileMode() const
{
    return d->fileMode;
}

void QFileDialogOptions::setAcceptMode(QFileDialogOptions::AcceptMode mode)
{
    d->acceptMode = mode;
}

QFileDialogOptions::AcceptMode QFileDialogOptions::acceptMode() const
{
    return d->acceptMode;
}

void QFileDialogOptions::setSidebarUrls(const QList<QUrl> &urls)
{
    d->sidebarUrls = urls;
}

QList<QUrl> QFileDialogOptions::sidebarUrls() const
{
    return d->sidebarUrls;
}

/*!
    \since 5.7
    \internal
    The bool property useDefaultNameFilters indicates that no name filters have been
    set or that they are equivalent to \gui{All Files (*)}. If it is true, the
    platform can choose to hide the filter combo box.

    \sa defaultNameFilterString().
*/
bool QFileDialogOptions::useDefaultNameFilters() const
{
    return d->useDefaultNameFilters;
}

void QFileDialogOptions::setUseDefaultNameFilters(bool dnf)
{
    d->useDefaultNameFilters = dnf;
}

void QFileDialogOptions::setNameFilters(const QStringList &filters)
{
    d->useDefaultNameFilters = filters.size() == 1
        && filters.first() == QFileDialogOptions::defaultNameFilterString();
    d->nameFilters = filters;
}

QStringList QFileDialogOptions::nameFilters() const
{
    return d->useDefaultNameFilters ?
        QStringList(QFileDialogOptions::defaultNameFilterString()) : d->nameFilters;
}

/*!
    \since 5.6
    \internal
    \return The translated default name filter string (\gui{All Files (*)}).
    \sa defaultNameFilters(), nameFilters()
*/

QString QFileDialogOptions::defaultNameFilterString()
{
    return QCoreApplication::translate("QFileDialog", "All Files (*)");
}

void QFileDialogOptions::setMimeTypeFilters(const QStringList &filters)
{
    d->mimeTypeFilters = filters;
}

QStringList QFileDialogOptions::mimeTypeFilters() const
{
    return d->mimeTypeFilters;
}

void QFileDialogOptions::setDefaultSuffix(const QString &suffix)
{
    d->defaultSuffix = suffix;
    if (d->defaultSuffix.size() > 1 && d->defaultSuffix.startsWith(QLatin1Char('.')))
        d->defaultSuffix.remove(0, 1); // Silently change ".txt" -> "txt".
}

QString QFileDialogOptions::defaultSuffix() const
{
    return d->defaultSuffix;
}

void QFileDialogOptions::setHistory(const QStringList &paths)
{
    d->history = paths;
}

QStringList QFileDialogOptions::history() const
{
    return d->history;
}

void QFileDialogOptions::setLabelText(QFileDialogOptions::DialogLabel label, const QString &text)
{
    if (label >= 0 && label < DialogLabelCount)
        d->labels[label] = text;
}

QString QFileDialogOptions::labelText(QFileDialogOptions::DialogLabel label) const
{
    return (label >= 0 && label < DialogLabelCount) ? d->labels[label] : QString();
}

bool QFileDialogOptions::isLabelExplicitlySet(DialogLabel label)
{
    return label >= 0 && label < DialogLabelCount && !d->labels[label].isEmpty();
}

QUrl QFileDialogOptions::initialDirectory() const
{
    return d->initialDirectory;
}

void QFileDialogOptions::setInitialDirectory(const QUrl &directory)
{
    d->initialDirectory = directory;
}

QString QFileDialogOptions::initiallySelectedNameFilter() const
{
    return d->initiallySelectedNameFilter;
}

void QFileDialogOptions::setInitiallySelectedNameFilter(const QString &filter)
{
    d->initiallySelectedNameFilter = filter;
}

QList<QUrl> QFileDialogOptions::initiallySelectedFiles() const
{
    return d->initiallySelectedFiles;
}

void QFileDialogOptions::setInitiallySelectedFiles(const QList<QUrl> &files)
{
    d->initiallySelectedFiles = files;
}

// Schemes supported by the application
void QFileDialogOptions::setSupportedSchemes(const QStringList &schemes)
{
    d->supportedSchemes = schemes;
}

QStringList QFileDialogOptions::supportedSchemes() const
{
    return d->supportedSchemes;
}

// Return true if the URL is supported by the filedialog implementation *and* by the application.
bool QPlatformFileDialogHelper::isSupportedUrl(const QUrl &url) const
{
    return url.isLocalFile();
}

/*!
    \class QPlatformFileDialogHelper
    \since 5.0
    \internal
    \ingroup qpa

    \brief The QPlatformFileDialogHelper class allows for platform-specific customization of file dialogs.

*/
const QSharedPointer<QFileDialogOptions> &QPlatformFileDialogHelper::options() const
{
    return m_options;
}

void QPlatformFileDialogHelper::setOptions(const QSharedPointer<QFileDialogOptions> &options)
{
    m_options = options;
}

const char *QPlatformFileDialogHelper::filterRegExp =
"^(.*)\\(([a-zA-Z0-9_.,*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$";

// Makes a list of filters from a normal filter string "Image Files (*.png *.jpg)"
QStringList QPlatformFileDialogHelper::cleanFilterList(const QString &filter)
{
    QRegExp regexp(QString::fromLatin1(filterRegExp));
    Q_ASSERT(regexp.isValid());
    QString f = filter;
    int i = regexp.indexIn(f);
    if (i >= 0)
        f = regexp.cap(2);
    return f.split(QLatin1Char(' '), QString::SkipEmptyParts);
}

// Message dialog

class QMessageDialogOptionsPrivate : public QSharedData
{
public:
    QMessageDialogOptionsPrivate() :
        icon(QMessageDialogOptions::NoIcon),
        buttons(QPlatformDialogHelper::Ok)
    {}

    QString windowTitle;
    QMessageDialogOptions::Icon icon;
    QString text;
    QString informativeText;
    QString detailedText;
    QPlatformDialogHelper::StandardButtons buttons;
};

QMessageDialogOptions::QMessageDialogOptions(QMessageDialogOptionsPrivate *dd)
    : d(dd)
{
}

QMessageDialogOptions::~QMessageDialogOptions()
{
}

namespace {
    struct MessageDialogCombined : QMessageDialogOptionsPrivate, QMessageDialogOptions
    {
        MessageDialogCombined() : QMessageDialogOptionsPrivate(), QMessageDialogOptions(this) {}
        MessageDialogCombined(const MessageDialogCombined &other)
            : QMessageDialogOptionsPrivate(other), QMessageDialogOptions(this) {}
    };
}

// static
QSharedPointer<QMessageDialogOptions> QMessageDialogOptions::create()
{
    return QSharedPointer<MessageDialogCombined>::create();
}

QSharedPointer<QMessageDialogOptions> QMessageDialogOptions::clone() const
{
    return QSharedPointer<MessageDialogCombined>::create(*static_cast<const MessageDialogCombined*>(this));
}

QString QMessageDialogOptions::windowTitle() const
{
    return d->windowTitle;
}

void QMessageDialogOptions::setWindowTitle(const QString &title)
{
    d->windowTitle = title;
}

QMessageDialogOptions::Icon QMessageDialogOptions::icon() const
{
    return d->icon;
}

void QMessageDialogOptions::setIcon(Icon icon)
{
    d->icon = icon;
}

QString QMessageDialogOptions::text() const
{
    return d->text;
}

void QMessageDialogOptions::setText(const QString &text)
{
    d->text = text;
}

QString QMessageDialogOptions::informativeText() const
{
    return d->informativeText;
}

void QMessageDialogOptions::setInformativeText(const QString &informativeText)
{
    d->informativeText = informativeText;
}

QString QMessageDialogOptions::detailedText() const
{
    return d->detailedText;
}

void QMessageDialogOptions::setDetailedText(const QString &detailedText)
{
    d->detailedText = detailedText;
}

void QMessageDialogOptions::setStandardButtons(QPlatformDialogHelper::StandardButtons buttons)
{
    d->buttons = buttons;
}

QPlatformDialogHelper::StandardButtons QMessageDialogOptions::standardButtons() const
{
    return d->buttons;
}

QPlatformDialogHelper::ButtonRole QPlatformDialogHelper::buttonRole(QPlatformDialogHelper::StandardButton button)
{
    switch (button) {
    case Ok:
    case Save:
    case Open:
    case SaveAll:
    case Retry:
    case Ignore:
        return AcceptRole;

    case Cancel:
    case Close:
    case Abort:
        return RejectRole;

    case Discard:
        return DestructiveRole;

    case Help:
        return HelpRole;

    case Apply:
        return ApplyRole;

    case Yes:
    case YesToAll:
        return YesRole;

    case No:
    case NoToAll:
        return NoRole;

    case RestoreDefaults:
    case Reset:
        return ResetRole;

    default:
        break;
    }
    return InvalidRole;
}

const int *QPlatformDialogHelper::buttonLayout(Qt::Orientation orientation, ButtonLayout policy)
{
    if (policy == UnknownLayout) {
#if defined (Q_OS_OSX)
        policy = MacLayout;
#elif defined (Q_OS_LINUX) || defined (Q_OS_UNIX)
        policy = KdeLayout;
#else
        policy = WinLayout;
#endif
    }
    return buttonRoleLayouts[orientation == Qt::Vertical][policy];
}

/*!
    \class QPlatformMessageDialogHelper
    \since 5.0
    \internal
    \ingroup qpa

    \brief The QPlatformMessageDialogHelper class allows for platform-specific customization of Message dialogs.

*/
const QSharedPointer<QMessageDialogOptions> &QPlatformMessageDialogHelper::options() const
{
    return m_options;
}

void QPlatformMessageDialogHelper::setOptions(const QSharedPointer<QMessageDialogOptions> &options)
{
    m_options = options;
}

QT_END_NAMESPACE
