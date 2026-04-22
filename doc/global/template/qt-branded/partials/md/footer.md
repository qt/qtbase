{#-
Copyright (C) 2026 The Qt Company Ltd.
SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only
-#}
{% if hasNavigation %}
{% if existsIn(navigation, "prevLink") or existsIn(navigation, "nextLink") %}

---

{% if existsIn(navigation, "prevLink") %}[<< {{ navigation.prevLink.title }}]({{ navigation.prevLink.href }}){% endif %}{% if existsIn(navigation, "prevLink") and existsIn(navigation, "nextLink") %} | {% endif %}{% if existsIn(navigation, "nextLink") %}[{{ navigation.nextLink.title }} >>]({{ navigation.nextLink.href }}){% endif %}

{% endif %}
{% endif %}

---

&copy; 2025 The Qt Company Ltd.
Documentation contributions included herein are the copyrights of their
respective owners. The documentation provided herein is licensed under
the terms of the
[GNU Free Documentation License version 1.3](http://www.gnu.org/licenses/fdl.html)
as published by the Free Software Foundation. Qt and respective logos
are [trademarks](https://doc.qt.io/qt/trademarks.html) of The Qt
Company Ltd. in Finland and/or other countries worldwide. All other
trademarks are property of their respective owners.
