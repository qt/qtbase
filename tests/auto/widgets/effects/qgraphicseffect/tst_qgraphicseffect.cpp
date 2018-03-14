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


#include <QtTest/QtTestWidgets>
#include <QtWidgets/qdesktopwidget.h>
#include <QtWidgets/qgraphicseffect.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qgraphicsscene.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtWidgets/qgraphicswidget.h>
#include <QtWidgets/qstyleoption.h>

#include <private/qgraphicseffect_p.h>

class tst_QGraphicsEffect : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();

private slots:
    void setEnabled();
    void source();
    void boundingRectFor();
    void boundingRect();
    void boundingRect2();
    void draw();
    void opacity();
    void grayscale();
    void colorize();
    void drawPixmapItem();
    void deviceCoordinateTranslateCaching();
    void inheritOpacity();
    void dropShadowClipping();
    void childrenVisibilityShouldInvalidateCache();
    void prepareGeometryChangeInvalidateCache();
    void itemHasNoContents();
};

void tst_QGraphicsEffect::initTestCase()
{}

class CustomItem : public QGraphicsRectItem
{
public:
    CustomItem(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent = 0)
        : QGraphicsRectItem(x, y, width, height, parent), numRepaints(0),
          m_painter(0), m_styleOption(0)
    {}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        m_painter = painter;
        m_styleOption = option;
        ++numRepaints;
        QGraphicsRectItem::paint(painter, option, widget);
    }

    void reset()
    {
        numRepaints = 0;
        m_painter = 0;
        m_styleOption = 0;
    }

    int numRepaints;
    QPainter *m_painter;
    const QStyleOption *m_styleOption;
};

class CustomEffect : public QGraphicsEffect
{
public:
    CustomEffect()
        : QGraphicsEffect(), numRepaints(0), m_margin(10),
          doNothingInDraw(false), m_painter(0), m_styleOption(0), m_source(0), m_opacity(1.0)
    {}

    QRectF boundingRectFor(const QRectF &rect) const
    { return rect.adjusted(-m_margin, -m_margin, m_margin, m_margin); }

    void reset()
    {
        numRepaints = 0;
        m_sourceChangedFlags = QGraphicsEffect::ChangeFlags();
        m_painter = 0;
        m_styleOption = 0;
        m_source = 0;
        m_opacity = 1.0;
    }

    void setMargin(int margin)
    {
        m_margin = margin;
        updateBoundingRect();
    }

    int margin() const
    { return m_margin; }

    void draw(QPainter *painter)
    {
        ++numRepaints;
        if (doNothingInDraw)
            return;
        m_source = source();
        m_painter = painter;
        m_styleOption = source()->styleOption();
        m_opacity = painter->opacity();
        drawSource(painter);
    }

    void sourceChanged(QGraphicsEffect::ChangeFlags flags)
    { m_sourceChangedFlags |= flags; }

    int numRepaints;
    int m_margin;
    QGraphicsEffect::ChangeFlags m_sourceChangedFlags;
    bool doNothingInDraw;
    QPainter *m_painter;
    const QStyleOption *m_styleOption;
    QGraphicsEffectSource *m_source;
    qreal m_opacity;
};

void tst_QGraphicsEffect::setEnabled()
{
    CustomEffect effect;
    QVERIFY(effect.isEnabled());

    effect.setEnabled(false);
    QVERIFY(!effect.isEnabled());
}

void tst_QGraphicsEffect::source()
{
    QPointer<CustomEffect> effect = new CustomEffect;
    QVERIFY(!effect->source());
    QVERIFY(!effect->m_sourceChangedFlags);

    // Install effect on QGraphicsItem.
    QGraphicsItem *item = new QGraphicsRectItem(0, 0, 10, 10);
    item->setGraphicsEffect(effect);
    QVERIFY(effect->source());
    QCOMPARE(effect->source()->graphicsItem(), (const QGraphicsItem*)item);
    QVERIFY(effect->m_sourceChangedFlags & QGraphicsEffect::SourceAttached);
    effect->reset();

    // Make sure disabling/enabling the effect doesn't change the source.
    effect->setEnabled(false);
    QVERIFY(effect->source());
    QCOMPARE(effect->source()->graphicsItem(), (const QGraphicsItem*)item);
    QVERIFY(!effect->m_sourceChangedFlags);
    effect->reset();

    effect->setEnabled(true);
    QVERIFY(effect->source());
    QCOMPARE(effect->source()->graphicsItem(), (const QGraphicsItem*)item);
    QVERIFY(!effect->m_sourceChangedFlags);
    effect->reset();

    // Uninstall effect on QGraphicsItem.
    effect->reset();
    item->setGraphicsEffect(0);
    QVERIFY(!effect);
    effect = new CustomEffect;

    // The item takes ownership and should delete the effect when destroyed.
    item->setGraphicsEffect(effect);
    QPointer<QGraphicsEffectSource> source = effect->source();
    QVERIFY(source);
    QCOMPARE(source->graphicsItem(), (const QGraphicsItem*)item);
    delete item;
    QVERIFY(!effect);
    QVERIFY(!source);
}

void tst_QGraphicsEffect::boundingRectFor()
{
    CustomEffect effect;
    int margin = effect.margin();
    const QRectF source(0, 0, 100, 100);
    QCOMPARE(effect.boundingRectFor(source), source.adjusted(-margin, -margin, margin, margin));

    effect.setMargin(margin = 20);
    QCOMPARE(effect.boundingRectFor(source), source.adjusted(-margin, -margin, margin, margin));
}

void tst_QGraphicsEffect::boundingRect()
{
    // No source; empty bounding rect.
    CustomEffect *effect = new CustomEffect;
    QCOMPARE(effect->boundingRect(), QRectF());

    // Install effect on QGraphicsItem.
    QRectF itemRect(0, 0, 100, 100);
    QGraphicsRectItem *item = new QGraphicsRectItem;
    item->setPen(QPen(Qt::black, 0));
    item->setRect(itemRect);
    item->setGraphicsEffect(effect);
    int margin = effect->margin();
    QCOMPARE(effect->boundingRect(), itemRect.adjusted(-margin, -margin, margin, margin));
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(itemRect));

    // Make sure disabling/enabling the effect doesn't change the bounding rect.
    effect->setEnabled(false);
    QCOMPARE(effect->boundingRect(), itemRect.adjusted(-margin, -margin, margin, margin));
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(itemRect));
    effect->setEnabled(true);
    QCOMPARE(effect->boundingRect(), itemRect.adjusted(-margin, -margin, margin, margin));
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(itemRect));

    // Change effect margins.
    effect->setMargin(margin = 20);
    QCOMPARE(effect->boundingRect(), itemRect.adjusted(-margin, -margin, margin, margin));
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(itemRect));

    // Uninstall effect on QGraphicsItem.
    QPointer<CustomEffect> ptr = effect;
    item->setGraphicsEffect(0);
    QVERIFY(!ptr);

    delete item;
}

void tst_QGraphicsEffect::boundingRect2()
{
    CustomEffect *effect = new CustomEffect;
    QGraphicsRectItem *root = new QGraphicsRectItem;
    root->setPen(QPen(Qt::black, 0));
    root->setGraphicsEffect(effect);

    QGraphicsRectItem *child = new QGraphicsRectItem;
    QRectF childRect(0, 0, 100, 100);
    child->setPen(QPen(Qt::black, 0));
    child->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    child->setRect(childRect);
    child->setParentItem(root);

    QGraphicsRectItem *grandChild = new QGraphicsRectItem;
    QRectF grandChildRect(0, 0, 200, 200);
    grandChild->setPen(QPen(Qt::black, 0));
    grandChild->setRect(grandChildRect);
    grandChild->setParentItem(child);

    // Make sure the effect's bounding rect is clipped to the child's bounding rect.
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(childRect));

    // Disable ItemClipsChildrenToShape; effect's bounding rect is no longer clipped.
    child->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(childRect | grandChildRect));

    // Add root item to a scene, do the same tests as above. Results should be the same.
    QGraphicsScene scene;
    scene.addItem(root);

    child->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(childRect));

    child->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(childRect | grandChildRect));

    // Now add the scene to a view, results should be the same.
    QGraphicsView view(&scene);

    child->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(childRect));

    child->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(childRect | grandChildRect));

    CustomEffect *childEffect = new CustomEffect;
    child->setGraphicsEffect(childEffect);
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(childEffect->boundingRectFor(childRect | grandChildRect)));

    child->setGraphicsEffect(0);
    QCOMPARE(effect->boundingRect(), effect->boundingRectFor(childRect | grandChildRect));
}

void tst_QGraphicsEffect::draw()
{
    QGraphicsScene scene;
    CustomItem *item = new CustomItem(0, 0, 100, 100);
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(item->numRepaints > 0);
    item->reset();

    // Make sure installing the effect triggers a repaint.
    CustomEffect *effect = new CustomEffect;
    item->setGraphicsEffect(effect);
    QTRY_COMPARE(effect->numRepaints, 1);
    QTRY_COMPARE(item->numRepaints, 1);

    // Make sure QPainter* and QStyleOptionGraphicsItem* stays persistent
    // during QGraphicsEffect::draw/QGraphicsItem::paint.
    QVERIFY(effect->m_painter);
    QCOMPARE(effect->m_painter, item->m_painter);
    QCOMPARE(effect->m_styleOption, item->m_styleOption);
    // Make sure QGraphicsEffect::source is persistent.
    QCOMPARE(effect->m_source, effect->source());
    effect->reset();
    item->reset();

    // Make sure updating the source triggers a repaint.
    item->update();
    QTRY_COMPARE(effect->numRepaints, 1);
    QTRY_COMPARE(item->numRepaints, 1);
    QVERIFY(effect->m_sourceChangedFlags & QGraphicsEffect::SourceInvalidated);
    effect->reset();
    item->reset();

    // Make sure changing the effect's bounding rect triggers a repaint.
    effect->setMargin(20);
    QTRY_COMPARE(effect->numRepaints, 1);
    QTRY_COMPARE(item->numRepaints, 1);
    effect->reset();
    item->reset();

    // Make sure change the item's bounding rect triggers a repaint.
    item->setRect(0, 0, 50, 50);
    QTRY_COMPARE(effect->numRepaints, 1);
    QTRY_COMPARE(item->numRepaints, 1);
    QVERIFY(effect->m_sourceChangedFlags & QGraphicsEffect::SourceBoundingRectChanged);
    effect->reset();
    item->reset();

    // Make sure the effect is the one to issue a repaint of the item.
    effect->doNothingInDraw = true;
    item->update();
    QTRY_COMPARE(effect->numRepaints, 1);
    QCOMPARE(item->numRepaints, 0);
    effect->doNothingInDraw = false;
    effect->reset();
    item->reset();

    // Make sure we update the source when disabling/enabling the effect.
    effect->setEnabled(false);
    QTest::qWait(50);
    QCOMPARE(effect->numRepaints, 0);
    QCOMPARE(item->numRepaints, 1);
    effect->reset();
    item->reset();

    effect->setEnabled(true);
    QTRY_COMPARE(effect->numRepaints, 1);
    QTRY_COMPARE(item->numRepaints, 1);
    effect->reset();
    item->reset();

    // Effect is already enabled; nothing should happen.
    effect->setEnabled(true);
    QTest::qWait(50);
    QCOMPARE(effect->numRepaints, 0);
    QCOMPARE(item->numRepaints, 0);

    // Make sure uninstalling an effect triggers a repaint.
    QPointer<CustomEffect> ptr = effect;
    item->setGraphicsEffect(0);
    QVERIFY(!ptr);
    QTRY_COMPARE(item->numRepaints, 1);
}

void tst_QGraphicsEffect::opacity()
{
    // Make sure the painter's opacity is correct in QGraphicsEffect::draw.
    QGraphicsScene scene;
    CustomItem *item = new CustomItem(0, 0, 100, 100);
    item->setOpacity(0.5);
    CustomEffect *effect = new CustomEffect;
    item->setGraphicsEffect(effect);
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(effect->numRepaints > 0);
    QCOMPARE(effect->m_opacity, qreal(0.5));
}

void tst_QGraphicsEffect::grayscale()
{
    if (qApp->desktop()->depth() < 24)
        QSKIP("Test only works on 32 bit displays");

    QGraphicsScene scene(0, 0, 100, 100);

    QGraphicsRectItem *item = scene.addRect(0, 0, 50, 50);
    item->setPen(Qt::NoPen);
    item->setBrush(QColor(122, 193, 66)); // Qt light green

    QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
    effect->setColor(Qt::black);
    item->setGraphicsEffect(effect);

    QPainter painter;
    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);

    image.fill(0);
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(10, 10), qRgb(148, 148, 148));

    effect->setStrength(0.5);

    image.fill(0);
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(10, 10), qRgb(135, 171, 107));

    effect->setStrength(0.0);

    image.fill(0);
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(10, 10), qRgb(122, 193, 66));
}

void tst_QGraphicsEffect::colorize()
{
    if (qApp->desktop()->depth() < 24)
        QSKIP("Test only works on 32 bit displays");

    QGraphicsScene scene(0, 0, 100, 100);

    QGraphicsRectItem *item = scene.addRect(0, 0, 50, 50);
    item->setPen(Qt::NoPen);
    item->setBrush(QColor(122, 193, 66)); // Qt light green

    QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
    effect->setColor(QColor(102, 153, 51)); // Qt dark green
    item->setGraphicsEffect(effect);

    QPainter painter;
    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);

    image.fill(0);
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(10, 10), qRgb(191, 212, 169));

    effect->setStrength(0.5);

    image.fill(0);
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(10, 10), qRgb(156, 203, 117));

    effect->setStrength(0.0);

    image.fill(0);
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(10, 10), qRgb(122, 193, 66));
}

class PixmapItemEffect : public QGraphicsEffect
{
public:
    PixmapItemEffect(const QPixmap &source)
        : QGraphicsEffect()
        , pixmap(source)
        , repaints(0)
    {}

    QRectF boundingRectFor(const QRectF &rect) const
    { return rect; }

    void draw(QPainter *painter)
    {
        QCOMPARE(sourcePixmap(Qt::LogicalCoordinates).handle(), pixmap.handle());
        QVERIFY((painter->worldTransform().type() <= QTransform::TxTranslate) == (sourcePixmap(Qt::DeviceCoordinates).handle() == pixmap.handle()));

        ++repaints;
    }
    QPixmap pixmap;
    int repaints;
};

void tst_QGraphicsEffect::drawPixmapItem()
{
    QImage image(32, 32, QImage::Format_RGB32);
    QPainter p(&image);
    p.fillRect(0, 0, 32, 16, Qt::blue);
    p.fillRect(0, 16, 32, 16, Qt::red);
    p.end();

    QGraphicsScene scene;
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    scene.addItem(item);

    PixmapItemEffect *effect = new PixmapItemEffect(item->pixmap());
    item->setGraphicsEffect(effect);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(effect->repaints >= 1);

    item->setTransform(QTransform().rotate(180), true);

    QTRY_VERIFY(effect->repaints >= 2);
}

class DeviceEffect : public QGraphicsEffect
{
public:
    QRectF boundingRectFor(const QRectF &rect) const
    { return rect; }

    void draw(QPainter *painter)
    {
        QPoint offset;
        QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset, QGraphicsEffect::NoPad);

        if (pixmap.isNull())
            return;

        painter->save();
        painter->setWorldTransform(QTransform());
        painter->drawPixmap(offset, pixmap);
        painter->restore();
    }
};

void tst_QGraphicsEffect::deviceCoordinateTranslateCaching()
{
    QGraphicsScene scene;
    CustomItem *item = new CustomItem(0, 0, 10, 10);
    scene.addItem(item);
    scene.setSceneRect(0, 0, 50, 0);

    item->setGraphicsEffect(new DeviceEffect);
    item->setPen(Qt::NoPen);
    item->setBrush(Qt::red);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTRY_VERIFY(item->numRepaints >= 1);
    int numRepaints = item->numRepaints;

    item->setTransform(QTransform::fromTranslate(10, 0), true);

    QTRY_COMPARE(item->numRepaints, numRepaints);
}

void tst_QGraphicsEffect::inheritOpacity()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rectItem = new QGraphicsRectItem(0, 0, 10, 10);
    CustomItem *item = new CustomItem(0, 0, 10, 10, rectItem);

    scene.addItem(rectItem);

    item->setGraphicsEffect(new DeviceEffect);
    item->setPen(Qt::NoPen);
    item->setBrush(Qt::red);

    rectItem->setOpacity(0.5);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTRY_VERIFY(item->numRepaints >= 1);

    int numRepaints = item->numRepaints;

    rectItem->setOpacity(1);

    // item should have been rerendered due to opacity changing
    QTRY_VERIFY(item->numRepaints > numRepaints);
}

void tst_QGraphicsEffect::dropShadowClipping()
{
    QImage img(128, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(0xffffffff);

    QGraphicsScene scene;
    QGraphicsRectItem *item = new QGraphicsRectItem(-5, -500, 10, 1000);
    item->setGraphicsEffect(new QGraphicsDropShadowEffect);
    item->setPen(Qt::NoPen);
    item->setBrush(Qt::red);

    scene.addItem(item);

    QPainter p(&img);
    scene.render(&p, img.rect(), QRect(-64, -64, 128, 128));
    p.end();

    for (int y = 1; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            QCOMPARE(img.pixel(x, y), img.pixel(x, y-1));
}

class MyGraphicsItem : public QGraphicsWidget
{
public:
    MyGraphicsItem(QGraphicsItem *parent = 0) :
            QGraphicsWidget(parent), nbPaint(0)
    {}
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        nbPaint++;
        QGraphicsWidget::paint(painter, option, widget);
    }
    int nbPaint;
};

void tst_QGraphicsEffect::childrenVisibilityShouldInvalidateCache()
{
    QGraphicsScene scene;
    MyGraphicsItem parent;
    parent.resize(200, 200);
    QGraphicsWidget child(&parent);
    child.resize(200, 200);
    child.setVisible(false);
    scene.addItem(&parent);
    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(parent.nbPaint >= 1);
    //we set an effect on the parent
    parent.setGraphicsEffect(new QGraphicsDropShadowEffect(&parent));
    //flush the events
    QApplication::processEvents();
    //new effect applied->repaint
    QVERIFY(parent.nbPaint >= 2);
    child.setVisible(true);
    //flush the events
    QApplication::processEvents();
    //a new child appears we need to redraw the effect.
    QVERIFY(parent.nbPaint >= 3);
}

void tst_QGraphicsEffect::prepareGeometryChangeInvalidateCache()
{
    MyGraphicsItem *item = new MyGraphicsItem;
    item->resize(200, 200);

    QGraphicsScene scene;
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(item->nbPaint >= 1);

    item->nbPaint = 0;
    item->setGraphicsEffect(new QGraphicsDropShadowEffect);
    QTRY_COMPARE(item->nbPaint, 1);

    item->nbPaint = 0;
    item->resize(300, 300);
    QTRY_COMPARE(item->nbPaint, 1);

    item->nbPaint = 0;
    item->setPos(item->pos() + QPointF(10, 10));
    QTest::qWait(50);
    QCOMPARE(item->nbPaint, 0);
}

void tst_QGraphicsEffect::itemHasNoContents()
{
    QGraphicsRectItem *parent = new QGraphicsRectItem;
    parent->setFlag(QGraphicsItem::ItemHasNoContents);

    MyGraphicsItem *child = new MyGraphicsItem;
    child->setParentItem(parent);
    child->resize(200, 200);

    QGraphicsScene scene;
    scene.addItem(parent);

    QGraphicsView view(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(child->nbPaint >= 1);

    CustomEffect *effect = new CustomEffect;
    parent->setGraphicsEffect(effect);
    QTRY_COMPARE(effect->numRepaints, 1);

    for (int i = 0; i < 3; ++i) {
        effect->reset();
        effect->update();
        QTRY_COMPARE(effect->numRepaints, 1);
    }
}

QTEST_MAIN(tst_QGraphicsEffect)
#include "tst_qgraphicseffect.moc"

