// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef LTTNG_H
#define LTTNG_H

struct Provider;
class QFile;

void writeLttng(QFile &device, const Provider &p);

#endif // LTTNG_H
