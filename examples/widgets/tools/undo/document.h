/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QUndoStack)
QT_FORWARD_DECLARE_CLASS(QTextStream)

class Shape
{
public:
    enum Type { Rectangle, Circle, Triangle };

    explicit Shape(Type type = Rectangle, const QColor &color = Qt::red, const QRect &rect = QRect());

    Type type() const;
    QString name() const;
    QRect rect() const;
    QRect resizeHandle() const;
    QColor color() const;

    static QString typeToString(Type type);
    static Type stringToType(const QString &s, bool *ok = 0);

    static const QSize minSize;

private:
    Type m_type;
    QRect m_rect;
    QColor m_color;
    QString m_name;

    friend class Document;
};

class Document : public QWidget
{
    Q_OBJECT

public:
    Document(QWidget *parent = 0);

    QString addShape(const Shape &shape);
    void deleteShape(const QString &shapeName);
    Shape shape(const QString &shapeName) const;
    QString currentShapeName() const;

    void setShapeRect(const QString &shapeName, const QRect &rect);
    void setShapeColor(const QString &shapeName, const QColor &color);

    bool load(QTextStream &stream);
    void save(QTextStream &stream);

    QString fileName() const;
    void setFileName(const QString &fileName);

    QUndoStack *undoStack() const;

signals:
    void currentShapeChanged(const QString &shapeName);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    void setCurrentShape(int index);
    int indexOf(const QString &shapeName) const;
    int indexAt(const QPoint &pos) const;
    QString uniqueName(const QString &name) const;

    QList<Shape> m_shapeList;
    int m_currentIndex;
    int m_mousePressIndex;
    QPoint m_mousePressOffset;
    bool m_resizeHandlePressed;
    QString m_fileName;

    QUndoStack *m_undoStack;
};

#endif // DOCUMENT_H
