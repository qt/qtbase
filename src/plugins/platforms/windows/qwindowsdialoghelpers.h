/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSDIALOGHELPER_H
#define QWINDOWSDIALOGHELPER_H

#include <QtCore/qt_windows.h>
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformtheme.h>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>

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
    Q_DISABLE_COPY(QWindowsDialogHelperBase)
public:
    typedef QSharedPointer<QWindowsNativeDialogBase> QWindowsNativeDialogBasePtr;
    ~QWindowsDialogHelperBase() { cleanupThread(); }

    void exec() override;
    bool show(Qt::WindowFlags windowFlags,
              Qt::WindowModality windowModality,
              QWindow *parent) override;
    void hide() override;

    virtual bool supportsNonModalDialog(const QWindow * /* parent */ = 0) const { return true; }

protected:
    QWindowsDialogHelperBase() {}
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
    HWND m_ownerWindow = 0;
    int m_timerId = 0;
    QThread *m_thread = nullptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIALOGHELPER_H
