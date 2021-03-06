//
// Copyright © 2020 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>


class ShadowShader : public Shader {
public:
    typedef uint32_t Features;
    
    ShadowShader(Renderer* renderer, Features features) : Shader(renderer) {
        declareAttribute("texcoord", VariableType::Float2);
        _u_halfSize = declareUniform("halfSize", VariableType::Float2);
        _u_cornerRadius = declareUniform("cornerRadius", VariableType::Float1);
        _u_sigma = declareUniform("sigma", VariableType::Float1, Uniform::Usage::Fragment);
    }
    string getFragmentSource() override {
        return
        SL_FLOAT1 " cornerRadius=12.0;\n"
        SL_FLOAT1 " sigma=" SL_UNIFORM(sigma) ";\n"
        // texcoord is fragment xy in surface coords
        SL_FLOAT2 " p=" SL_VERTEX_OUTPUT(texcoord) ";\n"
        SL_FLOAT2 " half_size = " SL_UNIFORM(halfSize) " - sigma*2.0;\n"
        "half_size -= cornerRadius;\n"
        SL_FLOAT1 " dist = -(length(max(abs(p)-half_size, 0.0)) - cornerRadius - 0.5);\n"
        SL_FLOAT1 " x = dist / (sigma * sqrt(0.5));\n"
        SL_FLOAT1 " s = sign(x), a = abs(x);\n"
        " x = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;\n"
        SL_FLOAT1 " erf = s - s / (x * x);\n"
        SL_FLOAT1 " integral = 0.5 + 0.5 * erf;\n"
        SL_OUTPIXVAL " = " SL_HALF4 "(0.0, 0.0, 0.0, 0.3 * integral);\n";
    }
    
    int16_t _u_halfSize;
    int16_t _u_cornerRadius;
    int16_t _u_sigma;
};

static ShaderFactory<ShadowShader> s_factory;

void ShadowRenderOp::validateShader(RenderTask* r) {
    if (!_shader) {
        _shader = s_factory.get(r->_renderer, 0);
    }
    _blendMode = BLENDMODE_NORMAL;
}

void ShadowRenderOp::prepareToRender(RenderTask* r, class Surface* surface) {
    RenderOp::prepareToRender(r, surface);
    ShadowShader* shader = _shader.as<ShadowShader>();
    r->setUniform(shader->_u_sigma, _sigma);
    r->setUniform(shader->_u_halfSize, VECTOR2(_rect.size.width/2, _rect.size.height/2));
   // r->setUniform(shader->_u_cornerRadius, _cornerRadius);
    r->setUniform(shader->_u_alpha, _alpha);
}

void ShadowRenderOp::setSigma(float sigma) {
    if (_sigma != sigma) {
        _sigma = sigma;
        invalidateBatch();
        updateRect();
    }
}
void ShadowRenderOp::setRect(const RECT& rect) {
    _baseRect = rect;
    updateRect();
}

void ShadowRenderOp::updateRect() {
    RECT rect = _baseRect;
    rect.inset(-_sigma*2, -_sigma*2);
    rect.origin.y += _sigma;
    RenderOp::setRect(rect);
}

void ShadowRenderOp::asQuads(QUAD *quad) {
    rectToSurfaceQuad(_rect, quad);
    // Put the quad size into the texture coords so the shader
    // can trivially calc distance to quad center
    quad->tl.s = quad->bl.s = -_rect.size.width/2;
    quad->tl.t = quad->tr.t = -_rect.size.height/2;
    quad->tr.s = quad->br.s = _rect.size.width/2;
    quad->bl.t = quad->br.t = _rect.size.height/2;
}

