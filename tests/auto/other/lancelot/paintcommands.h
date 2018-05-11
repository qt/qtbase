/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef PAINTCOMMANDS_H
#define PAINTCOMMANDS_H

#include <qcolor.h>
#include <qmap.h>
#include <qpainterpath.h>
#include <qregion.h>
#include <qregularexpression.h>
#include <qstringlist.h>
#include <qpixmap.h>
#include <qbrush.h>
#include <qhash.h>

QT_FORWARD_DECLARE_CLASS(QPainter)
#ifndef QT_NO_OPENGL
QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)
QT_FORWARD_DECLARE_CLASS(QOpenGLPaintDevice)
QT_FORWARD_DECLARE_CLASS(QOpenGLContext)
#endif

enum DeviceType {
        WidgetType,
        BitmapType,
        PixmapType,
        ImageType,
        ImageMonoType,
        OpenGLType,
        OpenGLBufferType,
        PictureType,
        PrinterType,
        PdfType,
        PsType,
        GrabType,
        CustomDeviceType,
        CustomWidgetType,
        ImageWidgetType
};

/************************************************************************/
class PaintCommands
{
public:
    // construction / initialization
    PaintCommands(const QStringList &cmds, int w, int h, QImage::Format format)
        : m_painter(0)
        , m_surface_painter(0)
        , m_format(format)
        , m_commands(cmds)
        , m_gradientSpread(QGradient::PadSpread)
        , m_gradientCoordinate(QGradient::LogicalMode)
        , m_width(w)
        , m_height(h)
        , m_verboseMode(false)
        , m_type(WidgetType)
        , m_checkers_background(true)
        , m_shouldDrawText(true)
#ifndef QT_NO_OPENGL
        , m_default_glcontext(0)
        , m_surface_glcontext(0)
        , m_surface_glbuffer(0)
        , m_surface_glpaintdevice(0)
#endif
    { staticInit(); }

public:
    void setCheckersBackground(bool b) { staticInit(); m_checkers_background = b; }
    void setContents(const QStringList &cmds) {
        staticInit();
        m_blockMap.clear();
        m_pathMap.clear();
        m_pixmapMap.clear();
        m_imageMap.clear();
        m_regionMap.clear();
        m_gradientStops.clear();
        m_controlPoints.clear();
        m_gradientSpread = QGradient::PadSpread;
        m_gradientCoordinate = QGradient::LogicalMode;
        m_commands = cmds;


    }
    void setPainter(QPainter *pt) { staticInit(); m_painter = pt; }
    void setType(DeviceType t) { staticInit(); m_type = t; }
    void setFilePath(const QString &path) { staticInit(); m_filepath = path; }
    void setControlPoints(const QVector<QPointF> &points) { staticInit(); m_controlPoints = points; }
    void setVerboseMode(bool v) { staticInit(); m_verboseMode = v; }
    void insertAt(int commandIndex, const QStringList &newCommands);
    void setShouldDrawText(bool drawText) { m_shouldDrawText = drawText; }

    // run
    void runCommands();

private:
    // run
    void runCommand(const QString &scriptLine);

    // conversion methods
    int convertToInt(const QString &str);
    double convertToDouble(const QString &str);
    float convertToFloat(const QString &str);
    QColor convertToColor(const QString &str);

    // commands: comments
    void command_comment(QRegularExpressionMatch re);

    // commands: importer
    void command_import(QRegularExpressionMatch re);

    // commands: blocks
    void command_begin_block(QRegularExpressionMatch re);
    void command_end_block(QRegularExpressionMatch re);
    void command_repeat_block(QRegularExpressionMatch re);

    // commands: misc
    void command_textlayout_draw(QRegularExpressionMatch re);
    void command_abort(QRegularExpressionMatch re);

    // commands: noops
    void command_noop(QRegularExpressionMatch re);

    // commands: setters
    void command_setBgMode(QRegularExpressionMatch re);
    void command_setBackground(QRegularExpressionMatch re);
    void command_setOpacity(QRegularExpressionMatch re);
    void command_path_setFillRule(QRegularExpressionMatch re);
    void command_setBrush(QRegularExpressionMatch re);
    void command_setBrushOrigin(QRegularExpressionMatch re);
    void command_brushTranslate(QRegularExpressionMatch re);
    void command_brushRotate(QRegularExpressionMatch re);
    void command_brushScale(QRegularExpressionMatch re);
    void command_brushShear(QRegularExpressionMatch re);
    void command_setClipPath(QRegularExpressionMatch re);
    void command_setClipRect(QRegularExpressionMatch re);
    void command_setClipRectangle(QRegularExpressionMatch re);
    void command_setClipRegion(QRegularExpressionMatch re);
    void command_setClipping(QRegularExpressionMatch re);
    void command_setCompositionMode(QRegularExpressionMatch re);
    void command_setFont(QRegularExpressionMatch re);
    void command_setPen(QRegularExpressionMatch re);
    void command_setPen2(QRegularExpressionMatch re);
    void command_pen_setDashOffset(QRegularExpressionMatch re);
    void command_pen_setDashPattern(QRegularExpressionMatch re);
    void command_pen_setCosmetic(QRegularExpressionMatch re);
    void command_setRenderHint(QRegularExpressionMatch re);
    void command_clearRenderHint(QRegularExpressionMatch re);
    void command_gradient_appendStop(QRegularExpressionMatch re);
    void command_gradient_clearStops(QRegularExpressionMatch re);
    void command_gradient_setConical(QRegularExpressionMatch re);
    void command_gradient_setLinear(QRegularExpressionMatch re);
    void command_gradient_setRadial(QRegularExpressionMatch re);
    void command_gradient_setRadialExtended(QRegularExpressionMatch re);
    void command_gradient_setLinearPen(QRegularExpressionMatch re);
    void command_gradient_setSpread(QRegularExpressionMatch re);
    void command_gradient_setCoordinateMode(QRegularExpressionMatch re);

    // commands: drawing ops
    void command_drawArc(QRegularExpressionMatch re);
    void command_drawChord(QRegularExpressionMatch re);
    void command_drawConvexPolygon(QRegularExpressionMatch re);
    void command_drawEllipse(QRegularExpressionMatch re);
    void command_drawImage(QRegularExpressionMatch re);
    void command_drawLine(QRegularExpressionMatch re);
    void command_drawPath(QRegularExpressionMatch re);
    void command_drawPie(QRegularExpressionMatch re);
    void command_drawPixmap(QRegularExpressionMatch re);
    void command_drawPoint(QRegularExpressionMatch re);
    void command_drawPolygon(QRegularExpressionMatch re);
    void command_drawPolyline(QRegularExpressionMatch re);
    void command_drawRect(QRegularExpressionMatch re);
    void command_drawRoundedRect(QRegularExpressionMatch re);
    void command_drawRoundRect(QRegularExpressionMatch re);
    void command_drawText(QRegularExpressionMatch re);
    void command_drawStaticText(QRegularExpressionMatch re);
    void command_drawTiledPixmap(QRegularExpressionMatch re);
    void command_path_addEllipse(QRegularExpressionMatch re);
    void command_path_addPolygon(QRegularExpressionMatch re);
    void command_path_addRect(QRegularExpressionMatch re);
    void command_path_addText(QRegularExpressionMatch re);
    void command_path_arcTo(QRegularExpressionMatch re);
    void command_path_closeSubpath(QRegularExpressionMatch re);
    void command_path_createOutline(QRegularExpressionMatch re);
    void command_path_cubicTo(QRegularExpressionMatch re);
    void command_path_debugPrint(QRegularExpressionMatch re);
    void command_path_lineTo(QRegularExpressionMatch re);
    void command_path_moveTo(QRegularExpressionMatch re);
    void command_region_addEllipse(QRegularExpressionMatch re);
    void command_region_addRect(QRegularExpressionMatch re);

    // getters
    void command_region_getClipRegion(QRegularExpressionMatch re);
    void command_path_getClipPath(QRegularExpressionMatch re);

    // commands: surface begin/end
    void command_surface_begin(QRegularExpressionMatch re);
    void command_surface_end(QRegularExpressionMatch re);

    // commands: save/restore painter state
    void command_restore(QRegularExpressionMatch re);
    void command_save(QRegularExpressionMatch re);

    // commands: pixmap/image
    void command_pixmap_load(QRegularExpressionMatch re);
    void command_pixmap_setMask(QRegularExpressionMatch re);
    void command_bitmap_load(QRegularExpressionMatch re);
    void command_image_convertToFormat(QRegularExpressionMatch re);
    void command_image_load(QRegularExpressionMatch re);
    void command_image_setColor(QRegularExpressionMatch re);
    void command_image_setColorCount(QRegularExpressionMatch re);

    // commands: transformation
    void command_resetMatrix(QRegularExpressionMatch re);
    void command_translate(QRegularExpressionMatch re);
    void command_rotate(QRegularExpressionMatch re);
    void command_rotate_x(QRegularExpressionMatch re);
    void command_rotate_y(QRegularExpressionMatch re);
    void command_scale(QRegularExpressionMatch re);
    void command_mapQuadToQuad(QRegularExpressionMatch re);
    void command_setMatrix(QRegularExpressionMatch re);

    // attributes
    QPainter *m_painter;
    QPainter *m_surface_painter;
    QImage::Format m_format;
    QImage m_surface_image;
    QRectF m_surface_rect;
    QStringList m_commands;
    QString m_currentCommand;
    int m_currentCommandIndex;
    QString m_filepath;
    QMap<QString, QStringList> m_blockMap;
    QMap<QString, QPainterPath> m_pathMap;
    QMap<QString, QPixmap> m_pixmapMap;
    QMap<QString, QImage> m_imageMap;
    QMap<QString, QRegion> m_regionMap;
    QGradientStops m_gradientStops;
    QGradient::Spread m_gradientSpread;
    QGradient::CoordinateMode m_gradientCoordinate;
    bool m_abort;
    int m_width;
    int m_height;

    bool m_verboseMode;
    DeviceType m_type;
    bool m_checkers_background;
    bool m_shouldDrawText;

    QVector<QPointF> m_controlPoints;

#ifndef QT_NO_OPENGL
    QOpenGLContext *m_default_glcontext;
    QOpenGLContext *m_surface_glcontext;
    QOpenGLFramebufferObject *m_surface_glbuffer;
    QOpenGLPaintDevice *m_surface_glpaintdevice;
#endif

    // painter functionalities string tables
    static const char *brushStyleTable[];
    static const char *penStyleTable[];
    static const char *fontWeightTable[];
    static const char *fontHintingTable[];
    static const char *clipOperationTable[];
    static const char *spreadMethodTable[];
    static const char *coordinateMethodTable[];
    static const char *compositionModeTable[];
    static const char *imageFormatTable[];
    static const char *sizeModeTable[];
    static int translateEnum(const char *table[], const QString &pattern, int limit);

    // utility
    template <typename T> T image_load(const QString &filepath);

    // commands dictionary management
    static void staticInit();

public:
    struct PaintCommandInfos
    {
        PaintCommandInfos(QString id, void (PaintCommands::*p)(QRegularExpressionMatch), QRegularExpression r, QString sy, QString sa)
            : identifier(id)
            , regExp(r)
            , syntax(sy)
            , sample(sa)
            , paintMethod(p)
            {}
        PaintCommandInfos(QString title)
            : identifier(title), paintMethod(0) {}
        bool isSectionHeader() const { return paintMethod == 0; }
        QString identifier;
        QRegularExpression regExp;
        QString syntax;
        QString sample;
        void (PaintCommands::*paintMethod)(QRegularExpressionMatch);
    };

    static PaintCommandInfos *findCommandById(const QString &identifier) {
        for (int i=0; i<s_commandInfoTable.size(); i++)
            if (s_commandInfoTable[i].identifier == identifier)
                return &s_commandInfoTable[i];
        return 0;
    }

    static QList<PaintCommandInfos> s_commandInfoTable;
    static QList<QPair<QString,QStringList> > s_enumsTable;
    static QMultiHash<QString, int> s_commandHash;
};

#endif // PAINTCOMMANDS_H
