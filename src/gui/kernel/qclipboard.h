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

#ifndef QCLIPBOARD_H
#define QCLIPBOARD_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_CLIPBOARD

class QMimeData;
class QImage;
class QPixmap;

class Q_GUI_EXPORT QClipboard : public QObject
{
    Q_OBJECT
private:
    explicit QClipboard(QObject *parent);
    ~QClipboard();

public:
    enum Mode { Clipboard, Selection, FindBuffer, LastMode = FindBuffer };

    void clear(Mode mode = Clipboard);

    bool supportsSelection() const;
    bool supportsFindBuffer() const;

    bool ownsSelection() const;
    bool ownsClipboard() const;
    bool ownsFindBuffer() const;

    QString text(Mode mode = Clipboard) const;
    QString text(QString& subtype, Mode mode = Clipboard) const;
    void setText(const QString &, Mode mode = Clipboard);

    const QMimeData *mimeData(Mode mode = Clipboard ) const;
    void setMimeData(QMimeData *data, Mode mode = Clipboard);

    QImage image(Mode mode = Clipboard) const;
    QPixmap pixmap(Mode mode = Clipboard) const;
    void setImage(const QImage &, Mode mode  = Clipboard);
    void setPixmap(const QPixmap &, Mode mode  = Clipboard);

Q_SIGNALS:
    void changed(QClipboard::Mode mode);
    void selectionChanged();
    void findBufferChanged();
    void dataChanged();

protected:
    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QGuiApplication;
    friend class QBaseApplication;
    friend class QDragManager;
    friend class QPlatformClipboard;

private:
    Q_DISABLE_COPY(QClipboard)

    bool supportsMode(Mode mode) const;
    bool ownsMode(Mode mode) const;
    void emitChanged(Mode mode);
};

#endif // QT_NO_CLIPBOARD

QT_END_NAMESPACE

#endif // QCLIPBOARD_H
