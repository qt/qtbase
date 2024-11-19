// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSDIALOGHELPER_H
#define QWINDOWSDIALOGHELPER_H

#include <QtCore/qt_windows.h>
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformtheme.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

class QFileDialog;
class QDialog;
class QThread;
class QWindowsNativeDialogBase;

namespace QWindowsDialogs
{
    void eatMouseMove();

    bool useHelper(QPlatformTheme::DialogType type);
    QPlatformDialogHelper *createHelper(QPlatformTheme::DialogType type);
} // namespace QWindowsDialogs

template <class BaseClass>
class QWindowsDialogHelperBase : public BaseClass
{
    Q_DISABLE_COPY_MOVE(QWindowsDialogHelperBase)
public:
    using QWindowsNativeDialogBasePtr = QSharedPointer<QWindowsNativeDialogBase>;
    ~QWindowsDialogHelperBase();

    void exec() override;
    bool show(Qt::WindowFlags windowFlags,
              Qt::WindowModality windowModality,
              QWindow *parent) override;
    void hide() override;

    virtual bool supportsNonModalDialog(const QWindow * /* parent */ = nullptr) const { return true; }

protected:
    QWindowsDialogHelperBase() = default;
    QWindowsNativeDialogBase *nativeDialog() const;
    inline bool hasNativeDialog() const { return !m_nativeDialog.isNull(); }
    void timerEvent(QTimerEvent *) override;

private:
    virtual QWindowsNativeDialogBase *createNativeDialog() = 0;
    inline QWindowsNativeDialogBase *ensureNativeDialog();
    inline void startDialogThread();
    inline void stopTimer();
    void cleanupThread();

    QWindowsNativeDialogBasePtr m_nativeDialog;
    HWND m_ownerWindow = nullptr;
    QBasicTimer m_timer;
    QThread *m_thread = nullptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIALOGHELPER_H
