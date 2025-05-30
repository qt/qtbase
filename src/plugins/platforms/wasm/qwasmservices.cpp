// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmservices.h"
#include "qwasmstring.h"

#include <QtCore/QUrl>
#include <QtCore/QDebug>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

bool QWasmServices::openUrl(const QUrl &url)
{
    emscripten::val jsUrl = QWasmString::fromQString(url.toString());
    emscripten::val::global("window").call<void>("open", jsUrl, emscripten::val("_blank"));
    return true;
}

QT_END_NAMESPACE
