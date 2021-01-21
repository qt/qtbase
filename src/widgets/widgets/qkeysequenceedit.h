/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2013 Ivan Komissarov.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QKEYSEQUENCEEDIT_H
#define QKEYSEQUENCEEDIT_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

QT_REQUIRE_CONFIG(keysequenceedit);

QT_BEGIN_NAMESPACE

class QKeySequenceEditPrivate;
class Q_WIDGETS_EXPORT QKeySequenceEdit : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QKeySequence keySequence READ keySequence WRITE setKeySequence NOTIFY keySequenceChanged USER true)

public:
    explicit QKeySequenceEdit(QWidget *parent = nullptr);
    explicit QKeySequenceEdit(const QKeySequence &keySequence, QWidget *parent = nullptr);
    ~QKeySequenceEdit();

    QKeySequence keySequence() const;

public Q_SLOTS:
    void setKeySequence(const QKeySequence &keySequence);
    void clear();

Q_SIGNALS:
    void editingFinished();
    void keySequenceChanged(const QKeySequence &keySequence);

protected:
    QKeySequenceEdit(QKeySequenceEditPrivate &d, QWidget *parent, Qt::WindowFlags f);

    bool event(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void timerEvent(QTimerEvent *) override;

private:
    Q_DISABLE_COPY(QKeySequenceEdit)
    Q_DECLARE_PRIVATE(QKeySequenceEdit)
};

QT_END_NAMESPACE

#endif // QKEYSEQUENCEEDIT_H
