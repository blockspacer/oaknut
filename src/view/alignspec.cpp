//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>


ALIGNSPEC::ALIGNSPEC() {
}
ALIGNSPEC::ALIGNSPEC(View* anchor, float multiplierAnchor, float multiplierSelf, float margin) {
    this->anchor = anchor;
    this->multiplierAnchor = multiplierAnchor;
    this->multiplierSelf = multiplierSelf;
    this->margin = margin;
}

ALIGNSPEC ALIGNSPEC::None()   { return ALIGNSPEC(NO_ANCHOR, 0,0,0); }
ALIGNSPEC ALIGNSPEC::Left()   { return ALIGNSPEC(NULL, 0.0f, 0.0f, 0); }
ALIGNSPEC ALIGNSPEC::Abs(float x)   { return ALIGNSPEC(NULL, 0.0f, 0.0f, x); }
ALIGNSPEC ALIGNSPEC::Center() { return ALIGNSPEC(NULL, 0.5f,-0.5f, 0); }
ALIGNSPEC ALIGNSPEC::Right()  { return ALIGNSPEC(NULL, 1.0f,-1.0f, 0); }
ALIGNSPEC ALIGNSPEC::Top()    { return ALIGNSPEC(NULL, 0.0f, 0.0f, 0); }
ALIGNSPEC ALIGNSPEC::Bottom() { return ALIGNSPEC(NULL, 1.0f,-1.0f, 0); }

ALIGNSPEC ALIGNSPEC::ToLeftOf(View* view, float margin) {
    return ALIGNSPEC(view, 0.0f,  -1.0f, -margin);
}
ALIGNSPEC ALIGNSPEC::ToRightOf(View* view, float margin) {
    return ALIGNSPEC(view, 1.0f,  0.0f, margin);
}
ALIGNSPEC ALIGNSPEC::Above(View* view, float margin) {
    return ALIGNSPEC(view, 0.0f,  -1.0f, -margin);
}
ALIGNSPEC ALIGNSPEC::Below(View* view, float margin) {
    return ALIGNSPEC(view, 1.0f,  0.0f, margin);
}


ALIGNSPEC::ALIGNSPEC(const variant& value, View* view) {
    anchor = NULL;
    if (value.isNumeric()) {
        multiplierAnchor = 0;
        multiplierSelf = 0;
        margin = value.floatVal();
        return;
    }
    string str = value.stringVal();
    string type = str.tokenise("(");
    str.hadSuffix(")");

    bool anchorMandatory = false;
    if (type=="center") *this=Center();
    else if (type=="centre") *this=Center();
    else if (type=="left") *this=Left();
    else if (type=="right") *this=Right();
    else if (type=="top") *this=Top();
    else if (type=="bottom") *this=Bottom();
    else if (type=="toLeftOf") {multiplierAnchor=0.0f; multiplierSelf=-1.0f; anchorMandatory=true; }
    else if (type=="toRightOf") {multiplierAnchor=1.0f; multiplierSelf=0.0f; anchorMandatory=true; }
    else if (type=="above") {multiplierAnchor=0.0f; multiplierSelf=-1.0f; anchorMandatory=true; }
    else if (type=="below") {multiplierAnchor=1.0f; multiplierSelf=0.0f; anchorMandatory=true; }
    else assert(false); // unknown alignspec type

    margin = 0;

    // Arguments: if only one argument then its either anchor or margin.
    if (str.length() > 0) {
        string anchorName;
        StringProcessor proc(str);
        string tok1 = proc.nextToken(); //.parse(proc, PARSEFLAG_IS_ARGUMENT);
        StringProcessor proc1(tok1);
        style arg1;
        arg1.parse(proc1);
        proc.skipWhitespace();
        if (proc.eof()) {
            if (arg1.isString()) {
                anchorName = arg1.stringVal();
            } else {
                margin  = arg1.floatVal();
            }
        } else {
            variant arg2 = variant::parse(proc, 0);
            assert(arg1.isString()); // if two args provided, first must be anchor
            assert(arg2.isNumeric()); // and second must be margin
            anchorName = arg1.stringVal();
            margin = arg2.floatVal();
            assert(proc.eof());
        }
        if (anchorName.length()) {
            anchor = view->getParent()->findViewById(anchorName);
            assert(anchor); // NB: anchor must be previously declared. TODO: remove this restriction
        }
    }
    
    // If an anchor is required but none was declared, implicitly anchor to previously-declared view
    if (anchorMandatory && !anchor) {
        int index = view->getParent()->indexOfSubview(view);
        assert(index>=1);
        anchor = view->getParent()->getSubview(index-1);
    }
}

float ALIGNSPEC::calc(float measuredSize, float refOrigin, float refSize) const {
    
    if (anchor == NO_ANCHOR) {
        return refOrigin;
    }
    float val = refOrigin + (multiplierAnchor * refSize)
              + (multiplierSelf * measuredSize)
              + margin;
    return floorf(val);
    
}

