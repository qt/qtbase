// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMOFFSCREENSURFACE_H
#define QWASMOFFSCREENSURFACE_H

#include <qpa/qplatformoffscreensurface.h>

#include <emscripten/val.h>

#include <string>

QT_BEGIN_NAMESPACE

class QOffscreenSurface;
class QWasmOffscreenSurface final : public QPlatformOffscreenSurface
{
public:
    explicit QWasmOffscreenSurface(QOffscreenSurface *offscreenSurface);
    ~QWasmOffscreenSurface() final;

    const std::string &id() const { return m_specialTargetId; }

private:
    std::string m_specialTargetId;
    emscripten::val m_offscreenCanvas;
};

QT_END_NAMESPACE

#endif
