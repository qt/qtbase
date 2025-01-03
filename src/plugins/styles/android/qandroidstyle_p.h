// Copyright (C) 2013 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDSTYLE_P_H
#define QANDROIDSTYLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qstylefactory.cpp.  This header may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/private/qfusionstyle_p.h>
#include <QtCore/QList>
#include <QtCore/QMargins>
#include <QtCore/QHash>
#include <QtCore/QVariantMap>

QT_BEGIN_NAMESPACE

class Q_WIDGETS_EXPORT QAndroidStyle : public QFusionStyle
{
    Q_OBJECT

public:
    enum ItemType
    {
        QC_UnknownType = -1,
        QC_View,
        QC_GroupBox,
        QC_Button,
        QC_Checkbox,
        QC_RadioButton,
        QC_Slider,
        QC_Switch,
        QC_EditText,
        QC_Combobox,
        QC_BusyIndicator,
        QC_ProgressBar,
        QC_Tab,
        QC_TabButton,
        QC_RatingIndicator,
        QC_SearchBox,
        QC_CustomControl=0xf00,
        QC_ControlMask=0xfff
    };

    struct Android9PatchChunk
    {
        QList<int> xDivs;
        QList<int> yDivs;
        QList<int> colors;
    };

    struct AndroidItemStateInfo
    {
        AndroidItemStateInfo():state(0){}
        int state;
        QByteArray filePath;
        QByteArray hashKey;
        Android9PatchChunk chunkData;
        QSize size;
        QMargins padding;
    };

    enum AndroidDrawableType
    {
        Color,
        Image,
        Clip,
        NinePatch,
        Gradient,
        State,
        Layer
    };

    class AndroidDrawable
    {
    public:
        AndroidDrawable(const QVariantMap &drawable, ItemType itemType);
        virtual ~AndroidDrawable();
        virtual void initPadding(const QVariantMap &drawable);
        virtual AndroidDrawableType type() const = 0;
        virtual void draw(QPainter *painter,const QStyleOption *opt) const = 0;
        const QMargins &padding() const;
        virtual QSize size() const;
        static AndroidDrawable *fromMap(const QVariantMap &drawable, ItemType itemType);
        static QMargins extractMargins(const QVariantMap &value);
        virtual void setPaddingLeftToSizeWidth();
    protected:
        ItemType m_itemType;
        QMargins m_padding;
    };

    class AndroidColorDrawable: public AndroidDrawable
    {
    public:
        AndroidColorDrawable(const QVariantMap &drawable, ItemType itemType);
        virtual AndroidDrawableType type() const;
        virtual void draw(QPainter *painter,const QStyleOption *opt) const;

    protected:
        QColor m_color;
    };

    class AndroidImageDrawable: public AndroidDrawable
    {
    public:
        AndroidImageDrawable(const QVariantMap &drawable, ItemType itemType);
        virtual AndroidDrawableType type() const;
        virtual void draw(QPainter *painter,const QStyleOption *opt) const;
        virtual QSize size() const;

    protected:
        QString m_filePath;
        mutable QString m_hashKey;
        QSize m_size;
    };

    class Android9PatchDrawable: public AndroidImageDrawable
    {
    public:
        Android9PatchDrawable(const QVariantMap &drawable, ItemType itemType);
        virtual AndroidDrawableType type() const;
        virtual void draw(QPainter *painter, const QStyleOption *opt) const;
    private:
        static int calculateStretch(int boundsLimit, int startingPoint,
                                  int srcSpace, int numStrechyPixelsRemaining,
                                  int numFixedPixelsRemaining);
        void extractIntArray(const QVariantList &values, QList<int> &array);
    private:
        Android9PatchChunk m_chunkData;
    };

    class AndroidGradientDrawable: public AndroidDrawable
    {
    public:
        enum GradientOrientation
        {
            TOP_BOTTOM,
            TR_BL,
            RIGHT_LEFT,
            BR_TL,
            BOTTOM_TOP,
            BL_TR,
            LEFT_RIGHT,
            TL_BR
        };

    public:
        AndroidGradientDrawable(const QVariantMap &drawable, ItemType itemType);
        virtual AndroidDrawableType type() const;
        virtual void draw(QPainter *painter, const QStyleOption *opt) const;
        QSize size() const;
    private:
        mutable QLinearGradient m_gradient;
        GradientOrientation m_orientation;
        int m_radius;
    };

    class AndroidClipDrawable: public AndroidDrawable
    {
    public:
        AndroidClipDrawable(const QVariantMap &drawable, ItemType itemType);
        ~AndroidClipDrawable();
        virtual AndroidDrawableType type() const;
        virtual void setFactor(double factor, Qt::Orientation orientation);
        virtual void draw(QPainter *painter, const QStyleOption *opt) const;

    private:
        double m_factor;
        Qt::Orientation m_orientation;
        const AndroidDrawable *m_drawable;
    };

    class AndroidStateDrawable: public AndroidDrawable
    {
    public:
        AndroidStateDrawable(const QVariantMap &drawable, ItemType itemType);
        ~AndroidStateDrawable();
        virtual AndroidDrawableType type() const;
        virtual void draw(QPainter *painter, const QStyleOption *opt) const;
        inline const AndroidDrawable *bestAndroidStateMatch(const QStyleOption *opt) const;
        static int extractState(const QVariantMap &value);
        virtual void setPaddingLeftToSizeWidth();
        QSize sizeImage(const QStyleOption *opt) const;
    private:
        typedef QPair<int, const AndroidDrawable *> StateType;
        QList<StateType> m_states;
    };

    class AndroidLayerDrawable: public AndroidDrawable
    {
    public:
        AndroidLayerDrawable(const QVariantMap &drawable, QAndroidStyle::ItemType itemType);
        ~AndroidLayerDrawable();
        virtual AndroidDrawableType type() const;
        virtual void setFactor(int id, double factor, Qt::Orientation orientation);
        virtual void draw(QPainter *painter, const QStyleOption *opt) const;
        AndroidDrawable *layer(int id) const;
        QSize size() const;
    private:
        typedef QPair<int, AndroidDrawable *> LayerType;
        QList<LayerType> m_layers;
        int m_id;
        double m_factor;
        Qt::Orientation m_orientation;
    };

    class AndroidControl
    {
    public:
        AndroidControl(const QVariantMap &control, ItemType itemType);
        virtual ~AndroidControl();
        virtual void drawControl(const QStyleOption *opt, QPainter *p, const QWidget *w);
        virtual QRect subElementRect(SubElement subElement,
                                     const QStyleOption *option,
                                     const QWidget *widget = nullptr) const;
        virtual QRect subControlRect(const QStyleOptionComplex *option,
                                     SubControl sc,
                                     const QWidget *widget = nullptr) const;
        virtual QSize sizeFromContents(const QStyleOption *opt,
                                       const QSize &contentsSize,
                                       const QWidget *w) const;
        virtual QMargins padding();
        virtual QSize size(const QStyleOption *option);
    protected:
        virtual const AndroidDrawable * backgroundDrawable() const;
        const AndroidDrawable *m_background;
        QSize m_minSize;
        QSize m_maxSize;
    };

    class AndroidCompoundButtonControl : public AndroidControl
    {
    public:
        AndroidCompoundButtonControl(const QVariantMap &control, ItemType itemType);
        virtual ~AndroidCompoundButtonControl();
        virtual void drawControl(const QStyleOption *opt, QPainter *p, const QWidget *w);
        virtual QMargins padding();
        virtual QSize size(const QStyleOption *option);
    protected:
        virtual const AndroidDrawable * backgroundDrawable() const;
        const AndroidDrawable *m_button;
    };

    class AndroidProgressBarControl : public AndroidControl
    {
    public:
        AndroidProgressBarControl(const QVariantMap &control, ItemType itemType);
        virtual ~AndroidProgressBarControl();
        virtual void drawControl(const QStyleOption *option, QPainter *p, const QWidget *w);
        virtual QRect subElementRect(SubElement subElement,
                                     const QStyleOption *option,
                                     const QWidget *widget = nullptr) const;

        QSize sizeFromContents(const QStyleOption *opt,
                               const QSize &contentsSize,
                               const QWidget *w) const;
    protected:
        AndroidDrawable *m_progressDrawable;
        AndroidDrawable *m_indeterminateDrawable;
        int m_secondaryProgress_id;
        int m_progressId;
    };

    class AndroidSeekBarControl : public AndroidProgressBarControl
    {
    public:
        AndroidSeekBarControl(const QVariantMap &control, ItemType itemType);
        virtual ~AndroidSeekBarControl();
        virtual void drawControl(const QStyleOption *option, QPainter *p, const QWidget *w);
        QSize sizeFromContents(const QStyleOption *opt,
                                       const QSize &contentsSize, const QWidget *w) const;
        QRect subControlRect(const QStyleOptionComplex *option, SubControl sc,
                             const QWidget *widget = nullptr) const;
    private:
        AndroidDrawable *m_seekBarThumb;
    };

    class AndroidSpinnerControl : public AndroidControl
    {
    public:
        AndroidSpinnerControl(const QVariantMap &control, ItemType itemType);
        virtual ~AndroidSpinnerControl(){}
        virtual QRect subControlRect(const QStyleOptionComplex *option,
                                     SubControl sc,
                                     const QWidget *widget = nullptr) const;
    };

    typedef QList<AndroidItemStateInfo *> AndroidItemStateInfoList;

public:
    QAndroidStyle();
    ~QAndroidStyle();

    virtual void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                               const QWidget *w = nullptr) const;

    virtual void drawControl(QStyle::ControlElement element, const QStyleOption *opt, QPainter *p,
                             const QWidget *w = nullptr) const;

    virtual QRect subElementRect(SubElement subElement, const QStyleOption *option,
                                 const QWidget *widget = nullptr) const;
    virtual void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                                    const QWidget *widget = nullptr) const;
    virtual SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                             const QPoint &pt, const QWidget *widget = nullptr) const;
    virtual QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                                 SubControl sc, const QWidget *widget = nullptr) const;

    virtual int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                            const QWidget *widget = nullptr) const;

    virtual QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                                   const QSize &contentsSize, const QWidget *w = nullptr) const;

    virtual QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt = nullptr,
                                   const QWidget *widget = nullptr) const;

    virtual QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                        const QStyleOption *opt) const;

    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const;

    virtual QPalette standardPalette() const;
    void polish(QWidget *widget);
    void unpolish(QWidget *widget);

private:
    Q_DISABLE_COPY_MOVE(QAndroidStyle)
    static ItemType qtControl(QStyle::ComplexControl control);
    static ItemType qtControl(QStyle::ContentsType contentsType);
    static ItemType qtControl(QStyle::ControlElement controlElement);
    static ItemType qtControl(QStyle::PrimitiveElement primitiveElement);
    static ItemType qtControl(QStyle::SubElement subElement);
    static ItemType qtControl(const QString &android);

private:
    typedef QHash<int, AndroidControl *> AndroidControlsHash;
    AndroidControlsHash m_androidControlsHash;
    QPalette m_standardPalette;
    AndroidCompoundButtonControl *checkBoxControl;
};

QT_END_NAMESPACE

#endif // QANDROIDSTYLE_P_H
