//
// Copyright © 2017 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>

DECLARE_DYNCREATE(Label);

/*
 Setting text, font, or anything else that affects text size invalidates the content
 size. To determine the content size the text is initially processed into lines and
 renderparams (unique text styles).
 
 Second stage processing occurs on the first render after layout & updateContentSize,
 where the Label must determine which lines are visible and build the appropriate ops.
 
 Second stage processing also occurs when contentOffset changes but effort is
 made to minimise the amount of work done, i.e. nothing happens unless lines are
 scrolled in or out of view.
 
 Line spacing
 ============
 Line bounds are determined by the largest ascent and descent of the glyphs in it, i.e.
 the bounds rect tightly fits the glyphs that make up the line.
 
 Line spacing (the distance from the baseline to the baseline above) is determined
 mainly by the height of the tallest font in the line. It is possible for freakishly
 tall glyphs to exceed the font height, eg Ǻ (improperly rendered in xcode, the acute
 accent above the ring is missing!) in which case the line height is raised to :
 
  tallestGlyph.ascent+tallestGlyph.descent + (font.height-(font.ascent+font.descent)).
 
 
 */




Label::Label() : View() {
    string defaultFontName = app.getStyleString("font-name");
    float defaultFontSize = app.getStyleFloat("font-size");
    _textRenderer.setDefaultFont(new Font(defaultFontName, defaultFontSize));
    _textRenderer.setDefaultColour(0xFF000000);
    _textRenderer.setGravity(_gravity);
}
Label::~Label() {	
}


bool Label::applyStyleValue(const string& name, StyleValue* value) {
    if (name=="font-name") {
        setFont(new Font(value->str, _textRenderer.getDefaultFont()->_size));
        return true;
    }
    if (name=="font-size") {
        setFont(new Font(_textRenderer.getDefaultFont()->_name, value->getAsFloat()));
        return true;
    }
    if (name=="forecolour") {
        setTextColour(value->i);
        return true;
    }
    if (name=="text") {
        setText(value->str);
        return true;
    }
    if (name=="maxLines") {
        setMaxLines(value->i);
        return true;
    }
    if (name=="gravityX") {
        if (value->str == "left") {
            _gravity.horz = GRAVITY_LEFT;
        } else if (value->str == "right") {
            _gravity.horz = GRAVITY_RIGHT;
        } else if (value->str == "center" || value->str == "centre") {
            _gravity.horz = GRAVITY_CENTER;
        } else {
            assert(0);
        }
        _textRenderer.setGravity(_gravity);
        return true;
    }
    if (name=="gravityY") {
        if (value->str == "top") {
            _gravity.vert = GRAVITY_TOP;
        } else if (value->str == "bottom") {
            _gravity.vert = GRAVITY_BOTTOM;
        } else if (value->str == "center" || value->str == "centre") {
            _gravity.vert = GRAVITY_CENTER;
        } else {
            assert(0);
        }
        _textRenderer.setGravity(_gravity);
        return true;
    }
    return View::applyStyleValue(name, value);
}

void Label::setText(const string& text) {
    _textRenderer.setText(text);
    invalidateContentSize();
}

string vformat(const char *fmt, va_list ap)  {
    // Allocate a buffer on the stack that's big enough for us almost
    // all the time.  Be prepared to allocate dynamically if it doesn't fit.
    size_t size = 1024;
    char stackbuf[1024];
    std::vector<char> dynamicbuf;
    char *buf = &stackbuf[0];
    va_list ap_copy;

    while (1) {
        // Try to vsnprintf into our buffer.
        va_copy(ap_copy, ap);
        int needed = vsnprintf (buf, size, fmt, ap);
        va_end(ap_copy);

        // NB. C99 (which modern Linux and OS X follow) says vsnprintf
        // failure returns the length it would have needed.  But older
        // glibc and current Windows return -1 for failure, i.e., not
        // telling us how much was needed.

        if (needed <= (int)size && needed >= 0) {
            // It fit fine so we're done.
            return std::string (buf, (size_t) needed);
        }

        // vsnprintf reported that it wanted to write more characters
        // than we allotted.  So try again using a dynamic buffer.  This
        // doesn't happen very often if we chose our initial size well.
        size = (needed > 0) ? (needed+1) : (size*2);
        dynamicbuf.resize (size);
        buf = &dynamicbuf[0];
    }
}
void Label::setText(const char* format, ...) {
    va_list ap;
    va_start (ap, format);
    string str = vformat (format, ap);
    va_end (ap);
    setText(str);
}

void Label::setFont(Font* font) {
    _textRenderer.setDefaultFont(font);
    invalidateContentSize();
}

void Label::setMaxLines(int maxLines) {
    _textRenderer.setMaxLines(maxLines);
    invalidateContentSize();
}


void Label::setTextColour(COLOUR colour) {
    _defaultColour = colour;
    onEffectiveTintColourChanged();
}

void Label::onEffectiveTintColourChanged() {
    _textRenderer.setDefaultColour(_effectiveTintColour ? _effectiveTintColour : _defaultColour);
    setNeedsFullRedraw();
}



void Label::updateContentSize(float parentWidth, float parentHeight) {

    _contentSize.width = 0;
    _contentSize.height = 0;

    parentWidth -= (_padding.left + _padding.right);
    parentHeight -= (_padding.top + _padding.bottom);
    
    _textRenderer.measure(SIZE_Make(parentWidth, parentHeight));
    
    _contentSize = _textRenderer.measuredSize();
    // If we had to use a soft linebreak then we know we filled the available width
    //if (_textRenderer.hasSoftLineBreaks) {
    //    _contentSize.width = parentWidth;
    //}

    // Flag that renderOps will need updating after layout
    _textRendererMustRelayout = true;
    _updateRenderOpsNeeded = true;
}


void Label::updateRenderOps() {
    if (_textRendererMustRelayout) {
        _textRenderer.layout(getOwnRectPadded());
        _textRendererMustRelayout = false;
    }
    _textRenderer.updateRenderOps(this);
}

void Label::layout() {
	View::layout();

    _textRenderer.layout(getOwnRectPadded());
    _textRendererMustRelayout = false;
    _updateRenderOpsNeeded = true;
}

void Label::setGravity(GRAVITY gravity) {
    View::setGravity(gravity);
    _textRenderer.setGravity(gravity);
}

void Label::setStyle(string styleName) {
    // TODO: this method should just fetch the named map from Styles:: and call View::applyStyleValues()
    string fontName = app.getStyleString(styleName + ".font-name");
    float fontSize = app.getStyleFloat(styleName + ".font-size");
    if (fontName.size() && fontSize) {
        setFont(new Font(fontName, fontSize));
    }
    COLOUR textColour = app.getStyleColour(styleName + ".forecolour");
    if (textColour) {
        setTextColour(textColour);
    }
}

#ifdef DEBUG

string Label::debugViewType() {
    char ach[256];
    snprintf(ach, 256, "Label:%s", _textRenderer.getText().data());
    return string(ach);
}

#endif