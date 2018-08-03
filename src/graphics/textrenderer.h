//
// Copyright © 2017 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

/**
 Generic text layout component. Effectively converts a string of codepoints (i.e. a string)
 into RenderOps. At present it is embarrassingly simple in the following ways:
 
   - Left-to-right latin script only. Apologies to the rest of world, this will be addressed soon.
   - Soft linebreaks are based on whitespace only. The proper logic for this is horrific
     (see http://www.unicode.org/reports/tr14/) and will be implemented some day, maybe by integrating Pango.
   - No composite glyphs. The platform layer renders whole characters, not individual glyphs.
   - No stylistic ligatures. The platform layer does not yet provide a way to get inside fonts to find out what ligatures are available. Also am not sure how to render them on any platform... there must be APIs that returns a glyph index given TWO characters ('ff', 'fi', etc) but damned if I know what they are.
   - No kerning. Again, the platform layer does not yet provide a way to get at this font information.
 */


class TextRenderer : public Object {
    
public:
    TextRenderer();
    void measure();
    void measure(SIZE maxSize);
    void layout(RECT rect);
    const SIZE measuredSize() { return _measuredSize; }
    void updateRenderOps(View* view);
    const string& getText() { return _text; }
    void setText(const string& text);
    const AttributedString& getAttributedText() { return _text; }
    void setAttributedText(const AttributedString& text);
    void setDefaultColour(COLOUR colour);
    COLOUR getDefaultColour() const { return _defaultColour; }
    void setDefaultFontName(const string& fontName);
    void setDefaultFontSize(float fontSize);
    void setDefaultFont(Font* font);
    const Font* getDefaultFont() const { return _defaultFont; }
    void setGravity(GRAVITY gravity);
    void setMaxLines(int maxLines);
    void getCharacterOrigin(int32_t characterIndex, POINT* origin, float* ascent, float* descent) const;
    
    
    
    typedef struct {
        int32_t codepointIndex; // index into _text of first codepoint of this character
        RECT rect;    // client coords. TODO: wasteful. Any given char rect is almost same as adjacent rects, esp. vertically. There's probably a dozen more efficient delta encodings could be used here.
        TEXTRENDERPARAMS* renderParams;
        int16_t numCodepoints; // one less than the number of codepoints used in this char, so usually 0. todo: wasteful, explore better ways once we have more control over char rendering
        Glyph* glyph; // todo: I envisage each DISPLAYED_CHAR pointing to a variably-sized array of glyphs.
        // NB: in the case of a ligature, the second char will have NULL glyph pointer
    } DISPLAYED_CHAR;

    // All the characters in all lines
    vector<DISPLAYED_CHAR> _characters;
    
    typedef struct {
        int32_t startCharacterIndex; // offset into _characters
        int32_t numCharacters; // the number of characters in this line, not including any terminating break character
        Font* font; // font at start of line
        RECT bounds; // in client coords
        float baseline; // offset from bounds.top to baseline
        bool ellipsis;
    } TEXTLINE;
    

    // Given a POINT, find the nearest text index
    int32_t characterIndexFromPoint(const POINT& pt) const;
    
    // Move the character index
    int32_t moveCharacterIndex(int32_t characterIndex, int dx, int dy) const;
    
    const TEXTLINE* getLineForCharacterIndex(int32_t charIndex, int dLine) const;

    friend class Label;
    
protected:
    AttributedString _text;
    ObjPtr<Font> _defaultFont;
    COLOUR _defaultColour;
    GRAVITY _gravity;
    bool _measuredSizeValid;
    SIZE _measuredSize;
    set<TEXTRENDERPARAMS> _renderParams;
    list<TextRenderOp*> _textOps;
    bool _renderOpsValid;
    vector<TEXTLINE> _lines;
    int _maxLines; // 0=as many as needed, >=1 = text will ellipsize on the last line
    RECT _layoutRect;

};
