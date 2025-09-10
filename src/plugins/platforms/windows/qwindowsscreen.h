// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSSCREEN_H
#define QWINDOWSSCREEN_H

#include "qtwindowsglobal.h"

#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/qscreen_platform.h>

QT_BEGIN_NAMESPACE

struct QWindowsScreenData
{
    enum Flags
    {
        PrimaryScreen = 0x1,
        VirtualDesktop = 0x2,
        LockScreen = 0x4 // Temporary screen existing during user change, etc.
    };

    QRect geometry;
    QRect availableGeometry;
    QDpi dpi{96, 96};
    QSizeF physicalSizeMM;
    int depth = 32;
    QImage::Format format = QImage::Format_ARGB32_Premultiplied;
    unsigned flags = VirtualDesktop;
    QString name;
    QString manufacturer;
    QString model;
    QString serialNumber;
    Qt::ScreenOrientation orientation = Qt::LandscapeOrientation;
    qreal refreshRateHz = 60;
    HMONITOR hMonitor = nullptr;
    QString deviceName;
    QString devicePath;
    std::optional<int> deviceIndex = std::nullopt;
};

class QWindowsScreen : public QPlatformScreen, public QNativeInterface::QWindowsScreen
{
public:
#ifndef QT_NO_CURSOR
    using CursorPtr = QScopedPointer<QPlatformCursor>;
#endif

    explicit QWindowsScreen(const QWindowsScreenData &data);

    QRect geometry() const override { return m_data.geometry; }
    QRect availableGeometry() const override { return m_data.availableGeometry; }
    int depth() const override { return m_data.depth; }
    QImage::Format format() const override { return m_data.format; }
    QSizeF physicalSize() const override { return m_data.physicalSizeMM; }
    QDpi logicalDpi() const override { return m_data.dpi; }
    QDpi logicalBaseDpi() const override { return QDpi(baseDpi, baseDpi); }
    qreal devicePixelRatio() const override { return 1.0; }
    qreal refreshRate() const override { return m_data.refreshRateHz; }
    QString name() const override;
    QString manufacturer() const override { return m_data.manufacturer; }
    QString model() const override { return m_data.model; }
    QString serialNumber() const override { return m_data.serialNumber; }
    Qt::ScreenOrientation orientation() const override { return m_data.orientation; }
    QList<QPlatformScreen *> virtualSiblings() const override;
    QWindow *topLevelAt(const QPoint &point) const override;
    static QWindow *windowAt(const QPoint &point, unsigned flags);

    QPixmap grabWindow(WId window, int qX, int qY, int qWidth, int qHeight) const override;
    QPlatformScreen::SubpixelAntialiasingType subpixelAntialiasingTypeHint() const override;

    static Qt::ScreenOrientation orientationPreference();
    static bool setOrientationPreference(Qt::ScreenOrientation o);

    inline void handleChanges(const QWindowsScreenData &newData);

    HMONITOR handle() const override;

#ifndef QT_NO_CURSOR
    QPlatformCursor *cursor() const override { return m_cursor.data(); }
    const CursorPtr &cursorPtr() const { return m_cursor; }
#else
    QPlatformCursor *cursor() const               { return 0; }
#endif // !QT_NO_CURSOR

    const QWindowsScreenData &data() const  { return m_data; }

    static QRect virtualGeometry(const QPlatformScreen *screen);
    static inline int baseDpi = 96;

private:
    QWindowsScreenData m_data;
#ifndef QT_NO_CURSOR
    const CursorPtr m_cursor;
#endif
};

class QWindowsScreenManager
{
    Q_DISABLE_COPY_MOVE(QWindowsScreenManager)
public:
    using WindowsScreenList = QList<QWindowsScreen *>;

    QWindowsScreenManager();
    void initialize();
    void destroyWindow();
    ~QWindowsScreenManager();

    void clearScreens();

    bool handleScreenChanges();
    const WindowsScreenList &screens() const { return m_screens; }

    const QWindowsScreen *screenAtDp(const QPoint &p) const;
    const QWindowsScreen *screenForHwnd(HWND hwnd) const;

    static bool isSingleScreen();

private:
    void addScreen(const QWindowsScreenData &screenData);
    void removeScreen(int index);

    HWND m_displayChangeObserver = nullptr;
    WindowsScreenList m_screens;
};

QT_END_NAMESPACE

#endif // QWINDOWSSCREEN_H
