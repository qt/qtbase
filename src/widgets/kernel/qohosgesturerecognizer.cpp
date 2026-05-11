// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qevent.h"
#include "qgesture.h"
#include "qohosgesturerecognizer_p.h"
#include <QtCore/qcoreapplication.h>

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

class QOhosPinchGestureRecognizer : public QGestureRecognizer
{
public:
    QGesture *create(QObject *target) override;
    QGestureRecognizer::Result
    recognize(QGesture *gesture, QObject *watched, QEvent *event) override;
    void reset(QGesture *gesture) override;
};

QGesture *QOhosPinchGestureRecognizer::create(QObject *)
{
    return new QPinchGesture;
}

QGestureRecognizer::Result
QOhosPinchGestureRecognizer::recognize(QGesture *gesture, QObject *, QEvent *event)
{
    auto *g = static_cast<QPinchGesture *>(gesture);

    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;
    if (event->type() == QEvent::NativeGesture) {
        const auto *ev = static_cast<QNativeGestureEvent *>(event);

        switch (ev->gestureType()) {
        case Qt::BeginNativeGesture:
            g->setStartCenterPoint(ev->globalPosition().toPoint());
            result = QGestureRecognizer::MayBeGesture;
            reset(g);
            break;
        case Qt::EndNativeGesture:
            result = (g->state() != Qt::NoGesture)
                ? QGestureRecognizer::FinishGesture
                : QGestureRecognizer::CancelGesture;
            break;
        case Qt::ZoomNativeGesture:
            g->setChangeFlags({});
            g->setHotSpot(ev->globalPosition());

            if (g->centerPoint() != ev->globalPosition().toPoint()) {
                g->setLastCenterPoint(g->centerPoint());
                g->setCenterPoint(ev->globalPosition().toPoint());
                g->setChangeFlags(g->changeFlags() | QPinchGesture::CenterPointChanged);
            }

            if (g->scaleFactor() != ev->value()) {
                g->setLastScaleFactor(g->scaleFactor());
                g->setScaleFactor(ev->value());
                g->setTotalScaleFactor(g->totalScaleFactor() * g->scaleFactor());
                g->setChangeFlags(g->changeFlags() | QPinchGesture::ScaleFactorChanged);
            }

            if (g->changeFlags() != 0)
                result = QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
            break;
        case Qt::RotateNativeGesture:
            g->setChangeFlags({});
            g->setHotSpot(ev->globalPosition());

            if (g->centerPoint() != ev->globalPosition().toPoint()) {
                g->setLastCenterPoint(g->centerPoint());
                g->setCenterPoint(ev->globalPosition().toPoint());
                g->setChangeFlags(g->changeFlags() | QPinchGesture::CenterPointChanged);
            }

            if (g->rotationAngle() != ev->value()) {
                g->setLastRotationAngle(g->rotationAngle());
                g->setRotationAngle(ev->value());
                g->setTotalRotationAngle(g->rotationAngle());
                g->setChangeFlags(g->changeFlags() | QPinchGesture::RotationAngleChanged);
            }

            if (g->changeFlags() != 0)
                result = QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
            break;
        default:
            break;
        };
    }

    g->setTotalChangeFlags(g->totalChangeFlags() |= g->changeFlags());

    return result;
}

void QOhosPinchGestureRecognizer::reset(QGesture *gesture)
{
    auto *g = static_cast<QPinchGesture *>(gesture);
    g->setChangeFlags({});
    g->setTotalChangeFlags({});
    g->setScaleFactor(1.0f);
    g->setTotalScaleFactor(1.0f);
    g->setLastScaleFactor(1.0f);
    g->setRotationAngle(0.0f);
    g->setTotalRotationAngle(0.0f);
    g->setLastRotationAngle(0.0f);
    g->setCenterPoint(QPointF());
    g->setStartCenterPoint(QPointF());
    g->setLastCenterPoint(QPointF());
    QGestureRecognizer::reset(gesture);
}

std::unique_ptr<QGestureRecognizer> makeQOhosPinchGestureRecognizer()
{
    return std::make_unique<QOhosPinchGestureRecognizer>();
}

QT_END_NAMESPACE

#endif // QT_NO_GESTURES
