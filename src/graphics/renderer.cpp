//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>

RenderResource::RenderResource(Renderer* renderer) : _renderer(renderer) {
}

Texture::Texture(Renderer* renderer, int format) : RenderResource(renderer) {
    _it = renderer->_textures.insert(renderer->_textures.end(), this);
    _type = Normal;
    _format = format;
}
Texture::~Texture() {
    if (_renderer) {
        _renderer->releaseTexture(this);
    }
}

Shader::Shader(Renderer* renderer) : Shader(renderer, Shader::Features()) {
}
Shader::Shader(Renderer* renderer, Shader::Features features) : RenderResource(renderer), _features(features) {
    _it = renderer->_shaders.insert(renderer->_shaders.end(), this);
    declareAttribute("position", VariableType::Float2);
    _u_mvp = declareUniform("mvp", VariableType::Matrix4, Uniform::Vertex);
    bool useTexCoordAttrib = false;
    if (_features.textures[0]) {
        useTexCoordAttrib = true;
    }
    if (_features.alpha) {
        _u_alpha = declareUniform("alpha", VariableType::Float1);
    }
    if (_features.roundRect) {
        _u_strokeColor = declareUniform("strokeColor", VariableType::Color);
        _u_u = declareUniform("u", VariableType::Float4);
        if (_features.roundRect == SHADER_ROUNDRECT_1) {
            _u_radius = declareUniform("radius", VariableType::Float1);
        } else {
            _u_radii = declareUniform("radii", VariableType::Float4);
        }
        useTexCoordAttrib = true;
    }
    
    if (useTexCoordAttrib) {
        declareAttribute("texcoord", VariableType::Float2);
    }
    declareAttribute("color", VariableType::Color);
}

Shader::~Shader() {
    if (_renderer) {
        _renderer->releaseShader(this);
    }
}


int16_t Shader::Uniform::length() {
    switch (type) {
        case Color: return SL_SIZEOF_COLOR;
        case Int1:  return 4;
        case Float1: return 4;
        case Float2: return 8;
        case Float4: return 16;
        case Matrix4: return 64;
    }
    assert(0);
    return 4;
}


int16_t Shader::declareAttribute(const string& name, VariableType type, string outValue) {
    Attribute attribute;
    attribute.name = name;
    attribute.type = type;
    attribute.outValue = outValue;
    _attributes.push_back(attribute);
    return 0;
}

int16_t Shader::declareUniform(const string& name, VariableType type, Uniform::Usage usage) {
    Uniform uniform;
    uniform.name = name;
    uniform.usage = usage;
    uniform.type = type;    
    _uniforms.push_back(uniform);
    return (int16_t)(_uniforms.size()-1);
}



string oak::sl_getTypeString(Shader::VariableType type) {
    switch (type) {
        case Shader::VariableType::Color: return SL_HALF4_DECL;
        case Shader::VariableType::Int1: return "int";
        case Shader::VariableType::Float1: return SL_FLOAT1;
        case Shader::VariableType::Float2: return SL_FLOAT2;
        case Shader::VariableType::Float4: return SL_FLOAT4;
        case Shader::VariableType::Matrix4: return SL_MATRIX4;
    }
}


string Shader::getVertexSource() {
    return "";
}


string Shader::getFragmentSource() {
    bool useTexCoords = false;
    bool useTexSampler = false;
    int roundRect = _features.roundRect;
    if (roundRect) {
        useTexCoords = true; // we don't use the sampler, we just want the texcoord attributes, which are
        // not actually texture coords, v_texcoords is x-dist and y-dist from quad centre
    }
    if (_features.textures[0]) {
        useTexSampler = true;
        useTexCoords = true;
        //_sampler.set(0);
    }
    
    
    
    
    string fs = "c = ";
    if (useTexSampler) {
        switch (_features.textures[0]) {
            case Texture::Type::None:
                break;
            case Texture::Type::Normal:
                fs += SL_TEXSAMPLE_2D("texture",  SL_VERTEX_OUTPUT(texcoord));
                break;
            case Texture::Type::Rect:
                fs += SL_TEXSAMPLE_2D_RECT("texture",  SL_VERTEX_OUTPUT(texcoord));
                break;
            case Texture::Type::OES:
                fs += SL_TEXSAMPLE_2D_OES("texture",  SL_VERTEX_OUTPUT(texcoord));
                break;
        }
        fs += ";\n";
        
        if (_features.tint) {
            fs += "    c.rgb = " SL_VERTEX_OUTPUT(color) ".rgb;\n";
        }
    } else {
        fs += SL_VERTEX_OUTPUT(color);
        fs += ";\n";

    }
    

    if (roundRect) {
        
        if (roundRect == SHADER_ROUNDRECT_1) {
            fs += SL_FLOAT2 " b = " SL_UNIFORM(u) ".xy - " SL_FLOAT2 "(" SL_UNIFORM(radius) ");\n"
                  "float dist = length(max(abs(" SL_VERTEX_OUTPUT(texcoord)")-b, 0.0)) - " SL_UNIFORM(radius) "  - 0.5;\n";
        }
        else if (roundRect == SHADER_ROUNDRECT_2H) {
            // branchless selection of radius=r.x if on left side of quad or radius=r.y on right side
            fs +=
            SL_FLOAT2 " size = " SL_UNIFORM(u) ".xy;\n"
            SL_FLOAT2 " r = " SL_UNIFORM(radii) ".xw\n;" // TODO: this is specific to left|right config
            "float s=step(" SL_VERTEX_OUTPUT(texcoord) ".x,0.0);\n"
            "float radius = s*r.x + (1.0-s)*r.y;\n"
            "size -= " SL_FLOAT2 "(radius);\n"
            SL_FLOAT2 " d = abs(" SL_VERTEX_OUTPUT(texcoord) ") - size;\n"
            "float dist = min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - radius;\n";
        }
        fs +=  SL_HALF4_DECL " col = " SL_HALF4 "(" SL_UNIFORM(strokeColor) ");\n"
        "   col.a = mix(" SL_HALF1 "(0.0), " SL_HALF1 "(" SL_UNIFORM(strokeColor) ".a), " SL_HALF1 "(clamp(-dist, 0.0, 1.0)));\n"   // outer edge blend
        SL_OUTPIXVAL " = mix(col, c, " SL_HALF4 "(clamp(-(dist + " SL_UNIFORM(u) ".w), 0.0, 1.0)));\n";
    }
    else {
        fs += SL_OUTPIXVAL " = c;\n";
    }
    
    
    if (_features.alpha) {
        fs += SL_OUTPIXVAL ".a *= " SL_UNIFORM(alpha) ";\n";
    }

    return fs;
}



Renderer::Renderer(Window* window) : _window(window), _quadBuffer(sizeof(QUAD), 256) {
    _primarySurfaceFormat = PIXELFORMAT_DEFAULT32;
}

Shader* Renderer::getStandardShader(Shader::Features features) {
    Shader* shader = _standardShaders[features];
    if (!shader) {
        shader = new Shader(this, features);
        _standardShaders[features] = shader;
    }
    return shader;    
}


void Renderer::reset() {
    _doneInit = false;
    //delete _quadBuffer; todo: fix this leak
    while (_textures.size() > 0) {
        releaseTexture((Texture*)*_textures.begin());
    }
    while (_shaders.size() > 0) {
        releaseShader((Shader*)*_shaders.begin());
    }
}


template<>
void Renderer::setUniform<int>(int16_t uniformIndex, const int& val) {
    setUniformData(uniformIndex, &val, sizeof(val));
}
template<>
void Renderer::setUniform<float>(int16_t uniformIndex, const float& val) {
    setUniformData(uniformIndex, &val, sizeof(val));
}
template<>
void Renderer::setUniform<MATRIX4>(int16_t uniformIndex, const MATRIX4& val) {
    setUniformData(uniformIndex, val.get(), 16*sizeof(float));
}
template<>
void Renderer::setUniform<COLOR>(int16_t uniformIndex, const COLOR& val) {
    auto& uniform = _currentShader->_uniforms[uniformIndex];
    if (uniform.cachedColorVal == val) {
        return;
    }
    uniform.cachedColorVal = val;
    float c[4];
    c[3] = ((val&0xff000000)>>24)/255.0f;
    c[2] = ((val&0xff0000)>>16)/255.0f;
    c[1] = ((val&0xff00)>>8)/255.0f;
    c[0] = (val&0xff)/255.0f;
    setColorUniform(uniformIndex, c);
}
template<>
void Renderer::setUniform<SIZE>(int16_t uniformIndex, const SIZE& val) {
    setUniformData(uniformIndex, &val, sizeof(val));
}
template<>
void Renderer::setUniform<VECTOR2>(int16_t uniformIndex, const VECTOR2& val) {
    setUniformData(uniformIndex, &val, sizeof(val));
}
template<>
void Renderer::setUniform<VECTOR4>(int16_t uniformIndex, const VECTOR4& val) {
    setUniformData(uniformIndex, &val, sizeof(val));
}



void Renderer::releaseTexture(Texture* tex) {
    if (tex->_renderer) {
        assert(tex->_renderer == this);
        _textures.erase(tex->_it);
        tex->_renderer = NULL;
    }
}

void Renderer::releaseShader(Shader* shader) {
    if (shader->_renderer) {
        assert(shader->_renderer == this);
        if (shader->_shaderState) {
            deleteShaderState(shader->_shaderState);
            shader->_shaderState = NULL;
        }
        _shaders.erase(shader->_it);
        shader->_renderer = NULL;
    }
}

void Renderer::createTextureForBitmap(Bitmap* bitmap) {
    bitmap->_texture = createTexture(bitmap->_format);
    bitmap->_texture->_bitmap = bitmap;
    bitmap->_texture->_minFilterLinear = !bitmap->_sampleNearest;
    bitmap->_texture->_magFilterLinear = !bitmap->_sampleNearest;
    bitmap->_texture->resize(bitmap->_width, bitmap->_height);
    bitmap->_texture->_needsUpload = true;
}

void Renderer::bindBitmap(Bitmap* bitmap) {
    assert(bitmap);
    if (!bitmap->_texture) {
        createTextureForBitmap(bitmap);
    }
    setCurrentTexture(bitmap->_texture);
}

void Renderer::setCurrentShader(Shader *shader) {
    if (_currentShader != shader) {
        if (!shader->_shaderState) {
            shader->_shaderState = createShaderState(shader);
        }
        _currentShader = shader;
        bindCurrentShader();
    }
}

ItemPool::Alloc* Renderer::allocQuads(int num, ItemPool::Alloc* existingAlloc) {
    return _quadBuffer.alloc(num, existingAlloc);
}


void Renderer::invalidateQuads(ItemPool::Alloc* alloc) {
    uint8_t* lo = alloc->addr();
    uint8_t* hi = lo + alloc->cb();
    if (!_quadBufferDirtyLo) {
        _quadBufferDirtyLo = lo;
        _quadBufferDirtyHi = hi;
    } else {
        _quadBufferDirtyLo = MIN(_quadBufferDirtyLo, lo);
        _quadBufferDirtyHi = MAX(_quadBufferDirtyHi, hi);
    }
}



