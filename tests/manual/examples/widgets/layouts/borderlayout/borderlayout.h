// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BORDERLAYOUT_H
#define BORDERLAYOUT_H

#include <QLayout>
#include <QRect>

class BorderLayout : public QLayout
{
public:
    enum Position { West, North, South, East, Center };

    explicit BorderLayout(QWidget *parent, const QMargins &margins = QMargins(), int spacing = -1);
    BorderLayout(int spacing = -1);
    ~BorderLayout();

    void addItem(QLayoutItem *item) override;
    void addWidget(QWidget *widget, Position position);
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

    void add(QLayoutItem *item, Position position);

private:
    struct ItemWrapper
    {
        ItemWrapper(QLayoutItem *i, Position p) {
            item = i;
            position = p;
        }

        QLayoutItem *item;
        Position position;
    };

    enum SizeType { MinimumSize, SizeHint };
    QSize calculateSize(SizeType sizeType) const;

    QList<ItemWrapper *> list;
};

#endif // BORDERLAYOUT_H
