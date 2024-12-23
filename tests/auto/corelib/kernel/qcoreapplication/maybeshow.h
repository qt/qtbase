// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#if defined(QT_WIDGETS_LIB)
#include <QtWidgets/QDialog>
#elif defined(QT_GUI_LIB)
#include <QtGui/QWindow>
#endif

inline auto maybeShowSomething()
{
#if defined(QT_WIDGETS_LIB)
    auto w = std::make_unique<QDialog>();
    w->setModal(true);
    w->show();
#elif defined(QT_GUI_LIB)
    auto w = std::make_unique<QWindow>();
    w->setModality(Qt::ApplicationModal);
    w->show();
#else
    void *w = nullptr;
#endif
    return w;
}

