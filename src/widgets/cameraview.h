//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#if OAKNUT_WANT_CAMERA

class CameraView : public View {
public:
    
    // API
    void show();
    void handleNewCameraFrame(Bitmap* newFrame);
    
    // Overrides
    virtual void attachToWindow(Window* window);
    virtual void detachFromWindow();

    
protected:
    TextureRenderOp* _renderOp;
};

#endif