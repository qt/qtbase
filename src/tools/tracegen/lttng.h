// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef LTTNG_H
#define LTTNG_H

struct Provider;
class QFile;

void writeLttng(QFile &device, const Provider &p);

#endif // LTTNG_H
