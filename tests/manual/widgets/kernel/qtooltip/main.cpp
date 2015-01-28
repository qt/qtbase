/****************************************************************************
**
** Copyright (C) 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
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
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QTest>
#include <QDialog>
#include <QToolTip>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QProxyStyle>
#include <QSpinBox>

class QToolTipTest : public QProxyStyle
{
    Q_OBJECT
public:
    QToolTipTest() : QProxyStyle()
    {
        wakeTime = QApplication::style()->styleHint(SH_ToolTip_WakeUpDelay);
        sleepTime = QApplication::style()->styleHint(SH_ToolTip_FallAsleepDelay);
    }

    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const
    {
        switch (hint) {
            case SH_ToolTip_WakeUpDelay:
                return wakeTime;
            case SH_ToolTip_FallAsleepDelay:
                return sleepTime;
            default:
                return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    }

public slots:
    void setWakeTime(int wake) { wakeTime = wake; }
    void setSleepTime(int sleep) { sleepTime = sleep; }
protected:
    int wakeTime;
    int sleepTime;
};

class TestDialog : public QDialog
{
    Q_OBJECT
public:
    TestDialog(QToolTipTest *s);
    QToolTipTest *style;
protected slots:
    void showSomeToolTips();
};

void TestDialog::showSomeToolTips()
{
    QPoint p(100 + 20, 100 + 20);

    for (int u = 1; u < 20; u += 5) {
        QString s = tr("Seconds: ") + QString::number(u);
        QToolTip::showText(p, s, 0, QRect(), 1000 * u);
        QTest::qWait((u + 1) * 1000);
    }

    QToolTip::showText(p, tr("Seconds: 2"), 0, QRect(), 2000);
    QTest::qWait(3000);

    QToolTip::showText(p, tr("Standard label"), 0, QRect());
    QTest::qWait(12000);
}

TestDialog::TestDialog(QToolTipTest *s) : style(s)
{
    // Notice that these tool tips will disappear if another tool tip is shown.
    QLabel *label1 = new QLabel(tr("Tooltip - Only two seconds display"));
    label1->setToolTip(tr("2 seconds display"));
    label1->setToolTipDuration(2000);
    Q_ASSERT(label1->toolTipDuration() == 2000);

    QLabel *label2 = new QLabel(tr("Tooltip - 30 seconds display time"));
    label2->setToolTip(tr("30 seconds display"));
    label2->setToolTipDuration(30000);

    QPushButton *pb = new QPushButton(tr("&Test"));
    pb->setToolTip(tr("Show some tool tips."));
    Q_ASSERT(pb->toolTipDuration() == -1);
    connect(pb, SIGNAL(clicked()), this, SLOT(showSomeToolTips()));

    QLabel *wakeLabel = new QLabel(tr("Wake Delay:"));
    QSpinBox *wakeSpinBox = new QSpinBox();
    wakeSpinBox->setRange(0, 100000);
    wakeSpinBox->setValue(style->styleHint(QStyle::SH_ToolTip_WakeUpDelay));
    connect(wakeSpinBox, SIGNAL(valueChanged(int)), style, SLOT(setWakeTime(int)));

    QLabel *sleepLabel = new QLabel(tr("Sleep Delay:"));
    QSpinBox *sleepSpinBox = new QSpinBox();
    sleepSpinBox->setRange(0, 100000);
    sleepSpinBox->setValue(style->styleHint(QStyle::SH_ToolTip_FallAsleepDelay));
    connect(sleepSpinBox, SIGNAL(valueChanged(int)), style, SLOT(setSleepTime(int)));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label1);
    layout->addWidget(label2);
    layout->addWidget(pb);
    layout->addWidget(wakeLabel);
    layout->addWidget(wakeSpinBox);
    layout->addWidget(wakeLabel);
    layout->addWidget(sleepLabel);
    layout->addWidget(sleepSpinBox);

    setLayout(layout);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QToolTipTest *style = new QToolTipTest();
    QApplication::setStyle(style);
    TestDialog dlg(style);
    dlg.show();
    return app.exec();
}

#include "main.moc"
