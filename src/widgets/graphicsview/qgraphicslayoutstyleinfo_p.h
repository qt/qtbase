/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#ifndef QGRAPHICSLAYOUTSTYLEINFO_P_H
#define QGRAPHICSLAYOUTSTYLEINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtGui/private/qabstractlayoutstyleinfo_p.h>
#include <QtWidgets/qstyleoption.h>

#include <memory>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QStyle;
class QWidget;
class QGraphicsLayoutPrivate;

class QGraphicsLayoutStyleInfo : public QAbstractLayoutStyleInfo
{
public:
    QGraphicsLayoutStyleInfo(const QGraphicsLayoutPrivate *layout);
    ~QGraphicsLayoutStyleInfo();

    virtual qreal combinedLayoutSpacing(QLayoutPolicy::ControlTypes controls1,
                                        QLayoutPolicy::ControlTypes controls2,
                                        Qt::Orientation orientation) const override;

    virtual qreal perItemSpacing(QLayoutPolicy::ControlType control1,
                                 QLayoutPolicy::ControlType control2,
                                 Qt::Orientation orientation) const override;

    virtual qreal spacing(Qt::Orientation orientation) const override;

    virtual qreal windowMargin(Qt::Orientation orientation) const override;

    virtual void invalidate() override
    {
        m_style = nullptr;
        QAbstractLayoutStyleInfo::invalidate();
    }

    QWidget *widget() const;
    QStyle *style() const;

private:
    const QGraphicsLayoutPrivate *m_layout;
    mutable QStyle *m_style;
    QStyleOption m_styleOption;
    std::unique_ptr<QWidget> m_widget;
};

QT_END_NAMESPACE

#endif // QGRAPHICSLAYOUTSTYLEINFO_P_H
