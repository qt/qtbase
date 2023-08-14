// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QT_NO_ICON
#include <private/qiconloader_p.h>

#include <private/qguiapplication_p.h>
#include <private/qicon_p.h>

#include <QtGui/QIconEnginePlugin>
#include <QtGui/QPixmapCache>
#include <qpa/qplatformtheme.h>
#include <QtGui/QIconEngine>
#include <QtGui/QPalette>
#include <QtCore/qmath.h>
#include <QtCore/QList>
#include <QtCore/QDir>
#include <QtCore/qloggingcategory.h>
#if QT_CONFIG(settings)
#include <QtCore/QSettings>
#endif
#include <QtGui/QPainter>

#include <private/qhexstring_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcIconLoader, "qt.gui.icon.loader")

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC(QIconLoader, iconLoaderInstance)

/* Theme to use in last resort, if the theme does not have the icon, neither the parents  */
static QString systemFallbackThemeName()
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::SystemIconFallbackThemeName);
        if (themeHint.isValid())
            return themeHint.toString();
    }
    return QString();
}

QIconLoader::QIconLoader() :
        m_themeKey(1), m_supportsSvg(false), m_initialized(false)
{
}

static inline QString systemThemeName()
{
    const auto override = qgetenv("QT_QPA_SYSTEM_ICON_THEME");
    if (!override.isEmpty())
        return QString::fromLocal8Bit(override);
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::SystemIconThemeName);
        if (themeHint.isValid())
            return themeHint.toString();
    }
    return QString();
}

static inline QStringList systemIconSearchPaths()
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::IconThemeSearchPaths);
        if (themeHint.isValid())
            return themeHint.toStringList();
    }
    return QStringList();
}

static inline QStringList systemFallbackSearchPaths()
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::IconFallbackSearchPaths);
        if (themeHint.isValid())
            return themeHint.toStringList();
    }
    return QStringList();
}

extern QFactoryLoader *qt_iconEngineFactoryLoader(); // qicon.cpp

void QIconLoader::ensureInitialized()
{
    if (!m_initialized) {
        if (!QGuiApplicationPrivate::platformTheme())
            return; // it's too early: try again later (QTBUG-74252)
        m_initialized = true;
        m_systemTheme = systemThemeName();

        if (m_systemTheme.isEmpty())
            m_systemTheme = systemFallbackThemeName();
        if (qt_iconEngineFactoryLoader()->keyMap().key("svg"_L1, -1) != -1)
            m_supportsSvg = true;

        qCDebug(lcIconLoader) << "Initialized icon loader with system theme"
            << m_systemTheme << "and SVG support" << m_supportsSvg;
    }
}

/*!
    \internal
    Gets an instance.

    \l QIcon::setFallbackThemeName() should be called before QGuiApplication is
    created, to avoid a race condition (QTBUG-74252). When this function is
    called from there, ensureInitialized() does not succeed because there
    is no QPlatformTheme yet, so systemThemeName() is empty, and we don't want
    m_systemTheme to get initialized to the fallback theme instead of the normal one.
*/
QIconLoader *QIconLoader::instance()
{
   iconLoaderInstance()->ensureInitialized();
   return iconLoaderInstance();
}

// Queries the system theme and invalidates existing
// icons if the theme has changed.
void QIconLoader::updateSystemTheme()
{
    const QString currentSystemTheme = m_systemTheme;
    m_systemTheme = systemThemeName();
    if (m_systemTheme.isEmpty())
        m_systemTheme = systemFallbackThemeName();
    if (m_systemTheme != currentSystemTheme)
        qCDebug(lcIconLoader) << "Updated system theme to" << m_systemTheme;
    // Invalidate even if the system theme name hasn't changed, as the
    // theme itself may have changed its underlying icon lookup logic.
    if (!hasUserTheme())
        invalidateKey();
}

void QIconLoader::invalidateKey()
{
    // Invalidating the key here will result in QThemeIconEngine
    // recreating the actual engine the next time the icon is used.
    // We don't need to clear the QIcon cache itself.
    m_themeKey++;
}

QString QIconLoader::themeName() const
{
    return m_userTheme.isEmpty() ? m_systemTheme : m_userTheme;
}

void QIconLoader::setThemeName(const QString &themeName)
{
    if (m_userTheme == themeName)
        return;

    qCDebug(lcIconLoader) << "Setting user theme name to" << themeName;

    const bool hadUserTheme = hasUserTheme();
    m_userTheme = themeName;
    // if we cleared the user theme, then reset search paths as well,
    // otherwise we'll keep looking in the user-defined search paths for
    // a system-provide theme, which will never work.
    if (!hasUserTheme() && hadUserTheme)
        setThemeSearchPath(systemIconSearchPaths());
    invalidateKey();
}

QString QIconLoader::fallbackThemeName() const
{
    return m_userFallbackTheme.isEmpty() ? systemFallbackThemeName() : m_userFallbackTheme;
}

void QIconLoader::setFallbackThemeName(const QString &themeName)
{
    qCDebug(lcIconLoader) << "Setting fallback theme name to" << themeName;
    m_userFallbackTheme = themeName;
    invalidateKey();
}

void QIconLoader::setThemeSearchPath(const QStringList &searchPaths)
{
    qCDebug(lcIconLoader) << "Setting theme search path to" << searchPaths;
    m_iconDirs = searchPaths;
    themeList.clear();
    invalidateKey();
}

QStringList QIconLoader::themeSearchPaths() const
{
    if (m_iconDirs.isEmpty()) {
        m_iconDirs = systemIconSearchPaths();
        // Always add resource directory as search path
        m_iconDirs.append(":/icons"_L1);
    }
    return m_iconDirs;
}

void QIconLoader::setFallbackSearchPaths(const QStringList &searchPaths)
{
    qCDebug(lcIconLoader) << "Setting fallback search path to" << searchPaths;
    m_fallbackDirs = searchPaths;
    invalidateKey();
}

QStringList QIconLoader::fallbackSearchPaths() const
{
    if (m_fallbackDirs.isEmpty()) {
        m_fallbackDirs = systemFallbackSearchPaths();
    }
    return m_fallbackDirs;
}

/*!
    \internal
    Helper class that reads and looks up into the icon-theme.cache generated with
    gtk-update-icon-cache. If at any point we detect a corruption in the file
    (because the offsets point at wrong locations for example), the reader
    is marked as invalid.
*/
class QIconCacheGtkReader
{
public:
    explicit QIconCacheGtkReader(const QString &themeDir);
    QList<const char *> lookup(QStringView);
    bool isValid() const { return m_isValid; }
private:
    QFile m_file;
    const unsigned char *m_data;
    quint64 m_size;
    bool m_isValid;

    quint16 read16(uint offset)
    {
        if (offset > m_size - 2 || (offset & 0x1)) {
            m_isValid = false;
            return 0;
        }
        return m_data[offset+1] | m_data[offset] << 8;
    }
    quint32 read32(uint offset)
    {
        if (offset > m_size - 4 || (offset & 0x3)) {
            m_isValid = false;
            return 0;
        }
        return m_data[offset+3] | m_data[offset+2] << 8
            | m_data[offset+1] << 16 | m_data[offset] << 24;
    }
};


QIconCacheGtkReader::QIconCacheGtkReader(const QString &dirName)
    : m_isValid(false)
{
    QFileInfo info(dirName + "/icon-theme.cache"_L1);
    if (!info.exists() || info.lastModified(QTimeZone::UTC) < QFileInfo(dirName).lastModified(QTimeZone::UTC))
        return;
    m_file.setFileName(info.absoluteFilePath());
    if (!m_file.open(QFile::ReadOnly))
        return;
    m_size = m_file.size();
    m_data = m_file.map(0, m_size);
    if (!m_data)
        return;
    if (read16(0) != 1) // VERSION_MAJOR
        return;

    m_isValid = true;

    // Check that all the directories are older than the cache
    const QDateTime lastModified = info.lastModified(QTimeZone::UTC);
    quint32 dirListOffset = read32(8);
    quint32 dirListLen = read32(dirListOffset);
    for (uint i = 0; i < dirListLen; ++i) {
        quint32 offset = read32(dirListOffset + 4 + 4 * i);
        if (!m_isValid || offset >= m_size || lastModified < QFileInfo(dirName + u'/'
                + QString::fromUtf8(reinterpret_cast<const char*>(m_data + offset))).lastModified(QTimeZone::UTC)) {
            m_isValid = false;
            return;
        }
    }
}

static quint32 icon_name_hash(const char *p)
{
    quint32 h = static_cast<signed char>(*p);
    for (p += 1; *p != '\0'; p++)
        h = (h << 5) - h + *p;
    return h;
}

/*! \internal
    lookup the icon name and return the list of subdirectories in which an icon
    with this name is present. The char* are pointers to the mapped data.
    For example, this would return { "32x32/apps", "24x24/apps" , ... }
 */
QList<const char *> QIconCacheGtkReader::lookup(QStringView name)
{
    QList<const char *> ret;
    if (!isValid() || name.isEmpty())
        return ret;

    QByteArray nameUtf8 = name.toUtf8();
    quint32 hash = icon_name_hash(nameUtf8);

    quint32 hashOffset = read32(4);
    quint32 hashBucketCount = read32(hashOffset);

    if (!isValid() || hashBucketCount == 0) {
        m_isValid = false;
        return ret;
    }

    quint32 bucketIndex = hash % hashBucketCount;
    quint32 bucketOffset = read32(hashOffset + 4 + bucketIndex * 4);
    while (bucketOffset > 0 && bucketOffset <= m_size - 12) {
        quint32 nameOff = read32(bucketOffset + 4);
        if (nameOff < m_size && strcmp(reinterpret_cast<const char*>(m_data + nameOff), nameUtf8) == 0) {
            quint32 dirListOffset = read32(8);
            quint32 dirListLen = read32(dirListOffset);

            quint32 listOffset = read32(bucketOffset+8);
            quint32 listLen = read32(listOffset);

            if (!m_isValid || listOffset + 4 + 8 * listLen > m_size) {
                m_isValid = false;
                return ret;
            }

            ret.reserve(listLen);
            for (uint j = 0; j < listLen && m_isValid; ++j) {
                quint32 dirIndex = read16(listOffset + 4 + 8 * j);
                quint32 o = read32(dirListOffset + 4 + dirIndex*4);
                if (!m_isValid || dirIndex >= dirListLen || o >= m_size) {
                    m_isValid = false;
                    return ret;
                }
                ret.append(reinterpret_cast<const char*>(m_data) + o);
            }
            return ret;
        }
        bucketOffset = read32(bucketOffset);
    }
    return ret;
}

QIconTheme::QIconTheme(const QString &themeName)
        : m_valid(false)
{
    QFile themeIndex;

    const QStringList iconDirs = QIcon::themeSearchPaths();
    for ( int i = 0 ; i < iconDirs.size() ; ++i) {
        QDir iconDir(iconDirs[i]);
        QString themeDir = iconDir.path() + u'/' + themeName;
        QFileInfo themeDirInfo(themeDir);

        if (themeDirInfo.isDir()) {
            m_contentDirs << themeDir;
            m_gtkCaches << QSharedPointer<QIconCacheGtkReader>::create(themeDir);
        }

        if (!m_valid) {
            themeIndex.setFileName(themeDir + "/index.theme"_L1);
            m_valid = themeIndex.exists();
            qCDebug(lcIconLoader) << "Probing theme file at" << themeIndex.fileName() << m_valid;
        }
    }
#if QT_CONFIG(settings)
    if (m_valid) {
        const QSettings indexReader(themeIndex.fileName(), QSettings::IniFormat);
        const QStringList keys = indexReader.allKeys();
        for (const QString &key : keys) {
            if (key.endsWith("/Size"_L1)) {
                // Note the QSettings ini-format does not accept
                // slashes in key names, hence we have to cheat
                if (int size = indexReader.value(key).toInt()) {
                    QString directoryKey = key.left(key.size() - 5);
                    QIconDirInfo dirInfo(directoryKey);
                    dirInfo.size = size;
                    QString type = indexReader.value(directoryKey + "/Type"_L1).toString();

                    if (type == "Fixed"_L1)
                        dirInfo.type = QIconDirInfo::Fixed;
                    else if (type == "Scalable"_L1)
                        dirInfo.type = QIconDirInfo::Scalable;
                    else
                        dirInfo.type = QIconDirInfo::Threshold;

                    dirInfo.threshold = indexReader.value(directoryKey +
                                                          "/Threshold"_L1,
                                                          2).toInt();

                    dirInfo.minSize = indexReader.value(directoryKey + "/MinSize"_L1, size).toInt();

                    dirInfo.maxSize = indexReader.value(directoryKey + "/MaxSize"_L1, size).toInt();

                    dirInfo.scale = indexReader.value(directoryKey + "/Scale"_L1, 1).toInt();
                    m_keyList.append(dirInfo);
                }
            }
        }

        // Parent themes provide fallbacks for missing icons
        m_parents = indexReader.value("Icon Theme/Inherits"_L1).toStringList();
        m_parents.removeAll(QString());
    }
#endif // settings
}

QStringList QIconTheme::parents() const
{
    // Respect explicitly declared parents
    QStringList result = m_parents;

    // Ensure a default fallback for all themes
    const QString fallback = QIconLoader::instance()->fallbackThemeName();
    if (!fallback.isEmpty())
        result.append(fallback);

    // Ensure that all themes fall back to hicolor as the last theme
    result.removeAll("hicolor"_L1);
    result.append("hicolor"_L1);

    return result;
}

QDebug operator<<(QDebug debug, const std::unique_ptr<QIconLoaderEngineEntry> &entry)
{
    QDebugStateSaver saver(debug);
    debug.noquote() << entry->filename;
    return debug;
}

QThemeIconInfo QIconLoader::findIconHelper(const QString &themeName,
                                           const QString &iconName,
                                           QStringList &visited) const
{
    qCDebug(lcIconLoader) << "Finding icon" << iconName << "in theme" << themeName
                          << "skipping" << visited;

    QThemeIconInfo info;
    Q_ASSERT(!themeName.isEmpty());

    // Used to protect against potential recursions
    visited << themeName;

    QIconTheme &theme = themeList[themeName];
    if (!theme.isValid()) {
        theme = QIconTheme(themeName);
        if (!theme.isValid()) {
            qCDebug(lcIconLoader) << "Theme" << themeName << "not found";
            return info;
        }
    }

    const QStringList contentDirs = theme.contentDirs();

    QStringView iconNameFallback(iconName);

    // Iterate through all icon's fallbacks in current theme
    while (info.entries.empty()) {
        const QString svgIconName = iconNameFallback + ".svg"_L1;
        const QString pngIconName = iconNameFallback + ".png"_L1;

        // Add all relevant files
        for (int i = 0; i < contentDirs.size(); ++i) {
            QList<QIconDirInfo> subDirs = theme.keyList();

            // Try to reduce the amount of subDirs by looking in the GTK+ cache in order to save
            // a massive amount of file stat (especially if the icon is not there)
            auto cache = theme.m_gtkCaches.at(i);
            if (cache->isValid()) {
                const auto result = cache->lookup(iconNameFallback);
                if (cache->isValid()) {
                    const QList<QIconDirInfo> subDirsCopy = subDirs;
                    subDirs.clear();
                    subDirs.reserve(result.size());
                    for (const char *s : result) {
                        QString path = QString::fromUtf8(s);
                        auto it = std::find_if(subDirsCopy.cbegin(), subDirsCopy.cend(),
                                               [&](const QIconDirInfo &info) {
                                                   return info.path == path; } );
                        if (it != subDirsCopy.cend()) {
                            subDirs.append(*it);
                        }
                    }
                }
            }

            QString contentDir = contentDirs.at(i) + u'/';
            for (int j = 0; j < subDirs.size() ; ++j) {
                const QIconDirInfo &dirInfo = subDirs.at(j);
                const QString subDir = contentDir + dirInfo.path + u'/';
                const QString pngPath = subDir + pngIconName;
                if (QFile::exists(pngPath)) {
                    auto iconEntry = std::make_unique<PixmapEntry>();
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = pngPath;
                    // Notice we ensure that pixmap entries always come before
                    // scalable to preserve search order afterwards
                    info.entries.insert(info.entries.begin(), std::move(iconEntry));
                } else if (m_supportsSvg) {
                    const QString svgPath = subDir + svgIconName;
                    if (QFile::exists(svgPath)) {
                        auto iconEntry = std::make_unique<ScalableEntry>();
                        iconEntry->dir = dirInfo;
                        iconEntry->filename = svgPath;
                        info.entries.push_back(std::move(iconEntry));
                    }
                }
            }
        }

        if (!info.entries.empty()) {
            info.iconName = iconNameFallback.toString();
            break;
        }

        // If it's possible - find next fallback for the icon
        const int indexOfDash = iconNameFallback.lastIndexOf(u'-');
        if (indexOfDash == -1)
            break;

        iconNameFallback.truncate(indexOfDash);
    }

    if (info.entries.empty()) {
        const QStringList parents = theme.parents();
        qCDebug(lcIconLoader) << "Did not find matching icons in theme;"
            << "trying parent themes" << parents
            << "skipping visited" << visited;

        // Search recursively through inherited themes
        for (int i = 0 ; i < parents.size() ; ++i) {

            const QString parentTheme = parents.at(i).trimmed();

            if (!visited.contains(parentTheme)) // guard against recursion
                info = findIconHelper(parentTheme, iconName, visited);

            if (!info.entries.empty()) // success
                break;
        }
    }

    return info;
}

QThemeIconInfo QIconLoader::lookupFallbackIcon(const QString &iconName) const
{
    qCDebug(lcIconLoader) << "Looking up fallback icon" << iconName;

    QThemeIconInfo info;

    const QString pngIconName = iconName + ".png"_L1;
    const QString xpmIconName = iconName + ".xpm"_L1;
    const QString svgIconName = iconName + ".svg"_L1;

    const auto searchPaths = QIcon::fallbackSearchPaths();
    for (const QString &iconDir: searchPaths) {
        QDir currentDir(iconDir);
        std::unique_ptr<QIconLoaderEngineEntry> iconEntry;
        if (currentDir.exists(pngIconName)) {
            iconEntry = std::make_unique<PixmapEntry>();
            iconEntry->dir.type = QIconDirInfo::Fallback;
            iconEntry->filename = currentDir.filePath(pngIconName);
        } else if (currentDir.exists(xpmIconName)) {
            iconEntry = std::make_unique<PixmapEntry>();
            iconEntry->dir.type = QIconDirInfo::Fallback;
            iconEntry->filename = currentDir.filePath(xpmIconName);
        } else if (m_supportsSvg &&
                   currentDir.exists(svgIconName)) {
            iconEntry = std::make_unique<ScalableEntry>();
            iconEntry->dir.type = QIconDirInfo::Fallback;
            iconEntry->filename = currentDir.filePath(svgIconName);
        }
        if (iconEntry) {
            info.entries.push_back(std::move(iconEntry));
            break;
        }
    }

    if (!info.entries.empty())
        info.iconName = iconName;

    return info;
}

QThemeIconInfo QIconLoader::loadIcon(const QString &name) const
{
    qCDebug(lcIconLoader) << "Loading icon" << name;

    QThemeIconInfo iconInfo;
    QStringList visitedThemes;
    if (!themeName().isEmpty())
        iconInfo = findIconHelper(themeName(), name, visitedThemes);

    if (iconInfo.entries.empty() && !fallbackThemeName().isEmpty())
        iconInfo = findIconHelper(fallbackThemeName(), name, visitedThemes);

    if (iconInfo.entries.empty())
        iconInfo = lookupFallbackIcon(name);

    qCDebug(lcIconLoader) << "Resulting icon entries" << iconInfo.entries;
    return iconInfo;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, QIconEngine *engine)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    if (engine) {
        debug.noquote() << engine->key() << "(";
        debug << static_cast<const void *>(engine);
        if (!engine->isNull())
            debug.quote() << ", " << engine->iconName();
        else
            debug << ", null";
        debug << ")";
    } else {
        debug << "QIconEngine(nullptr)";
    }
    return debug;
}
#endif

QIconEngine *QIconLoader::iconEngine(const QString &iconName) const
{
    qCDebug(lcIconLoader) << "Resolving icon engine for icon" << iconName;

    auto *platformTheme = QGuiApplicationPrivate::platformTheme();
    std::unique_ptr<QIconEngine> iconEngine;
    if (!hasUserTheme() && platformTheme)
        iconEngine.reset(platformTheme->createIconEngine(iconName));
    if (!iconEngine || iconEngine->isNull()) {
        iconEngine.reset(new QIconLoaderEngine(iconName));
    }

    qCDebug(lcIconLoader) << "Resulting engine" << iconEngine.get();
    return iconEngine.release();
}

/*!
    \internal
    \class QThemeIconEngine
    \inmodule QtGui

    \brief A named-based icon engine for providing theme icons.

    The engine supports invalidation of prior lookups, e.g. when
    the platform theme changes or the user sets an explicit icon
    theme.

    The actual icon lookup is handed over to an engine provided
    by QIconLoader::iconEngine().
*/

QThemeIconEngine::QThemeIconEngine(const QString& iconName)
    : QProxyIconEngine()
    , m_iconName(iconName)
{
}

QThemeIconEngine::QThemeIconEngine(const QThemeIconEngine &other)
    : QProxyIconEngine()
    , m_iconName(other.m_iconName)
{
}

QString QThemeIconEngine::key() const
{
    // Although we proxy the underlying engine, that's an implementation
    // detail, so from the point of view of QIcon, and in terms of
    // serialization, we are the one and only theme icon engine.
    return u"QThemeIconEngine"_s;
}

QIconEngine *QThemeIconEngine::clone() const
{
    return new QThemeIconEngine(*this);
}

bool QThemeIconEngine::read(QDataStream &in) {
    in >> m_iconName;
    return true;
}

bool QThemeIconEngine::write(QDataStream &out) const
{
    out << m_iconName;
    return true;
}

QIconEngine *QThemeIconEngine::proxiedEngine() const
{
    const auto *iconLoader = QIconLoader::instance();
    auto mostRecentThemeKey = iconLoader->themeKey();
    if (mostRecentThemeKey != m_themeKey) {
        qCDebug(lcIconLoader) << "Theme key" << mostRecentThemeKey << "is different"
            << "than cached key" << m_themeKey << "for icon" << m_iconName;
        m_proxiedEngine.reset(iconLoader->iconEngine(m_iconName));
        m_themeKey = mostRecentThemeKey;
    }
    return m_proxiedEngine.get();
}

/*!
    \internal
    \class QIconLoaderEngine
    \inmodule QtGui

    \brief An icon engine based on icon entries collected by QIconLoader.

    The design and implementation of QIconLoader is based on
    the XDG icon specification.
*/

QIconLoaderEngine::QIconLoaderEngine(const QString& iconName)
    : m_iconName(iconName)
    , m_info(QIconLoader::instance()->loadIcon(m_iconName))
{
}

QIconLoaderEngine::~QIconLoaderEngine() = default;

QIconEngine *QIconLoaderEngine::clone() const
{
    Q_UNREACHABLE();
    return nullptr; // Cannot be cloned
}

bool QIconLoaderEngine::hasIcon() const
{
    return !(m_info.entries.empty());
}

void QIconLoaderEngine::paint(QPainter *painter, const QRect &rect,
                             QIcon::Mode mode, QIcon::State state)
{
    QSize pixmapSize = rect.size() * painter->device()->devicePixelRatio();
    painter->drawPixmap(rect, pixmap(pixmapSize, mode, state));
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static bool directoryMatchesSize(const QIconDirInfo &dir, int iconsize, int iconscale)
{
    if (dir.scale != iconscale)
        return false;

    if (dir.type == QIconDirInfo::Fixed) {
        return dir.size == iconsize;

    } else if (dir.type == QIconDirInfo::Scalable) {
        return iconsize <= dir.maxSize &&
                iconsize >= dir.minSize;

    } else if (dir.type == QIconDirInfo::Threshold) {
        return iconsize >= dir.size - dir.threshold &&
                iconsize <= dir.size + dir.threshold;
    } else if (dir.type == QIconDirInfo::Fallback) {
        return true;
    }

    Q_ASSERT(1); // Not a valid value
    return false;
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static int directorySizeDistance(const QIconDirInfo &dir, int iconsize, int iconscale)
{
    const int scaledIconSize = iconsize * iconscale;
    if (dir.type == QIconDirInfo::Fixed) {
        return qAbs(dir.size * dir.scale - scaledIconSize);

    } else if (dir.type == QIconDirInfo::Scalable) {
        if (scaledIconSize < dir.minSize * dir.scale)
            return dir.minSize * dir.scale - scaledIconSize;
        else if (scaledIconSize > dir.maxSize * dir.scale)
            return scaledIconSize - dir.maxSize * dir.scale;
        else
            return 0;

    } else if (dir.type == QIconDirInfo::Threshold) {
        if (scaledIconSize < (dir.size - dir.threshold) * dir.scale)
            return dir.minSize * dir.scale - scaledIconSize;
        else if (scaledIconSize > (dir.size + dir.threshold) * dir.scale)
            return scaledIconSize - dir.maxSize * dir.scale;
        else return 0;
    } else if (dir.type == QIconDirInfo::Fallback) {
        return 0;
    }

    Q_ASSERT(1); // Not a valid value
    return INT_MAX;
}

QIconLoaderEngineEntry *QIconLoaderEngine::entryForSize(const QThemeIconInfo &info, const QSize &size, int scale)
{
    int iconsize = qMin(size.width(), size.height());

    // Note that m_info.entries are sorted so that png-files
    // come first

    // Search for exact matches first
    for (const auto &entry : info.entries) {
        if (directoryMatchesSize(entry->dir, iconsize, scale)) {
            return entry.get();
        }
    }

    // Find the minimum distance icon
    int minimalSize = INT_MAX;
    QIconLoaderEngineEntry *closestMatch = nullptr;
    for (const auto &entry : info.entries) {
        int distance = directorySizeDistance(entry->dir, iconsize, scale);
        if (distance < minimalSize) {
            minimalSize  = distance;
            closestMatch = entry.get();
        }
    }
    return closestMatch;
}

/*
 * Returns the actual icon size. For scalable svg's this is equivalent
 * to the requested size. Otherwise the closest match is returned but
 * we can never return a bigger size than the requested size.
 *
 */
QSize QIconLoaderEngine::actualSize(const QSize &size, QIcon::Mode mode,
                                   QIcon::State state)
{
    Q_UNUSED(mode);
    Q_UNUSED(state);

    QIconLoaderEngineEntry *entry = entryForSize(m_info, size);
    if (entry) {
        const QIconDirInfo &dir = entry->dir;
        if (dir.type == QIconDirInfo::Scalable) {
            return size;
        } else if (dir.type == QIconDirInfo::Fallback) {
            return QIcon(entry->filename).actualSize(size, mode, state);
        } else {
            int result = qMin<int>(dir.size, qMin(size.width(), size.height()));
            return QSize(result, result);
        }
    }
    return QSize(0, 0);
}

QPixmap PixmapEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);

    // Ensure that basePixmap is lazily initialized before generating the
    // key, otherwise the cache key is not unique
    if (basePixmap.isNull())
        basePixmap.load(filename);

    QSize actualSize = basePixmap.size();
    // If the size of the best match we have (basePixmap) is larger than the
    // requested size, we downscale it to match.
    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);

    QString key = "$qt_theme_"_L1
                  % HexString<qint64>(basePixmap.cacheKey())
                  % HexString<int>(mode)
                  % HexString<qint64>(QGuiApplication::palette().cacheKey())
                  % HexString<int>(actualSize.width())
                  % HexString<int>(actualSize.height());

    QPixmap cachedPixmap;
    if (QPixmapCache::find(key, &cachedPixmap)) {
        return cachedPixmap;
    } else {
        if (basePixmap.size() != actualSize)
            cachedPixmap = basePixmap.scaled(actualSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        else
            cachedPixmap = basePixmap;
        if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
            cachedPixmap = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(mode, cachedPixmap);
        QPixmapCache::insert(key, cachedPixmap);
    }
    return cachedPixmap;
}

QPixmap ScalableEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    if (svgIcon.isNull())
        svgIcon = QIcon(filename);

    // Bypass QIcon API, as that will scale by device pixel ratio of the
    // highest DPR screen since we're not passing on any QWindow.
    if (QIconEngine *engine = svgIcon.data_ptr() ? svgIcon.data_ptr()->engine : nullptr)
        return engine->pixmap(size, mode, state);

    return QPixmap();
}

QPixmap QIconLoaderEngine::pixmap(const QSize &size, QIcon::Mode mode,
                                 QIcon::State state)
{
    QIconLoaderEngineEntry *entry = entryForSize(m_info, size);
    if (entry)
        return entry->pixmap(size, mode, state);

    return QPixmap();
}

QString QIconLoaderEngine::key() const
{
    return u"QIconLoaderEngine"_s;
}

QString QIconLoaderEngine::iconName()
{
    return m_info.iconName;
}

bool QIconLoaderEngine::isNull()
{
    return m_info.entries.empty();
}

QPixmap QIconLoaderEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    const int integerScale = qCeil(scale);
    QIconLoaderEngineEntry *entry = entryForSize(m_info, size / integerScale, integerScale);
    return entry ? entry->pixmap(size, mode, state) : QPixmap();
}

QList<QSize> QIconLoaderEngine::availableSizes(QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(mode);
    Q_UNUSED(state);

    const qsizetype N = qsizetype(m_info.entries.size());
    QList<QSize> sizes;
    sizes.reserve(N);

    // Gets all sizes from the DirectoryInfo entries
    for (const auto &entry : m_info.entries) {
        if (entry->dir.type == QIconDirInfo::Fallback) {
            sizes.append(QIcon(entry->filename).availableSizes());
        } else {
            int size = entry->dir.size;
            sizes.append(QSize(size, size));
        }
    }
    return sizes;
}

QT_END_NAMESPACE

#endif //QT_NO_ICON
