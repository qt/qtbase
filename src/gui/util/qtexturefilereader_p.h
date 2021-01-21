/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
****************************************************************************/

#ifndef QTEXTUREFILEREADER_H
#define QTEXTUREFILEREADER_H

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

#include "qtexturefiledata_p.h"
#include <QString>
#include <QFileInfo>

QT_BEGIN_NAMESPACE

class QIODevice;
class QTextureFileHandler;

class Q_GUI_EXPORT QTextureFileReader
{
public:
    QTextureFileReader(QIODevice *device, const QString &fileName = QString());  //### drop this logname thing?
    ~QTextureFileReader();

    bool canRead();
    QTextureFileData read();

    // TBD access function to params
    // TBD ask for identified fmt

    static QList<QByteArray> supportedFileFormats();

private:
    bool init();
    QIODevice *m_device = nullptr;
    QString m_fileName;
    QTextureFileHandler *m_handler = nullptr;
    bool checked = false;
};

QT_END_NAMESPACE


#endif // QTEXTUREFILEREADER_H
