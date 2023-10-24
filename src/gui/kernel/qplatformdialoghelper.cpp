// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdialoghelper.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#if QT_CONFIG(regularexpression)
#include <QtCore/QRegularExpression>
#endif
#if QT_CONFIG(settings)
#include <QtCore/QSettings>
#endif
#include <QtCore/QSharedData>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QPixmap>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QT_IMPL_METATYPE_EXTERN_TAGGED(QPlatformDialogHelper::StandardButton,
                               QPlatformDialogHelper__StandardButton)
QT_IMPL_METATYPE_EXTERN_TAGGED(QPlatformDialogHelper::ButtonRole,
                               QPlatformDialogHelper__ButtonRole)

/*!
    \class QPlatformDialogHelper
    \since 5.0
    \internal
    \ingroup qpa

    \brief The QPlatformDialogHelper class allows for platform-specific customization of dialogs.

*/

/*!
    \enum QPlatformDialogHelper::StyleHint

    This enum type specifies platform-specific style hints.

    \value DialogIsQtWindow Indicates that a platform-specific dialog is implemented
                            as in-process Qt window. It allows to prevent blocking the
                            dialog by an invisible proxy Qt dialog.

    \sa styleHint()
*/

static const int buttonRoleLayouts[2][6][14] =
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

          // AndroidLayout (neutral, stretch, dismissive, affirmative)
          // https://material.io/guidelines/components/dialogs.html#dialogs-specs
        { QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::ApplyRole, QPlatformDialogHelper::ActionRole,
          QPlatformDialogHelper::Stretch, QPlatformDialogHelper::RejectRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::NoRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::DestructiveRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::AlternateRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::AcceptRole | QPlatformDialogHelper::Reverse,
          QPlatformDialogHelper::YesRole | QPlatformDialogHelper::Reverse, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL }
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

          // AndroidLayout
          // (affirmative
          //  dismissive
          //  neutral)
          // https://material.io/guidelines/components/dialogs.html#dialogs-specs
        { QPlatformDialogHelper::YesRole, QPlatformDialogHelper::AcceptRole, QPlatformDialogHelper::AlternateRole, QPlatformDialogHelper::DestructiveRole,
          QPlatformDialogHelper::NoRole, QPlatformDialogHelper::RejectRole, QPlatformDialogHelper::Stretch, QPlatformDialogHelper::ActionRole, QPlatformDialogHelper::ApplyRole,
          QPlatformDialogHelper::ResetRole, QPlatformDialogHelper::HelpRole, QPlatformDialogHelper::EOL, QPlatformDialogHelper::EOL }
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
    QFontDialogOptionsPrivate() = default;

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
#if QT_CONFIG(settings)
    const QSettings settings(QSettings::UserScope, QStringLiteral("QtProject"));
    for (int i = 0; i < int(CustomColorCount); ++i) {
        const QVariant v = settings.value("Qt/customColors/"_L1 + QString::number(i));
        if (v.isValid())
            customRgb[i] = v.toUInt();
    }
#endif
}

void QColorDialogStaticData::writeSettings() const
{
#if QT_CONFIG(settings)
    if (customSet) {
        const_cast<QColorDialogStaticData*>(this)->customSet = false;
        QSettings settings(QSettings::UserScope, QStringLiteral("QtProject"));
        for (int i = 0; i < int(CustomColorCount); ++i)
            settings.setValue("Qt/customColors/"_L1 + QString::number(i), customRgb[i]);
    }
#endif
}

Q_GLOBAL_STATIC(QColorDialogStaticData, qColorDialogStaticData)

class QColorDialogOptionsPrivate : public QSharedData
{
public:
    QColorDialogOptionsPrivate() = default;
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
    QFileDialogOptions::FileDialogOptions options;
    QString windowTitle;

    QFileDialogOptions::ViewMode viewMode = QFileDialogOptions::Detail;
    QFileDialogOptions::FileMode fileMode = QFileDialogOptions::AnyFile;
    QFileDialogOptions::AcceptMode acceptMode = QFileDialogOptions::AcceptOpen;
    QString labels[QFileDialogOptions::DialogLabelCount];
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs;
    QList<QUrl> sidebarUrls;
    bool useDefaultNameFilters = true;
    QStringList nameFilters;
    QStringList mimeTypeFilters;
    QString defaultSuffix;
    QStringList history;
    QUrl initialDirectory;
    QString initiallySelectedMimeTypeFilter;
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
    if (d->defaultSuffix.size() > 1 && d->defaultSuffix.startsWith(u'.'))
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
    if (unsigned(label) < unsigned(DialogLabelCount))
        d->labels[label] = text;
}

QString QFileDialogOptions::labelText(QFileDialogOptions::DialogLabel label) const
{
    return (unsigned(label) < unsigned(DialogLabelCount)) ? d->labels[label] : QString();
}

bool QFileDialogOptions::isLabelExplicitlySet(DialogLabel label)
{
    return unsigned(label) < unsigned(DialogLabelCount) && !d->labels[label].isEmpty();
}

QUrl QFileDialogOptions::initialDirectory() const
{
    return d->initialDirectory;
}

void QFileDialogOptions::setInitialDirectory(const QUrl &directory)
{
    d->initialDirectory = directory;
}

QString QFileDialogOptions::initiallySelectedMimeTypeFilter() const
{
    return d->initiallySelectedMimeTypeFilter;
}

void QFileDialogOptions::setInitiallySelectedMimeTypeFilter(const QString &filter)
{
    d->initiallySelectedMimeTypeFilter = filter;
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

void QPlatformFileDialogHelper::selectMimeTypeFilter(const QString &filter)
{
    Q_UNUSED(filter);
}

QString QPlatformFileDialogHelper::selectedMimeTypeFilter() const
{
    return QString();
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

const char QPlatformFileDialogHelper::filterRegExp[] =
"^(.*)\\(([a-zA-Z0-9_.,*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$";

// Makes a list of filters from a normal filter string "Image Files (*.png *.jpg)"
QStringList QPlatformFileDialogHelper::cleanFilterList(const QString &filter)
{
#if QT_CONFIG(regularexpression)
    static const QRegularExpression regexp(QString::fromLatin1(filterRegExp));
    Q_ASSERT(regexp.isValid());
    QString f = filter;
    QRegularExpressionMatch match = regexp.match(filter);
    if (match.hasMatch())
        f = match.captured(2);
    return f.split(u' ', Qt::SkipEmptyParts);
#else
    Q_UNUSED(filter);
    return QStringList();
#endif
}

// Message dialog

class QMessageDialogOptionsPrivate : public QSharedData
{
public:
    QMessageDialogOptionsPrivate() :
        icon(QMessageDialogOptions::NoIcon),
        buttons(QPlatformDialogHelper::Ok),
        nextCustomButtonId(QPlatformDialogHelper::LastButton + 1)
    {}

    QString windowTitle;
    QMessageDialogOptions::StandardIcon icon;
    QString text;
    QString informativeText;
    QString detailedText;
    QPlatformDialogHelper::StandardButtons buttons;
    QList<QMessageDialogOptions::CustomButton> customButtons;
    int nextCustomButtonId;
    QPixmap iconPixmap;
    QString checkBoxLabel;
    Qt::CheckState checkBoxState = Qt::Unchecked;
    int defaultButtonId = 0;
    int escapeButtonId = 0;
    QMessageDialogOptions::Options options;
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

QMessageDialogOptions::StandardIcon QMessageDialogOptions::standardIcon() const
{
    return d->icon;
}

void QMessageDialogOptions::setStandardIcon(StandardIcon icon)
{
    d->icon = icon;
}

void QMessageDialogOptions::setIconPixmap(const QPixmap &pixmap)
{
    d->iconPixmap = pixmap;
}

QPixmap QMessageDialogOptions::iconPixmap() const
{
    return d->iconPixmap;
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

int QMessageDialogOptions::addButton(const QString &label, QPlatformDialogHelper::ButtonRole role,
                                     void *buttonImpl, int buttonId)
{
    const CustomButton b(buttonId ? buttonId : d->nextCustomButtonId++, label, role, buttonImpl);
    d->customButtons.append(b);
    return b.id;
}

static inline bool operator==(const QMessageDialogOptions::CustomButton &a,
                              const QMessageDialogOptions::CustomButton &b) {
    return a.id == b.id;
}

void QMessageDialogOptions::removeButton(int id)
{
    d->customButtons.removeOne(CustomButton(id));
}

const QList<QMessageDialogOptions::CustomButton> &QMessageDialogOptions::customButtons()
{
    return d->customButtons;
}

void QMessageDialogOptions::clearCustomButtons()
{
    d->customButtons.clear();
}

const QMessageDialogOptions::CustomButton *QMessageDialogOptions::customButton(int id)
{
    const int i = int(d->customButtons.indexOf(CustomButton(id)));
    return (i < 0 ? nullptr : &d->customButtons.at(i));
}

void QMessageDialogOptions::setCheckBox(const QString &label, Qt::CheckState state)
{
    d->checkBoxLabel = label;
    d->checkBoxState = state;
}

QString QMessageDialogOptions::checkBoxLabel() const
{
    return d->checkBoxLabel;
}

Qt::CheckState QMessageDialogOptions::checkBoxState() const
{
    return d->checkBoxState;
}

void QMessageDialogOptions::setDefaultButton(int id)
{
    d->defaultButtonId = id;
}

int QMessageDialogOptions::defaultButton() const
{
    return d->defaultButtonId;
}

void QMessageDialogOptions::setEscapeButton(int id)
{
    d->escapeButtonId = id;
}

int QMessageDialogOptions::escapeButton() const
{
    return d->escapeButtonId;
}

void QMessageDialogOptions::setOption(QMessageDialogOptions::Option option, bool on)
{
    if (!(d->options & option) != !on)
        setOptions(d->options ^ option);
}

bool QMessageDialogOptions::testOption(QMessageDialogOptions::Option option) const
{
    return d->options & option;
}

void QMessageDialogOptions::setOptions(QMessageDialogOptions::Options options)
{
    if (options != d->options)
        d->options = options;
}

QMessageDialogOptions::Options QMessageDialogOptions::options() const
{
    return d->options;
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
#if defined (Q_OS_MACOS)
        policy = MacLayout;
#elif defined (Q_OS_LINUX) || defined (Q_OS_UNIX)
        policy = KdeLayout;
#elif defined (Q_OS_ANDROID)
        policy = AndroidLayout;
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

#include "moc_qplatformdialoghelper.cpp"
