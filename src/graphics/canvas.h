//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//
#pragma once

class Path : public Object {
public:
    virtual void moveTo(POINT pt)=0;
    virtual void lineTo(POINT pt)=0;
    virtual void curveTo(POINT ctrl1, POINT ctrl2, POINT pt)=0;
};

/**
 * @ingroup graphics_group
 * @class Canvas
 * @brief Basic 2D Canvas API.
 
 Canvas is a platform type, instantiate via `Canvas::create()`.
 
 NB: This is VERY limited at present, features get added as they become needed.
 */
class Canvas : public Object {
public:
    static Canvas* create();

    virtual void resize(int width, int height)=0;
    virtual Bitmap* getBitmap()=0;
    virtual void clear(COLOR color)=0;
    virtual void setFillColor(COLOR color)=0;
    virtual void setStrokeColor(COLOR color)=0;
    virtual void setStrokeWidth(float strokeWidth)=0;
    virtual void setTransform(const AFFINE_TRANSFORM& t)=0;
    virtual void clearTransform()=0;
    virtual void drawRect(RECT rect)=0;
    virtual void drawOval(RECT rect)=0;
    virtual void drawPath(Path* path)=0;
    virtual void drawBitmap(Bitmap* bitmap, const RECT& rectSrc, const RECT& rectDst)=0;
    virtual Path* createPath()=0;

protected:
    Canvas();
};

