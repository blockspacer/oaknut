//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//
#pragma once




/**
 Supported bitmap formats. Bitmaps in other formats must be converted by platform code
 into one of these (see Bitmap::createXXX()).
*/
#define BITMAPFORMAT_UNKNOWN 0
#define BITMAPFORMAT_RGBA32  1
#define BITMAPFORMAT_RGB565  2
#define BITMAPFORMAT_BGRA32  3   // iOS only
#define BITMAPFORMAT_A8      4   // Not supported in WebGL, convert to RGBA instead

typedef struct {
    void* data;
    int cb;
    int stride;
} PIXELDATA;

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif


/**
 * @ingroup graphics_group
 * @class Bitmap
 * @brief The standard 2D array of color values.
 *
 * Bitmap is a platform type, i.e. apps must instantiate them via one of the
 * Bitmap::createXXX() APIs, never with operator new().
 *
 * The Bitmap class does not directly own a block of RAM for pixel data, the
 * storage of pixel data is left to the platform layer. When application or
 * Oaknut code needs to access the pixel data it has to lock() and unlock() the
 * bitmap. This may or may not cause an expensive blit, it's up to the platform.
 * Only access pixel data when you really need to.
 *
 * Bitmap formats
 * ==============
 * Oaknut bitmaps have three core formats: RGBA32, RGB16, and A8. Apple platforms
 * allows a fourth format, BGRA32, in order to avoid needless conversion.
 *
 * On the web the only format supported is RGBA32 so bitmap data in other formats
 * will be converted.
 *
 */
class Bitmap : public Object, public ISerializeToVariant {
public:
    int32_t _width;
    int32_t _height;
    int32_t _format;
    GLuint _texSampleMethod; // default GL_LINEAR
    GLuint _texTarget;       // default GL_TEXTURE_2D
    GLuint _textureId;
    bool _allocdTexData;
    bool _paramsValid;
    bool _needsUpload;
    bool _hasPremultipliedAlpha;
    list<Bitmap*>::iterator _renderContextIt;
    
    // Constructor is protected, use Bitmap::createXXX() APIs to instantiate
protected:
    Bitmap(int width, int height, int format);
    Bitmap();
    ~Bitmap();
public:

    // Accessing pixel data
    virtual void lock(PIXELDATA* pixelData, bool forWriting)=0;
    virtual void unlock(PIXELDATA* pixelData, bool pixelsChanged)=0;
    uint8_t* pixelAddress(PIXELDATA* pixelData, int x, int y);

    // Rendering
    virtual void bind();
    virtual void onRenderContextDestroyed();
    
    int getBytesPerPixel();
    GLenum getGlFormat();
    GLenum getGlInternalFormat();
    int getGlPixelType();
    bool hasAlpha();
    bool hasPremultipliedAlpha();
    
    int sizeInBytes();
    Bitmap* convertToFormat(int newFormat);
    
    // ISerializeToVariant
    void fromVariant(const variant& v) override;
    void toVariant(variant& v) override;
    

    // Platform-specific instantiation
    static Bitmap* create(int width, int height, int format);
    static Task* createFromData(const void* data, int cb, std::function<void(Bitmap*)> callback);

};


