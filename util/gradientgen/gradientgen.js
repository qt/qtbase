#! /usr/bin/env node

/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
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

const _ = require('lodash');
const fs = require('fs');

const postcss = require('postcss');
const minifyGradients = require('postcss-minify-gradients');
const valueParser = require('postcss-value-parser');
const parseColor = require('parse-color');
const math = require('mathjs');

const argc = process.argv.length;
if (argc < 3) {
  console.log("usage: gradientgen [mode] <filename>");
  process.exit(1);
}

const filename = process.argv[argc - 1];
const mode = argc > 3 ? process.argv[argc - 2] : 'json';

fs.readFile(filename, (err, css) => {
  postcss([minifyGradients]).process(css)
    .then(result => {
      let enums = [];
      let gradients = [];

      result.root.walkRules(rule => {
        gradients.push(null); // Placeholder

        const name = _.startCase(rule.selector).replace(/\s/g, '');
        if (enums.indexOf(name) >= 0)
          return; // Duplicate entry

        // We can only support single gradient declarations
        if (rule.nodes.length > 1)
          return;

        valueParser(rule.nodes[0].value).walk(node => {
          if (node.type !== 'function')
            return;

          if (node.value !== 'linear-gradient')
            return;

          const args = node.nodes.reduce((args, arg) => {
              if (arg.type === 'div')
                args.push([]);
              else if (arg.type !== 'space')
                  args[args.length - 1].push(arg.value);
              return args;
          }, [[]]);

          let angle = valueParser.unit(args[0][0]);
          if (angle.unit !== 'deg')
            return;

          angle = parseInt(angle.number);
          if (angle < 0)
            angle += 360;

          // Angle is in degrees, but we need radians
          const radians = angle * math.pi / 180;

          const gradientLine = (math.abs(math.sin(radians)) + math.abs(math.cos(radians)));
          const cathetus = fn => math.round(fn(radians - math.pi / 2) * gradientLine / 2, 10);

          const x = cathetus(math.cos);
          const y = cathetus(math.sin);

          const start = { x: 0.5 - x, y: 0.5 - y };
          const end = { x: 0.5 + x, y: 0.5 + y };

          let stops = []

          let lastPosition = 0;
          args.slice(1).forEach((arg, index) => {
            let [color, position = !index ? '0%' : '100%'] = arg;
            position = parseInt(position) / 100;
            if (position < lastPosition)
              position = lastPosition;
            lastPosition = position;
            color = parseColor(color).hex;
            color = parseInt(color.slice(1), 16)
            stops.push({ color, position })
          });

          gradients[gradients.length - 1] = { start, end, stops };
        });

        if (!gradients[gradients.length - 1])
          return; // Not supported

        enums.push(name);

        if (mode == 'debug')
          console.log(name, args, gradients[gradients.length - 1])
        else if (mode == 'enums')
          console.log(`${name} = ${gradients.length},`)
      });

      // Done walking declarations
      if (mode == 'json')
        console.log(JSON.stringify(gradients, undefined, 4));
    });
});
