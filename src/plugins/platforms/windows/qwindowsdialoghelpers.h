/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QWINDOWSDIALOGHELPER_H
#define QWINDOWSDIALOGHELPER_H

#include <QtCore/qt_windows.h>
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformtheme.h>
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
    ~QWindowsDialogHelperBase() { cleanupThread(); }

    void exec() override;
    bool show(Qt::WindowFlags windowFlags,
              Qt::WindowModality windowModality,
              QWindow *parent) override;
    void hide() override;

    virtual bool supportsNonModalDialog(const QWindow * /* parent */ = nullptr) const { return true; }

protected:
    QWindowsDialogHelperBase() = default;
    QWindowsNativeDialogBase *nativeDialog() const;
    inline bool hasNativeDialog() const { return m_nativeDialog; }
    void timerEvent(QTimerEvent *) override;

private:
    virtual QWindowsNativeDialogBase *createNativeDialog() = 0;
    inline QWindowsNativeDialogBase *ensureNativeDialog();
    inline void startDialogThread();
    inline void stopTimer();
    void cleanupThread();

    QWindowsNativeDialogBasePtr m_nativeDialog;
    HWND m_ownerWindow = nullptr;
    int m_timerId = 0;
    QThread *m_thread = nullptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIALOGHELPER_H
