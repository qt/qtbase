// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "textprogressbar.h"

#include <QByteArray>

#include <cstdio>

using namespace std;

void TextProgressBar::clear()
{
    printf("\n");
    fflush(stdout);

    value = 0;
    maximum = -1;
    iteration = 0;
}

void TextProgressBar::update()
{
    ++iteration;

    if (maximum > 0) {
        // we know the maximum
        // draw a progress bar
        int percent = value * 100 / maximum;
        int hashes = percent / 2;

        QByteArray progressbar(hashes, '#');
        if (percent % 2)
            progressbar += '>';

        printf("\r[%-50s] %3d%% %s     ",
               progressbar.constData(),
               percent,
               qPrintable(message));
    } else {
        // we don't know the maximum, so we can't draw a progress bar
        int center = (iteration % 48) + 1; // 50 spaces, minus 2
        QByteArray before(qMax(center - 2, 0), ' ');
        QByteArray after(qMin(center + 2, 50), ' ');

        printf("\r[%s###%s]      %s      ",
               before.constData(), after.constData(), qPrintable(message));
    }
}

void TextProgressBar::setMessage(const QString &m)
{
    message = m;
}

void TextProgressBar::setStatus(qint64 val, qint64 max)
{
    value = val;
    maximum = max;
}
