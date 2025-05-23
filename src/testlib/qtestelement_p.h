// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QTESTELEMENT_P_H
#define QTESTELEMENT_P_H

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

#include <QtTest/qttestglobal.h>
#include <QtTest/private/qtestcoreelement_p.h>

QT_BEGIN_NAMESPACE


class QTestElement : public QTestCoreElement<QTestElement>
{
    public:
        QTestElement(QTest::LogElementType type = QTest::LET_Undefined);
        ~QTestElement();

        bool addChild(QTestElement *element);
        const std::vector<QTestElement*> &childElements() const;

        const QTestElement *parentElement() const;
        void setParent(const QTestElement *p);

    private:
        std::vector<QTestElement*> listOfChildren;
        const QTestElement *parent = nullptr;

};

QT_END_NAMESPACE

#endif
