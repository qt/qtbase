/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESTLITECLIPBOARD_H
#define QTESTLITECLIPBOARD_H

#include <QPlatformClipboard>
#include "qxlibstatic.h"

class QXlibScreen;
class QXlibClipboard : public QPlatformClipboard
{
public:
    QXlibClipboard(QXlibScreen *screen);

    const QMimeData *mimeData(QClipboard::Mode mode) const;
    void setMimeData(QMimeData *data, QClipboard::Mode mode);

    bool supportsMode(QClipboard::Mode mode) const;

    QXlibScreen *screen() const;

    Window requestor() const;
    void setRequestor(Window window);

    Window owner() const;

    void handleSelectionRequest(XEvent *event);

    bool clipboardReadProperty(Window win, Atom property, bool deleteProperty, QByteArray *buffer, int *size, Atom *type, int *format) const;
    QByteArray clipboardReadIncrementalProperty(Window win, Atom property, int nbytes, bool nullterm);

    QByteArray getDataInFormat(Atom modeAtom, Atom fmtatom);

private:
    void setOwner(Window window);

    Atom sendTargetsSelection(QMimeData *d, Window window, Atom property);
    Atom sendSelection(QMimeData *d, Atom target, Window window, Atom property);

    QXlibScreen *m_screen;

    QMimeData *m_xClipboard;
    QMimeData *m_clientClipboard;

    QMimeData *m_xSelection;
    QMimeData *m_clientSelection;

    Window m_requestor;
    Window m_owner;

    static const int clipboard_timeout;

};

#endif // QTESTLITECLIPBOARD_H
