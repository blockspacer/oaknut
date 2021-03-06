//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

class CameraFrame : public Object {
public:
    GLuint _textureId; // todo: this is android-specific?
    long _timestamp;
    int _width;
    int _height;

    virtual Bitmap* asBitmap()=0;
};


class Camera : public Object {
public:
    
    // Options.
    struct Options {
        bool frontFacing;
        int frameSizeShort; // e.g. 480
        int frameSizeLong;  // e.g. 640
        int frameRate;
    };

    // API
    static Camera* create(const Options& options);
    std::function<void (CameraFrame* frame)> onNewCameraFrame;
    int _previewWidth;
    int _previewHeight;

    virtual void open()=0;
    virtual void start()=0;
    virtual void stop()=0;
    virtual void close()=0;

protected:
    Camera(const Options& options);

    Options _options;
};

