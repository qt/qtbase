// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QIOSCOLORDIALOG_H
#define QIOSCOLORDIALOG_H

#include <QtCore/qeventloop.h>
#include <qpa/qplatformdialoghelper.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QIOSColorDialogController);

QT_BEGIN_NAMESPACE

class QIOSColorDialog : public QPlatformColorDialogHelper
{
public:
    QIOSColorDialog();
    ~QIOSColorDialog();

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

    void setCurrentColor(const QColor&) override;
    QColor currentColor() const override;

    void updateColor(const QColor&);

private:
    QEventLoop m_eventLoop;
    QIOSColorDialogController *m_viewController;
    QColor m_currentColor;
};

QT_END_NAMESPACE

#endif // QIOSCOLORDIALOG_H

