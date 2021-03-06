//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//
#if PLATFORM_APPLE


class CameraFrameApple : public oak::CameraFrame {
public:
    CMSampleBufferRef sampleBuffer;
    
    virtual oak::Bitmap* asBitmap() override;
};

#endif
