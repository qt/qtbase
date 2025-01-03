// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QICONLOADER_P_H
#define QICONLOADER_P_H

#include <QtGui/private/qtguiglobal_p.h>

#ifndef QT_NO_ICON
//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/QIcon>
#include <QtGui/QIconEngine>
#include <QtGui/QPixmapCache>
#include <private/qicon_p.h>
#include <private/qfactoryloader_p.h>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QTypeInfo>

#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

class QIconLoader;

struct QIconDirInfo
{
    enum Type { Fixed, Scalable, Threshold, Fallback };
    QIconDirInfo(const QString &_path = QString()) :
            path(_path),
            size(0),
            maxSize(0),
            minSize(0),
            threshold(0),
            scale(1),
            type(Threshold) {}
    QString path;
    short size;
    short maxSize;
    short minSize;
    short threshold;
    short scale;
    Type type;
};
Q_DECLARE_TYPEINFO(QIconDirInfo, Q_RELOCATABLE_TYPE);

class QIconLoaderEngineEntry
 {
public:
    virtual ~QIconLoaderEngineEntry() {}
    virtual QPixmap pixmap(const QSize &size,
                           QIcon::Mode mode,
                           QIcon::State state) = 0;
    QString filename;
    QIconDirInfo dir;
};

struct ScalableEntry : public QIconLoaderEngineEntry
{
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QIcon svgIcon;
};

struct PixmapEntry : public QIconLoaderEngineEntry
{
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap basePixmap;
};

using QThemeIconEntries = std::vector<std::unique_ptr<QIconLoaderEngineEntry>>;

struct QThemeIconInfo
{
    QThemeIconEntries entries;
    QString iconName;
};

class QIconLoaderEngine : public QIconEngine
{
public:
    QIconLoaderEngine(const QString& iconName = QString());
    ~QIconLoaderEngine();

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine *clone() const override;
    bool read(QDataStream &in) override;
    bool write(QDataStream &out) const override;

    QString iconName() override;
    bool isNull() override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) override;

    Q_GUI_EXPORT static QIconLoaderEngineEntry *entryForSize(const QThemeIconInfo &info, const QSize &size, int scale = 1);

private:
    QString key() const override;
    bool hasIcon() const;
    void ensureLoaded();

    QIconLoaderEngine(const QIconLoaderEngine &other);
    QThemeIconInfo m_info;
    QString m_iconName;
    uint m_key;

    friend class QIconLoader;
};

class QIconCacheGtkReader;

class QIconTheme
{
public:
    QIconTheme(const QString &name);
    QIconTheme() : m_valid(false) {}
    QStringList parents() const;
    QList<QIconDirInfo> keyList() { return m_keyList; }
    QStringList contentDirs() { return m_contentDirs; }
    bool isValid() { return m_valid; }
private:
    QStringList m_contentDirs;
    QList<QIconDirInfo> m_keyList;
    QStringList m_parents;
    bool m_valid;
public:
    QList<QSharedPointer<QIconCacheGtkReader>> m_gtkCaches;
};

class Q_GUI_EXPORT QIconLoader
{
public:
    QIconLoader();
    QThemeIconInfo loadIcon(const QString &iconName) const;
    uint themeKey() const { return m_themeKey; }

    QString themeName() const;
    void setThemeName(const QString &themeName);
    QString fallbackThemeName() const;
    void setFallbackThemeName(const QString &themeName);
    QIconTheme theme() { return themeList.value(themeName()); }
    void setThemeSearchPath(const QStringList &searchPaths);
    QStringList themeSearchPaths() const;
    void setFallbackSearchPaths(const QStringList &searchPaths);
    QStringList fallbackSearchPaths() const;
    QIconDirInfo dirInfo(int dirindex);
    static QIconLoader *instance();
    void updateSystemTheme();
    void invalidateKey();
    void ensureInitialized();
    bool hasUserTheme() const { return !m_userTheme.isEmpty(); }

private:
    QThemeIconInfo findIconHelper(const QString &themeName,
                                  const QString &iconName,
                                  QStringList &visited) const;
    QThemeIconInfo lookupFallbackIcon(const QString &iconName) const;

    uint m_themeKey;
    bool m_supportsSvg;
    bool m_initialized;

    mutable QString m_userTheme;
    mutable QString m_userFallbackTheme;
    mutable QString m_systemTheme;
    mutable QStringList m_iconDirs;
    mutable QHash <QString, QIconTheme> themeList;
    mutable QStringList m_fallbackDirs;
};

QT_END_NAMESPACE

#endif // QT_NO_ICON

#endif // QICONLOADER_P_H
