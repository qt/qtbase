// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
#include "textedit.h"
#include <QApplication>
#include <QProxyStyle>

class MyProxyStyle : public QProxyStyle
{
  public:
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override
    {
        if (hint == QStyle::SH_UnderlineShortcut)
            return 0;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(textedit);

    QApplication a(argc, argv);
    a.setStyle(new MyProxyStyle);
    TextEdit mw;
    mw.resize(700, 800);
    mw.show();
    //...
}
//! [1]

//! [2]
...
auto proxy = new MyProxyStyle(QApplication::style()->name());
proxy->setParent(widget);   // take ownership to avoid memleak
widget->setStyle(proxy);
...
//! [2]
