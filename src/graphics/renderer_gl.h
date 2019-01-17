//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#if RENDERER_GL

class GLTexture : public Texture {
public:
    GLuint _texTarget;       // default GL_TEXTURE_2D
    GLuint _textureId;
    bool _paramsValid = false;
    
    
    GLTexture(Renderer* renderer);
    virtual void upload();

    void resize(int width, int height) override;
    int getSampler() override;
    
protected:
    void realloc(int width, int height, void* pixelData, bool sizeChanged);
};


// GL Sampler types. There are 11 of these in the GLSL spec, see:
// https://www.khronos.org/opengl/wiki/Sampler_(GLSL)#Sampler_types

#define GLSAMPLER_NONE 0
#define GLSAMPLER_TEXTURE_2D 1
#define GLSAMPLER_TEXTURE_EXT_OES 2


class GLRenderer : public Renderer {
public:

    // Overrides
    Surface* getPrimarySurface() override;
    Surface* createPrivateSurface() override;
    void setCurrentSurface(Surface* surface) override;
    void* createShaderState(Shader* shader) override;
    void deleteShaderState(void* state) override;
    void bindCurrentShader() override;
    void setCurrentTexture(Texture* texture) override;
    void setCurrentBlendMode(int blendMode) override;
    Texture* createTexture() override;
    void releaseTexture(Texture* texture) override;
    void prepareToDraw() override;
    void pushClip(RECT clip) override;
    void popClip() override;
    void flushQuadBuffer() override;
    void uploadQuad(ItemPool::Alloc* alloc) override;
    void drawQuads(int numQuads, int index) override;
    void copyFromCurrent(const RECT& rect, Texture* destTex, const POINT& destOrigin) override;
    void generateMipmaps(Texture* tex) override;
    
    void setUniformData(int16_t uniformIndex, const void* data, int32_t cb) override;

    // Helpers
    void convertTexture(GLTexture* texture, int width, int height);
    
    int getIntProperty(IntProperty property) override;
    
protected:
    GLRenderer(Window* window);

    GLfloat _backgroundColor[4]; // TODO: this belongs on GLSurface...
    GLuint _indexBufferId;
    GLuint _vertexBufferId;
    GLuint _vao;
    GLshort* _indexes;
};

#endif

