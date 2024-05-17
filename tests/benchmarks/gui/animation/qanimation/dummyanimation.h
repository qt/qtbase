// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>

#ifndef _DUMMYANIMATION_H__

class DummyObject;

class DummyAnimation : public QVariantAnimation
{
public:
    DummyAnimation(DummyObject *d);

    void updateCurrentValue(const QVariant &value) override;
    void updateState(State newstate, State oldstate) override;

private:
    DummyObject *m_dummy;
};

#endif
