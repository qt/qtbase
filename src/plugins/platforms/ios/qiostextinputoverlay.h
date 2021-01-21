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

#ifndef QIOSTEXTEDITOVERLAY_H
#define QIOSTEXTEDITOVERLAY_H

#include <QtCore/QObject>

Q_FORWARD_DECLARE_OBJC_CLASS(QIOSEditMenu);
Q_FORWARD_DECLARE_OBJC_CLASS(QIOSCursorRecognizer);
Q_FORWARD_DECLARE_OBJC_CLASS(QIOSSelectionRecognizer);
Q_FORWARD_DECLARE_OBJC_CLASS(QIOSTapRecognizer);

QT_BEGIN_NAMESPACE

class QIOSTextInputOverlay : public QObject
{
public:
    QIOSTextInputOverlay();
    ~QIOSTextInputOverlay();

    static QIOSEditMenu *s_editMenu;

private:
    QIOSCursorRecognizer *m_cursorRecognizer;
    QIOSSelectionRecognizer *m_selectionRecognizer;
    QIOSTapRecognizer *m_openMenuOnTapRecognizer;

    void updateFocusObject();
};

QT_END_NAMESPACE

#endif
