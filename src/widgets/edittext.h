//
// Copyright © 2017 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//


class EditText : public Label, IKeyboardInputHandler, ITextInputReceiver {
public:

    // API
    EditText();
    virtual void setInsertionPoint(int32_t newInsertionPoint);
    
    // Overrides
    void setPadding(EDGEINSETS padding) override;
    IKeyboardInputHandler* getKeyboardInputHandler() override;
    ITextInputReceiver* getTextInputReceiver() override;
    bool onInputEvent(INPUTEVENT* event) override;
    bool setFocused(bool focused) override;
    void updateRenderOps() override;
    void layout() override;
    void detachFromWindow() override;
    void setText(const string& text) override;
    void setAttributedText(const AttributedString& text) override;
    void updateContentSize(float parentWidth, float parentHeight) override;
    
    // IKeyboardInputHandler
    void keyInputEvent(KeyboardInputEventType keyboardInputEventType, KeyboardInputSpecialKeyCode specialKeyCode, int osKeyCode, char32_t charCode) override;

    // ITextInputReceiver
    void insertText(string text, int replaceStart, int replaceEnd) override;
    void deleteBackward() override;
    int getTextLength() override;
    int getSelectionStart() override;
    int getInsertionPoint() override;
    string textInRange(int start, int end) override;
    void setSelectedRange(int start, int end) override;

protected:
    int _selectionStart;
    int _insertionPoint;
    bool _cursorOn;
    bool _cursorValid;
    bool _showClearButtonWhenNotEmpty;
    ObjPtr<Timer> _blinkCursorTimer;
    ObjPtr<RenderOp> _cursorRenderOp;
    ObjPtr<TextureRenderOp> _clearButtonOp;
    
    void updateCursor();
    void updateClearButton();
    void moveToLine(int dLine);
};

