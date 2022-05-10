// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DRAGWIDGET_H
#define DRAGWIDGET_H

#include <QByteArray>
#include <QFrame>
#include <QString>
#include <QStringList>

class QComboBox;
class QFrame;
class QLabel;
class QTextBrowser;

class DragWidget : public QFrame
{
    Q_OBJECT

public:
    explicit DragWidget(QWidget *parent = nullptr);
    void setData(const QString &mimetype, const QByteArray &newData);

signals:
    void dragResult(const QString &actionText);
    void mimeTypes(const QStringList &types);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QByteArray data;
    QLabel *dragDropLabel;
    QPoint dragStartPosition;
    QString mimeType;
};

#endif
