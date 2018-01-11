//
// Copyright © 2017 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

class CanvasView : public View {
public:
    CanvasView();
    
    // Overrides
    virtual void layout();
    
protected:
    virtual void redraw();
    
    void* _canvas;
    ObjPtr<TextureRenderOp> _textureRenderOp;
};


