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
#include "interactivewidget.h"
#include "widgets.h"
#include "paintcommands.h"

#include <QtCore>
#include <QtWidgets>
#include <QtPrintSupport>
#include <qimage.h>
#include <QPicture>

#include <private/qmath_p.h>

#ifdef USE_CUSTOM_DEVICE
#include "customdevice.h"
#endif

#ifndef QT_NO_OPENGL
#include <qgl.h>
#include <QGLPixelBuffer>
#endif

// #define DO_QWS_DEBUGGING

#ifdef DO_QWS_DEBUGGING
extern bool qt_show_painter_debug_output = false;
#endif

//#define CONSOLE_APPLICATION

static const struct {
    const char *name;
    QImage::Format format;
} imageFormats[] = {
    { "mono", QImage::Format_Mono },
    { "monolsb", QImage::Format_MonoLSB },
    { "indexed8", QImage::Format_Indexed8 },
    { "rgb32", QImage::Format_RGB32 },
    { "argb32", QImage::Format_ARGB32 },
    { "argb32_premultiplied", QImage::Format_ARGB32_Premultiplied },
    { "rgb16", QImage::Format_RGB16 },
    { "argb8565_premultiplied", QImage::Format_ARGB8565_Premultiplied },
    { "rgb666", QImage::Format_RGB666 },
    { "argb6666_premultiplied", QImage::Format_ARGB6666_Premultiplied },
    { "rgb555", QImage::Format_RGB555 },
    { "argb8555_premultiplied", QImage::Format_ARGB8555_Premultiplied },
    { "rgb888", QImage::Format_RGB888 },
    { "rgb444", QImage::Format_RGB444 },
    { "argb4444_premultiplied", QImage::Format_ARGB4444_Premultiplied }
};

static void printHelp()
{
    printf("\nUsage:\n\n"
           "  paintcmd [options] files\n"
           "\n"
           "  Options:\n"
           "    -widget         Paints the files to a widget on screen\n"
           "    -pixmap         Paints the files to a pixmap\n"
           "    -bitmap         Paints the files to a bitmap\n"
           "    -image          Paints the files to an image\n"
           "    -imageformat    Set the format of the image when painting to an image\n"
           "    -imagemono      Paints the files to a monochrome image\n"
           "    -imagewidget    same as image, but with interacion...\n"
#ifndef QT_NO_OPENGL
           "    -opengl         Paints the files to a QGLWidget (Qt4 style) on screen\n"
           "    -glbuffer       Paints the files to a QOpenGLFrameBufferObject (Qt5 style) \n"
#endif
#ifdef USE_CUSTOM_DEVICE
           "    -customdevice   Paints the files to the custom paint device\n"
           "    -customwidget   Paints the files to a custom widget on screen\n"
#endif
           "    -pdf            Paints to a pdf\n"
           "    -ps             Paints to a ps\n"
           "    -picture        Prints into a picture, then shows the result in a label\n"
           "    -printer        Prints the commands to a file called output.ps|pdf\n"
           "    -highres        Prints in highres mode\n"
           "    -printdialog    Opens a print dialog, then prints to the selected printer\n"
           "    -grab           Paints the files to an image called filename_qps.png\n"
           "    -i              Interactive mode.\n"
           "    -v              Verbose.\n"
           "    -commands       Displays all available commands\n"
           "    -w              Width of the paintdevice\n"
           "    -h              Height of the paintdevice\n"
           "    -scalefactor    Scale factor (device pixel ratio) of the paintdevice\n"
           "    -cmp            Show the reference picture\n"
           "    -bg-white       No checkers background\n");
}

static void displayCommands()
{
    printf("Drawing operations:\n"
           "  drawArc x y width height angle sweep\n"
           "  drawChord x y width height angle sweep\n"
           "  drawEllipse x y width height\n"
           "  drawLine x1 y1 x2 y2\n"
           "  drawPath pathname\n"
           "  drawPie x y width height angle sweep\n"
           "  drawPixmap pixmapfile x y width height sx sy sw sh\n"
           "  drawPolygon [ x1 y1 x2 y2 ... ] winding|oddeven\n"
           "  drawPolyline [ x1 y1 x2 y2 ... ]\n"
           "  drawRect x y width height\n"
           "  drawRoundRect x y width height xfactor yfactor\n"
           "  drawText x y \"text\"\n"
           "  drawTiledPixmap pixmapfile x y width height sx sy\n"
           "\n"
           "Path commands:\n"
           "  path_addEllipse pathname x y width height\n"
           "  path_addPolygon pathname [ x1 y1 x2 y2 ... ] winding?\n"
           "  path_addRect pathname x y width height\n"
           "  path_addText pathname x y \"text\"                        Uses current font\n"
           "  path_arcTo pathname x y width hegiht\n"
           "  path_closeSubpath pathname\n"
           "  path_createOutline pathname newoutlinename                Uses current pen\n"
           "  path_cubicTo pathname c1x c1y c2x c2y endx endy\n"
           "  path_lineTo pathname x y\n"
           "  path_moveTo pathname x y\n"
           "  path_setFillRule pathname winding?\n"
           "\n"
           "Painter configuration:\n"
           "  resetMatrix\n"
           "  restore\n"
           "  save\n"
           "  rotate degrees\n"
           "  translate dx dy\n"
           "  scale sx sy\n"
           "  mapQuadToQuad x0 y0 x1 y1 x2 y2 x3 y3  x4 y4 x5 y5 x6 y6 x7 y7"
           "  setMatrix m11 m12 m13 m21 m22 m23 m31 m32 m33"
           "  setBackground color pattern?\n"
           "  setBackgroundMode TransparentMode|OpaqueMode\n"
           "  setBrush pixmapfile\n"
           "  setBrush nobrush\n"
           "  setBrush color pattern\n"
           "  setBrush x1 y1 color1 x2 y2 color2                        gradient brush\n"
           "  setBrushOrigin x y\n"
           "  setFont \"fontname\" pointsize bold? italic?\n"
           "  setPen style color\n"
           "  setPen color width style capstyle joinstyle\n"
           "  setRenderHint LineAntialiasing\n"
           "  gradient_clearStops\n"
           "  gradient_appendStop pos color"
           "  gradient_setSpread [PadSpread|ReflectSpread|RepeatSpread]\n"
           "  gradient_setLinear x1 y1 x2 y2\n"
           "  gradient_setRadial center_x center_y radius focal_x focal_y\n"
           "  gradient_setConical center_x center_y angle\n"
           "\n"
           "Clipping commands:\n"
           "  region_addRect regionname x y width height\n"
           "  region_getClipRegion regionname\n"
           "  setClipRect x y width height\n"
           "  setClipRegion regionname\n"
           "  setClipping true|false\n"
           "  setClipPath pathname\n"
           "\n"
           "Various commands:\n"
           "  surface_begin x y width height\n"
           "  surface_end\n"
           "  pixmap_load filename name_in_script\n"
           "  image_load filename name_in_script\n");
}
static InteractiveWidget *interactive_widget = 0;

static void runInteractive()
{
    interactive_widget = new InteractiveWidget;
    interactive_widget->show();
}

static QLabel* createLabel()
{
    QLabel *label = new QLabel;
    QPalette palette = label->palette();
    palette.setBrush(QPalette::Window, QBrush(Qt::white));
    label->setPalette(palette);
    return label;
}

int main(int argc, char **argv)
{
#ifdef CONSOLE_APPLICATION
    QApplication app(argc, argv, QApplication::Tty);
#else
    QApplication app(argc, argv);
#endif
#ifdef DO_QWS_DEBUGGING
    qt_show_painter_debug_output = false;
#endif

    DeviceType type = WidgetType;
    bool checkers_background = true;

    QImage::Format imageFormat = QImage::Format_ARGB32_Premultiplied;

    QLocale::setDefault(QLocale::c());

    QStringList files;

    bool interactive = false;
    bool printdlg = false;
    bool highres = false;
    bool show_cmp = false;
    int width = 800, height = 800;
    int scaledWidth = width, scaledHeight = height;
    qreal scalefactor = 1.0;
    bool verboseMode = false;

#ifndef QT_NO_OPENGL
    QGLFormat f = QGLFormat::defaultFormat();
    f.setSampleBuffers(true);
    f.setStencil(true);
    f.setAlpha(true);
    f.setAlphaBufferSize(8);
    QGLFormat::setDefaultFormat(f);
#endif

    char *arg;
    for (int i=1; i<argc; ++i) {
        arg = argv[i];
        if (*arg == '-') {
            QString option = QString(arg + 1).toLower();
            if (option == "widget")
                type = WidgetType;
            else if (option == "bitmap")
                type = BitmapType;
            else if (option == "pixmap")
                type = PixmapType;
            else if (option == "image")
                type = ImageType;
            else if (option == "imageformat") {
                Q_ASSERT_X(i + 1 < argc, "main", "-imageformat must be followed by a value");
                QString format = QString(argv[++i]).toLower();

                imageFormat = QImage::Format_Invalid;
                static const int formatCount =
                    sizeof(imageFormats) / sizeof(imageFormats[0]);
                for (int ff = 0; ff < formatCount; ++ff) {
                    if (QLatin1String(imageFormats[ff].name) == format) {
                        imageFormat = imageFormats[ff].format;
                        break;
                    }
                }

                if (imageFormat == QImage::Format_Invalid) {
                    printf("Invalid image format.  Available formats are:\n");
                    for (int ff = 0; ff < formatCount; ++ff)
                        printf("\t%s\n", imageFormats[ff].name);
                    return -1;
                }
            } else if (option == "imagemono")
                type = ImageMonoType;
            else if (option == "imagewidget")
                type = ImageWidgetType;
#ifndef QT_NO_OPENGL
            else if (option == "opengl")
                type = OpenGLType;
            else if (option == "glbuffer")
                type = OpenGLBufferType;
#endif
#ifdef USE_CUSTOM_DEVICE
            else if (option == "customdevice")
                type = CustomDeviceType;
            else if (option == "customwidget")
                type = CustomWidgetType;
#endif
            else if (option == "pdf")
                type = PdfType;
            else if (option == "ps")
                type = PsType;
            else if (option == "picture")
                type = PictureType;
            else if (option == "printer")
                type = PrinterType;
            else if (option == "highres") {
                type = PrinterType;
                highres = true;
            } else if (option == "printdialog") {
                type = PrinterType;
                printdlg = true;
            }
            else if (option == "grab")
                type = GrabType;
            else if (option == "i")
                interactive = true;
            else if (option == "v")
                verboseMode = true;
            else if (option == "commands") {
                displayCommands();
                return 0;
            } else if (option == "w") {
                Q_ASSERT_X(i + 1 < argc, "main", "-w must be followed by a value");
                width = atoi(argv[++i]);
            } else if (option == "h") {
                Q_ASSERT_X(i + 1 < argc, "main", "-h must be followed by a value");
                height = atoi(argv[++i]);
            } else if (option == "scalefactor") {
                Q_ASSERT_X(i + 1 < argc, "main", "-scalefactor must be followed by a value");
                scalefactor = atof(argv[++i]);
            } else if (option == "cmp") {
                show_cmp = true;
            } else if (option == "bg-white") {
                checkers_background = false;
            }
        } else {
#if 0 // Used to be included in Qt4 for Q_WS_WIN
            QString input = QString::fromLocal8Bit(argv[i]);
            if (input.indexOf('*') >= 0) {
                QFileInfo info(input);
                QDir dir = info.dir();
                QFileInfoList infos = dir.entryInfoList(QStringList(info.fileName()));
                for (int ii=0; ii<infos.size(); ++ii)
                    files.append(infos.at(ii).absoluteFilePath());
            } else {
                files.append(input);
            }
#else
            files.append(QString(argv[i]));
#endif
        }
    }
    scaledWidth = width * scalefactor;
    scaledHeight = height * scalefactor;

    PaintCommands pcmd(QStringList(), 800, 800);
    pcmd.setVerboseMode(verboseMode);
    pcmd.setType(type);
    pcmd.setCheckersBackground(checkers_background);

    QWidget *activeWidget = 0;

    if (interactive) {
        runInteractive();
        if (!files.isEmpty())
            interactive_widget->load(files.at(0));
    } else if (files.isEmpty()) {
        printHelp();
        return 0;
    } else {
        for (int j=0; j<files.size(); ++j) {
            const QString &fileName = files.at(j);
            QStringList content;

            QFile file(fileName);
            QFileInfo fileinfo(file);
            if (file.open(QIODevice::ReadOnly)) {
                QTextStream textFile(&file);
                QString script = textFile.readAll();
                content = script.split("\n", QString::SkipEmptyParts);
            } else {
                printf("failed to read file: '%s'\n", qPrintable(fileinfo.absoluteFilePath()));
                continue;
            }
            pcmd.setContents(content);

            if (show_cmp) {
                QString pmFile = QString(files.at(j)).replace(".qps", "_qps") + ".png";
                qDebug() << pmFile << QFileInfo(pmFile).exists();
                QPixmap pixmap(pmFile);
                if (!pixmap.isNull()) {
                    QLabel *label = createLabel();
                    label->setWindowTitle("VERIFY: " + pmFile);
                    label->setPixmap(pixmap);
                    label->show();
                }
            }

            switch (type) {

            case WidgetType:
            {
                OnScreenWidget<QWidget> *qWidget =
                    new OnScreenWidget<QWidget>(files.at(j));
                qWidget->setVerboseMode(verboseMode);
                qWidget->setType(type);
                qWidget->setCheckersBackground(checkers_background);
                qWidget->m_commands = content;
                qWidget->resize(width, height);
                qWidget->show();
                activeWidget = qWidget;
                break;
            }

            case ImageWidgetType:
            {
                OnScreenWidget<QWidget> *qWidget = new OnScreenWidget<QWidget>(files.at(j));
                qWidget->setVerboseMode(verboseMode);
                qWidget->setType(type);
                qWidget->setCheckersBackground(checkers_background);
                qWidget->m_commands = content;
                qWidget->resize(width, height);
                qWidget->show();
                activeWidget = qWidget;
                break;

            }
#ifndef QT_NO_OPENGL
            case OpenGLBufferType:
            {
                QWindow win;
                win.setSurfaceType(QSurface::OpenGLSurface);
                win.create();
                QOpenGLFramebufferObjectFormat fmt;
                fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
                fmt.setSamples(4);
                QOpenGLContext ctx;
                ctx.create();
                ctx.makeCurrent(&win);
                QOpenGLFramebufferObject fbo(width, height, fmt);
                fbo.bind();
                QOpenGLPaintDevice pdev(width, height);

                QPainter pt(&pdev);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();

                QImage image = fbo.toImage();

                QLabel *label = createLabel();
                label->setPixmap(QPixmap::fromImage(image));
                label->resize(label->sizeHint());
                label->show();
                activeWidget = label;
                break;
            }
            case OpenGLType:
            {
                OnScreenWidget<QGLWidget> *qGLWidget = new OnScreenWidget<QGLWidget>(files.at(j));
                qGLWidget->setVerboseMode(verboseMode);
                qGLWidget->setType(type);
                qGLWidget->setCheckersBackground(checkers_background);
                qGLWidget->m_commands = content;
                qGLWidget->resize(width, height);
                qGLWidget->show();
                activeWidget = qGLWidget;
                break;
            }
#else
            case OpenGLType:
            case OpenGLBufferType:
            {
                printf("OpenGL type not supported in this Qt build\n");
                break;
            }
#endif
#ifdef USE_CUSTOM_DEVICE
            case CustomDeviceType:
            {
                CustomPaintDevice custom(width, height);
                QPainter pt;
                pt.begin(&custom);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();
                QImage *img = custom.image();
                if (img) {
                    QLabel *label = createLabel();
                    label->setPixmap(QPixmap::fromImage(*img));
                    label->resize(label->sizeHint());
                    label->show();
                    activeWidget = label;
                    img->save("custom_output_pixmap.png", "PNG");
                } else {
                    custom.save("custom_output_pixmap.png", "PNG");
                }
                break;
            }
            case CustomWidgetType:
            {
                OnScreenWidget<CustomWidget> *cWidget = new OnScreenWidget<CustomWidget>;
                cWidget->setVerboseMode(verboseMode);
                cWidget->setType(type);
                cWidget->setCheckersBackground(checkers_background);
                cWidget->m_filename = files.at(j);
                cWidget->setWindowTitle(fileinfo.filePath());
                cWidget->m_commands = content;
                cWidget->resize(width, height);
                cWidget->show();
                activeWidget = cWidget;
                break;
            }
#endif
            case PixmapType:
            {
                QPixmap pixmap(scaledWidth, scaledHeight);
                pixmap.setDevicePixelRatio(scalefactor);
                pixmap.fill(Qt::white);
                QPainter pt(&pixmap);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();
                pixmap.save("output_pixmap.png", "PNG");
                break;
            }

            case BitmapType:
            {
                QBitmap bitmap(scaledWidth, scaledHeight);
                bitmap.setDevicePixelRatio(scalefactor);
                QPainter pt(&bitmap);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();
                bitmap.save("output_bitmap.png", "PNG");

                QLabel *label = createLabel();
                label->setPixmap(bitmap);
                label->resize(label->sizeHint());
                label->show();
                activeWidget = label;
                break;
            }

            case ImageMonoType:
            case ImageType:
            {
                qDebug() << "Creating image";
                QImage image(scaledWidth, scaledHeight, type == ImageMonoType
                             ? QImage::Format_MonoLSB
                             : imageFormat);
                image.setDevicePixelRatio(scalefactor);
                image.fill(0);
                QPainter pt(&image);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();
                image.convertToFormat(QImage::Format_ARGB32).save("output_image.png", "PNG");
                image.setDevicePixelRatio(1.0); // reset scale factor: display "large" image.
#ifndef CONSOLE_APPLICATION
                QLabel *label = createLabel();
                label->setPixmap(QPixmap::fromImage(image));
                label->resize(label->sizeHint());
                label->show();
                activeWidget = label;
#endif
                break;
            }

            case PictureType:
            {
                QPicture pic;
                QPainter pt(&pic);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();

                QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
                image.fill(0);
                pt.begin(&image);
                pt.drawPicture(0, 0, pic);
                pt.end();
                QLabel *label = createLabel();
                label->setWindowTitle(fileinfo.absolutePath());
                label->setPixmap(QPixmap::fromImage(image));
                label->resize(label->sizeHint());
                label->show();
                activeWidget = label;
                break;
            }

            case PrinterType:
            {
#ifndef QT_NO_PRINTER
                PaintCommands pcmd(QStringList(), 800, 800);
                pcmd.setVerboseMode(verboseMode);
                pcmd.setType(type);
                pcmd.setCheckersBackground(checkers_background);
                pcmd.setContents(content);
                QString file = QString(files.at(j)).replace(QLatin1Char('.'), QLatin1Char('_')) + ".ps";

                QPrinter p(highres ? QPrinter::HighResolution : QPrinter::ScreenResolution);
                if (printdlg) {
                    QPrintDialog printDialog(&p, 0);
                    if (printDialog.exec() != QDialog::Accepted)
                        break;
                } else {
                    p.setOutputFileName(file);
                }

                QPainter pt(&p);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();

                if (!printdlg) {
                    printf("wrote file: %s\n", qPrintable(file));
                }

                Q_ASSERT(!p.paintingActive());
#endif
                break;
            }
            case PsType:
            case PdfType:
            {
#ifndef QT_NO_PRINTER
                PaintCommands pcmd(QStringList(), 800, 800);
                pcmd.setVerboseMode(verboseMode);
                pcmd.setType(type);
                pcmd.setCheckersBackground(checkers_background);
                pcmd.setContents(content);
                QPrinter p(highres ? QPrinter::HighResolution : QPrinter::ScreenResolution);
                QFileInfo input(files.at(j));
                const QString file = input.baseName() + QLatin1Char('_')
                                     + input.suffix() + QStringLiteral(".pdf");
                p.setOutputFormat(QPrinter::PdfFormat);
                p.setOutputFileName(file);
                p.setPageSize(QPrinter::A4);
                QPainter pt(&p);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();

                printf("write file: %s\n", qPrintable(file));
#endif
                break;
            }
            case GrabType:
            {
                QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
                image.fill(QColor(Qt::white).rgb());
                QPainter pt(&image);
                pcmd.setPainter(&pt);
                pcmd.setFilePath(fileinfo.absolutePath());
                pcmd.runCommands();
                pt.end();
                QImage image1(width, height, QImage::Format_RGB32);
                image1.fill(QColor(Qt::white).rgb());
                QPainter pt1(&image1);
                pt1.drawImage(QPointF(0, 0), image);
                pt1.end();

                QString filename = QString(files.at(j)).replace(".qps", "_qps") + ".png";
                image1.save(filename, "PNG");
                printf("%s grabbed to %s\n", qPrintable(files.at(j)), qPrintable(filename));
                break;
            }
            default:
                break;
            }
        }
    }
#ifndef CONSOLE_APPLICATION
    if (activeWidget || interactive) {
        QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
        app.exec();
    }
    delete activeWidget;
#endif
    delete interactive_widget;
    return 0;
}

