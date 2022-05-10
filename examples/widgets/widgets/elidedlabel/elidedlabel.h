// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QFrame>
#include <QString>

//! [0]
class ElidedLabel : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool isElided READ isElided)

public:
    explicit ElidedLabel(const QString &text, QWidget *parent = nullptr);

    void setText(const QString &text);
    const QString & text() const { return content; }
    bool isElided() const { return elided; }

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void elisionChanged(bool elided);

private:
    bool elided;
    QString content;
};
//! [0]

#endif // TEXTWRAPPINGWIDGET_H
