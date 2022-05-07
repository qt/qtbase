/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QOUTPUTMAPPING_P_H
#define QOUTPUTMAPPING_P_H

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

#include <QString>
#include <QHash>

QT_BEGIN_NAMESPACE

class QWindow;

class QOutputMapping
{
public:
    virtual ~QOutputMapping() {}

    static QOutputMapping *get();
    virtual bool load();
    virtual QString screenNameForDeviceNode(const QString &deviceNode);

#ifdef Q_OS_WEBOS
    virtual QWindow *windowForDeviceNode(const QString &deviceNode);
    static void set(QOutputMapping *mapping);
#endif
};

class QDefaultOutputMapping : public QOutputMapping
{
public:
    bool load() override;
    QString screenNameForDeviceNode(const QString &deviceNode) override;

private:
    QHash<QString, QString> m_screenTable;
};

QT_END_NAMESPACE

#endif // QOUTPUTMAPPING_P_H
