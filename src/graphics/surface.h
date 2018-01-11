//
// Copyright © 2017 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

class PrivateSurfaceRenderOp : public TextureRenderOp {
public:
    QuadBuffer::Alloc* _alloc;
    bool _dirty;
    
    PrivateSurfaceRenderOp(View* view, const RECT& rect);
    ~PrivateSurfaceRenderOp();
    virtual void rectToSurfaceQuad(RECT rect, QUAD* quad);
    virtual void render(Canvas* canvas, Surface* surface);
};

class Surface : public Object {
public:
	SIZE _size;
    REGION _invalidRegion; // unused on primary surface
    Matrix4 _mvp;
    GLuint _fb;
    GLuint _tex;
    GLint _pixelType;
    GLint _pixelFormat;
    POINT _savedOrigin;
    list<ObjPtr<RenderBatch>> _listBatches;
    bool _supportsPartialRedraw;
    ObjPtr<PrivateSurfaceRenderOp> _op;
    int _mvpNum;
    
    Surface();
    Surface(View* owningView);
    ~Surface();
    void render(View* view, Canvas* canvas);
	void setSize(SIZE size);
	void setupPrivateFbo();
	void use();
    void cleanup();
  
    void detachViewOps(View* view);
    void attachViewOps(View* view);

    void addRenderOp(RenderOp* op);
    void removeRenderOp(RenderOp* op);
    
#ifdef DEBUG
    string _renderLog;
#endif
};



