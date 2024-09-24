// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWINDOWSDRAG_H
#define QWINDOWSDRAG_H

#include <private/qstdweb_p.h>
#include <private/qsimpledrag_p.h>

#include <qpa/qplatformdrag.h>
#include <QtGui/qdrag.h>

#include <memory>

QT_BEGIN_NAMESPACE

struct DragEvent;

class QWasmDrag final : public QSimpleDrag
{
public:
    QWasmDrag();
    ~QWasmDrag() override;
    QWasmDrag(const QWasmDrag &other) = delete;
    QWasmDrag(QWasmDrag &&other) = delete;
    QWasmDrag &operator=(const QWasmDrag &other) = delete;
    QWasmDrag &operator=(QWasmDrag &&other) = delete;

    static QWasmDrag *instance();

    void onNativeDragOver(DragEvent *event);
    void onNativeDrop(DragEvent *event);
    void onNativeDragStarted(DragEvent *event);
    void onNativeDragFinished(DragEvent *event);
    void onNativeDragLeave(DragEvent *event);

    // QPlatformDrag:
    Qt::DropAction drag(QDrag *drag) final;

private:
    struct DragState;

    std::unique_ptr<DragState> m_dragState;
};

QT_END_NAMESPACE

#endif // QWINDOWSDRAG_H
