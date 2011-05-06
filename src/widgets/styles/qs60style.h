/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QS60STYLE_H
#define QS60STYLE_H

#include <QtGui/qcommonstyle.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

//Public custom pixel metrics values.
//These can be used to fetch custom pixel metric value from outside QS60Style.
enum {
    PM_FrameCornerWidth = QStyle::PM_CustomBase + 1,
    PM_FrameCornerHeight,
    PM_BoldLineWidth,
    PM_ThinLineWidth,
    PM_MessageBoxHeight
    };

class QS60StylePrivate;

class Q_GUI_EXPORT QS60Style : public QCommonStyle
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QS60Style)

public:
    QS60Style();
    ~QS60Style();

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = 0) const;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = 0) const;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = 0) const;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &contentsSize, const QWidget *w = 0) const;
    int styleHint(StyleHint sh, const QStyleOption *opt = 0, const QWidget *w = 0,
                  QStyleHintReturn *shret = 0) const;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl scontrol, const QWidget *widget = 0) const;
    QRect subElementRect(SubElement element, const QStyleOption *opt, const QWidget *widget = 0) const;
    void polish(QWidget *widget);
    void unpolish(QWidget *widget);
    void polish(QApplication *application);
    void unpolish(QApplication *application);
#ifndef Q_NO_USING_KEYWORD
    using QCommonStyle::polish;
#endif
    bool event(QEvent *e);

#ifndef Q_OS_SYMBIAN
    static QStringList partKeys();
    static QStringList colorListKeys();
    void setS60Theme(const QHash<QString, QPicture> &parts,
        const QHash<QPair<QString , int>, QColor> &colors);
    bool loadS60ThemeFromBlob(const QString &blobFile);
    bool saveS60ThemeToBlob(const QString &blobFile) const;
#endif // !Q_OS_SYMBIAN

protected Q_SLOTS:
    QIcon standardIconImplementation(
            StandardPixmap standardIcon, const QStyleOption * option = 0, const QWidget * widget = 0 ) const;

protected:
    void timerEvent(QTimerEvent *event);
    bool eventFilter(QObject *o, QEvent *e);
private:
    Q_DISABLE_COPY(QS60Style)
    friend class QStyleFactory;
    friend class QApplicationPrivate;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QS60STYLE_H
