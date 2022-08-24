// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef VIEW_H
#define VIEW_H

#include <QWidget>
#include <QTextOption>

class QTextLayout;
class View : public QWidget
{
    Q_OBJECT
public:
    explicit View(QWidget *parent = nullptr);
    ~View() override;

    void updateLayout(const QString &sourceString,
                      float width,
                      QTextOption::WrapMode mode,
                      const QFont &font);

    QSize sizeHint() const override;

    QTextLayout *layout() const { return m_layout; }

public slots:
    void setVisualizedBounds(const QRegion &region)
    {
        m_bounds = region;
        update();
    }

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    QTextLayout *m_layout = nullptr;
    QRegion m_bounds;
};

#endif // VIEW_H
