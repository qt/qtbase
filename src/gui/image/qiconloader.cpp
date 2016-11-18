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
#ifndef QT_NO_ICON
#include <private/qiconloader_p.h>

#include <private/qguiapplication_p.h>
#include <private/qicon_p.h>

#include <QtGui/QIconEnginePlugin>
#include <QtGui/QPixmapCache>
#include <qpa/qplatformtheme.h>
#include <QtGui/QIconEngine>
#include <QtGui/QPalette>
#include <QtCore/QList>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtGui/QPainter>

#include <private/qhexstring_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QIconLoader, iconLoaderInstance)

/* Theme to use in last resort, if the theme does not have the icon, neither the parents  */
static QString fallbackTheme()
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

extern QFactoryLoader *qt_iconEngineFactoryLoader(); // qicon.cpp

void QIconLoader::ensureInitialized()
{
    if (!m_initialized) {
        m_initialized = true;

        Q_ASSERT(qApp);

        m_systemTheme = systemThemeName();

        if (m_systemTheme.isEmpty())
            m_systemTheme = fallbackTheme();
        if (qt_iconEngineFactoryLoader()->keyMap().key(QLatin1String("svg"), -1) != -1)
            m_supportsSvg = true;
    }
}

QIconLoader *QIconLoader::instance()
{
   iconLoaderInstance()->ensureInitialized();
   return iconLoaderInstance();
}

// Queries the system theme and invalidates existing
// icons if the theme has changed.
void QIconLoader::updateSystemTheme()
{
    // Only change if this is not explicitly set by the user
    if (m_userTheme.isEmpty()) {
        QString theme = systemThemeName();
        if (theme.isEmpty())
            theme = fallbackTheme();
        if (theme != m_systemTheme) {
            m_systemTheme = theme;
            invalidateKey();
        }
    }
}

void QIconLoader::setThemeName(const QString &themeName)
{
    m_userTheme = themeName;
    invalidateKey();
}

void QIconLoader::setThemeSearchPath(const QStringList &searchPaths)
{
    m_iconDirs = searchPaths;
    themeList.clear();
    invalidateKey();
}

QStringList QIconLoader::themeSearchPaths() const
{
    if (m_iconDirs.isEmpty()) {
        m_iconDirs = systemIconSearchPaths();
        // Always add resource directory as search path
        m_iconDirs.append(QLatin1String(":/icons"));
    }
    return m_iconDirs;
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
    QVector<const char *> lookup(const QStringRef &);
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
    QFileInfo info(dirName + QLatin1String("/icon-theme.cache"));
    if (!info.exists() || info.lastModified() < QFileInfo(dirName).lastModified())
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
    auto lastModified = info.lastModified();
    quint32 dirListOffset = read32(8);
    quint32 dirListLen = read32(dirListOffset);
    for (uint i = 0; i < dirListLen; ++i) {
        quint32 offset = read32(dirListOffset + 4 + 4 * i);
        if (!m_isValid || offset >= m_size || lastModified < QFileInfo(dirName + QLatin1Char('/')
                + QString::fromUtf8(reinterpret_cast<const char*>(m_data + offset))).lastModified()) {
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
QVector<const char *> QIconCacheGtkReader::lookup(const QStringRef &name)
{
    QVector<const char *> ret;
    if (!isValid())
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
        QString themeDir = iconDir.path() + QLatin1Char('/') + themeName;
        QFileInfo themeDirInfo(themeDir);

        if (themeDirInfo.isDir()) {
            m_contentDirs << themeDir;
            m_gtkCaches << QSharedPointer<QIconCacheGtkReader>::create(themeDir);
        }

        if (!m_valid) {
            themeIndex.setFileName(themeDir + QLatin1String("/index.theme"));
            if (themeIndex.exists())
                m_valid = true;
        }
    }
#ifndef QT_NO_SETTINGS
    if (themeIndex.exists()) {
        const QSettings indexReader(themeIndex.fileName(), QSettings::IniFormat);
        const QStringList keys = indexReader.allKeys();
        for (const QString &key : keys) {
            if (key.endsWith(QLatin1String("/Size"))) {
                // Note the QSettings ini-format does not accept
                // slashes in key names, hence we have to cheat
                if (int size = indexReader.value(key).toInt()) {
                    QString directoryKey = key.left(key.size() - 5);
                    QIconDirInfo dirInfo(directoryKey);
                    dirInfo.size = size;
                    QString type = indexReader.value(directoryKey +
                                                     QLatin1String("/Type")
                                                     ).toString();

                    if (type == QLatin1String("Fixed"))
                        dirInfo.type = QIconDirInfo::Fixed;
                    else if (type == QLatin1String("Scalable"))
                        dirInfo.type = QIconDirInfo::Scalable;
                    else
                        dirInfo.type = QIconDirInfo::Threshold;

                    dirInfo.threshold = indexReader.value(directoryKey +
                                                        QLatin1String("/Threshold"),
                                                        2).toInt();

                    dirInfo.minSize = indexReader.value(directoryKey +
                                                         QLatin1String("/MinSize"),
                                                         size).toInt();

                    dirInfo.maxSize = indexReader.value(directoryKey +
                                                        QLatin1String("/MaxSize"),
                                                        size).toInt();
                    m_keyList.append(dirInfo);
                }
            }
        }

        // Parent themes provide fallbacks for missing icons
        m_parents = indexReader.value(
                QLatin1String("Icon Theme/Inherits")).toStringList();
        m_parents.removeAll(QString());

        // Ensure a default platform fallback for all themes
        if (m_parents.isEmpty()) {
            const QString fallback = fallbackTheme();
            if (!fallback.isEmpty())
                m_parents.append(fallback);
        }

        // Ensure that all themes fall back to hicolor
        if (!m_parents.contains(QLatin1String("hicolor")))
            m_parents.append(QLatin1String("hicolor"));
    }
#endif //QT_NO_SETTINGS
}

QThemeIconInfo QIconLoader::findIconHelper(const QString &themeName,
                                           const QString &iconName,
                                           QStringList &visited) const
{
    QThemeIconInfo info;
    Q_ASSERT(!themeName.isEmpty());

    // Used to protect against potential recursions
    visited << themeName;

    QIconTheme &theme = themeList[themeName];
    if (!theme.isValid()) {
        theme = QIconTheme(themeName);
        if (!theme.isValid())
            theme = QIconTheme(fallbackTheme());
    }

    const QStringList contentDirs = theme.contentDirs();

    QStringRef iconNameFallback(&iconName);

    // Iterate through all icon's fallbacks in current theme
    while (info.entries.isEmpty()) {
        const QString svgIconName = iconNameFallback + QLatin1String(".svg");
        const QString pngIconName = iconNameFallback + QLatin1String(".png");

        // Add all relevant files
        for (int i = 0; i < contentDirs.size(); ++i) {
            QVector<QIconDirInfo> subDirs = theme.keyList();

            // Try to reduce the amount of subDirs by looking in the GTK+ cache in order to save
            // a massive amount of file stat (especially if the icon is not there)
            auto cache = theme.m_gtkCaches.at(i);
            if (cache->isValid()) {
                const auto result = cache->lookup(iconNameFallback);
                if (cache->isValid()) {
                    const QVector<QIconDirInfo> subDirsCopy = subDirs;
                    subDirs.clear();
                    subDirs.reserve(result.count());
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

            QString contentDir = contentDirs.at(i) + QLatin1Char('/');
            for (int j = 0; j < subDirs.size() ; ++j) {
                const QIconDirInfo &dirInfo = subDirs.at(j);
                const QString subDir = contentDir + dirInfo.path + QLatin1Char('/');
                const QString pngPath = subDir + pngIconName;
                if (QFile::exists(pngPath)) {
                    PixmapEntry *iconEntry = new PixmapEntry;
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = pngPath;
                    // Notice we ensure that pixmap entries always come before
                    // scalable to preserve search order afterwards
                    info.entries.prepend(iconEntry);
                } else if (m_supportsSvg) {
                    const QString svgPath = subDir + svgIconName;
                    if (QFile::exists(svgPath)) {
                        ScalableEntry *iconEntry = new ScalableEntry;
                        iconEntry->dir = dirInfo;
                        iconEntry->filename = svgPath;
                        info.entries.append(iconEntry);
                    }
                }
            }
        }

        if (!info.entries.isEmpty()) {
            info.iconName = iconNameFallback.toString();
            break;
        }

        // If it's possible - find next fallback for the icon
        const int indexOfDash = iconNameFallback.lastIndexOf(QLatin1Char('-'));
        if (indexOfDash == -1)
            break;

        iconNameFallback.truncate(indexOfDash);
    }

    if (info.entries.isEmpty()) {
        const QStringList parents = theme.parents();
        // Search recursively through inherited themes
        for (int i = 0 ; i < parents.size() ; ++i) {

            const QString parentTheme = parents.at(i).trimmed();

            if (!visited.contains(parentTheme)) // guard against recursion
                info = findIconHelper(parentTheme, iconName, visited);

            if (!info.entries.isEmpty()) // success
                break;
        }
    }
    return info;
}

QThemeIconInfo QIconLoader::loadIcon(const QString &name) const
{
    if (!themeName().isEmpty()) {
        QStringList visited;
        return findIconHelper(themeName(), name, visited);
    }

    return QThemeIconInfo();
}


// -------- Icon Loader Engine -------- //


QIconLoaderEngine::QIconLoaderEngine(const QString& iconName)
        : m_iconName(iconName), m_key(0)
{
}

QIconLoaderEngine::~QIconLoaderEngine()
{
    qDeleteAll(m_info.entries);
}

QIconLoaderEngine::QIconLoaderEngine(const QIconLoaderEngine &other)
        : QIconEngine(other),
        m_iconName(other.m_iconName),
        m_key(0)
{
}

QIconEngine *QIconLoaderEngine::clone() const
{
    return new QIconLoaderEngine(*this);
}

bool QIconLoaderEngine::read(QDataStream &in) {
    in >> m_iconName;
    return true;
}

bool QIconLoaderEngine::write(QDataStream &out) const
{
    out << m_iconName;
    return true;
}

bool QIconLoaderEngine::hasIcon() const
{
    return !(m_info.entries.isEmpty());
}

// Lazily load the icon
void QIconLoaderEngine::ensureLoaded()
{
    if (!(QIconLoader::instance()->themeKey() == m_key)) {
        qDeleteAll(m_info.entries);
        m_info.entries.clear();
        m_info.iconName.clear();

        Q_ASSERT(m_info.entries.size() == 0);
        m_info = QIconLoader::instance()->loadIcon(m_iconName);
        m_key = QIconLoader::instance()->themeKey();
    }
}

void QIconLoaderEngine::paint(QPainter *painter, const QRect &rect,
                             QIcon::Mode mode, QIcon::State state)
{
    QSize pixmapSize = rect.size();
    painter->drawPixmap(rect, pixmap(pixmapSize, mode, state));
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static bool directoryMatchesSize(const QIconDirInfo &dir, int iconsize)
{
    if (dir.type == QIconDirInfo::Fixed) {
        return dir.size == iconsize;

    } else if (dir.type == QIconDirInfo::Scalable) {
        return iconsize <= dir.maxSize &&
                iconsize >= dir.minSize;

    } else if (dir.type == QIconDirInfo::Threshold) {
        return iconsize >= dir.size - dir.threshold &&
                iconsize <= dir.size + dir.threshold;
    }

    Q_ASSERT(1); // Not a valid value
    return false;
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static int directorySizeDistance(const QIconDirInfo &dir, int iconsize)
{
    if (dir.type == QIconDirInfo::Fixed) {
        return qAbs(dir.size - iconsize);

    } else if (dir.type == QIconDirInfo::Scalable) {
        if (iconsize < dir.minSize)
            return dir.minSize - iconsize;
        else if (iconsize > dir.maxSize)
            return iconsize - dir.maxSize;
        else
            return 0;

    } else if (dir.type == QIconDirInfo::Threshold) {
        if (iconsize < dir.size - dir.threshold)
            return dir.minSize - iconsize;
        else if (iconsize > dir.size + dir.threshold)
            return iconsize - dir.maxSize;
        else return 0;
    }

    Q_ASSERT(1); // Not a valid value
    return INT_MAX;
}

QIconLoaderEngineEntry *QIconLoaderEngine::entryForSize(const QSize &size)
{
    int iconsize = qMin(size.width(), size.height());

    // Note that m_info.entries are sorted so that png-files
    // come first

    const int numEntries = m_info.entries.size();

    // Search for exact matches first
    for (int i = 0; i < numEntries; ++i) {
        QIconLoaderEngineEntry *entry = m_info.entries.at(i);
        if (directoryMatchesSize(entry->dir, iconsize)) {
            return entry;
        }
    }

    // Find the minimum distance icon
    int minimalSize = INT_MAX;
    QIconLoaderEngineEntry *closestMatch = 0;
    for (int i = 0; i < numEntries; ++i) {
        QIconLoaderEngineEntry *entry = m_info.entries.at(i);
        int distance = directorySizeDistance(entry->dir, iconsize);
        if (distance < minimalSize) {
            minimalSize  = distance;
            closestMatch = entry;
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
    ensureLoaded();

    QIconLoaderEngineEntry *entry = entryForSize(size);
    if (entry) {
        const QIconDirInfo &dir = entry->dir;
        if (dir.type == QIconDirInfo::Scalable)
            return size;
        else {
            int result = qMin<int>(dir.size, qMin(size.width(), size.height()));
            return QSize(result, result);
        }
    }
    return QIconEngine::actualSize(size, mode, state);
}

QPixmap PixmapEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);

    // Ensure that basePixmap is lazily initialized before generating the
    // key, otherwise the cache key is not unique
    if (basePixmap.isNull())
        basePixmap.load(filename);

    QSize actualSize = basePixmap.size();
    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);

    QString key = QLatin1String("$qt_theme_")
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

    // Simply reuse svg icon engine
    return svgIcon.pixmap(size, mode, state);
}

QPixmap QIconLoaderEngine::pixmap(const QSize &size, QIcon::Mode mode,
                                 QIcon::State state)
{
    ensureLoaded();

    QIconLoaderEngineEntry *entry = entryForSize(size);
    if (entry)
        return entry->pixmap(size, mode, state);

    return QPixmap();
}

QString QIconLoaderEngine::key() const
{
    return QLatin1String("QIconLoaderEngine");
}

void QIconLoaderEngine::virtual_hook(int id, void *data)
{
    ensureLoaded();

    switch (id) {
    case QIconEngine::AvailableSizesHook:
        {
            QIconEngine::AvailableSizesArgument &arg
                    = *reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
            const int N = m_info.entries.size();
            QList<QSize> sizes;
            sizes.reserve(N);

            // Gets all sizes from the DirectoryInfo entries
            for (int i = 0; i < N; ++i) {
                int size = m_info.entries.at(i)->dir.size;
                sizes.append(QSize(size, size));
            }
            arg.sizes.swap(sizes); // commit
        }
        break;
    case QIconEngine::IconNameHook:
        {
            QString &name = *reinterpret_cast<QString*>(data);
            name = m_info.iconName;
        }
        break;
    case QIconEngine::IsNullHook:
        {
            *reinterpret_cast<bool*>(data) = m_info.entries.isEmpty();
        }
        break;
    default:
        QIconEngine::virtual_hook(id, data);
    }
}

QT_END_NAMESPACE

#endif //QT_NO_ICON
