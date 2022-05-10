// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TETRIXPIECE_H
#define TETRIXPIECE_H

enum TetrixShape { NoShape, ZShape, SShape, LineShape, TShape, SquareShape,
                   LShape, MirroredLShape };

//! [0]
class TetrixPiece
{
public:
    TetrixPiece() { setShape(NoShape); }

    void setRandomShape();
    void setShape(TetrixShape shape);

    TetrixShape shape() const { return pieceShape; }
    int x(int index) const { return coords[index][0]; }
    int y(int index) const { return coords[index][1]; }
    int minX() const;
    int maxX() const;
    int minY() const;
    int maxY() const;
    TetrixPiece rotatedLeft() const;
    TetrixPiece rotatedRight() const;

private:
    void setX(int index, int x) { coords[index][0] = x; }
    void setY(int index, int y) { coords[index][1] = y; }

    TetrixShape pieceShape;
    int coords[4][2];
};
//! [0]

#endif
