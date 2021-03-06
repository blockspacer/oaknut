//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>


Animation::Animation(View* view) : Animation(view, 0,1) {
}
Animation::Animation(View* view, float fromVal, float toVal) : _view(view), _interpolator(linear),
												   _fromVal(fromVal),
												   _toVal(toVal) {
}

Animation::~Animation() {
}

void Animation::start(int duration) {
    assert(_view && _view->_window);
    _view->_window->startAnimation(this, duration,0);
}
void Animation::start(int duration, int delay) {
    assert(_view && _view->_window);
    _view->_window->startAnimation(this, duration, delay);
}

void Animation::setInterpolater(InterpolateFunc interpolator) {
    _interpolator = interpolator;
}
InterpolateFunc Animation::getInterpolater() const {
    return _interpolator;
}

class SimpleAnimation : public Animation {
public:
    SimpleAnimation(View* view, std::function<void(float)> callback) : Animation(view), _callback(callback) {
        _fromVal = 0;
        _toVal = 1;
    }
    
    void apply(float val) {
        _callback(val);
    }

    
protected:
    std::function<void(float)> _callback;
};

Animation* Animation::start(View* view, int duration, std::function<void(float)> callback, InterpolateFunc interpolator) {
    sp<Animation> anim = new SimpleAnimation(view, callback);
    anim->_interpolator = interpolator;
    anim->start(duration);
    return anim;
}


void Animation::stop() {
    if (_window) {
        _window->stopAnimation(this);
    }
}

void Animation::pause() {
    if (!_window || _paused) {
        return;
    }
    _paused = true;
    _elapsedAtPause = app->currentMillis() - _timeStarted;
}

void Animation::unpause() {
    if (!_paused) {
        return;
    }
    _timeStarted = app->currentMillis() - _elapsedAtPause;
    _elapsedAtPause = 0;
    _paused = false;
}


bool Animation::tick(TIMESTAMP now) {
	float elapsed = now - (_timeStarted + _delay);
	if (elapsed<0) { // not started yet
		return true;
	}

	// Clamp elapsed time to duration. If duration has been exceeded set 'finished' true.
	bool finished = false;
	finished = (elapsed >= _duration) ;
	if (finished) {
		elapsed = _duration;
	}
        
	// Call apply()
    float val = finished?1:_interpolator(elapsed, 0, 1, _duration);
	apply(_fromVal + (_toVal-_fromVal)*val);
        
	// If animation has finished
	if (finished) {
		if (onFinished) {
			onFinished(this);
        }
    }
    return !finished;
}



AlphaAnimation::AlphaAnimation(View* view, float target) : Animation(view) {
	_fromVal = view->getAlpha();
	_toVal = target;
}
void AlphaAnimation::apply(float val) {
    _view->setAlpha(val);
}




LayoutAnimation::LayoutAnimation(View* view, InterpolateFunc interpolator) : Animation(view) {
    _interpolator = interpolator;
}

LayoutAnimation* LayoutAnimation::startHorizontal(View* view, ALIGNSPEC newAlignspec, int duration, InterpolateFunc interpolator) {
    LayoutAnimation* anim = new LayoutAnimation(view, interpolator);
    anim->_affectsAlignspecHorz = true;
    anim->_newAlignspecHorz = newAlignspec;
    anim->start(duration, 0);
    return anim;
}


LayoutAnimation* LayoutAnimation::startVertical(View* view, ALIGNSPEC newAlignspec, int duration, InterpolateFunc interpolator) {
    LayoutAnimation* anim = new LayoutAnimation(view, interpolator);
    anim->_affectsAlignspecVert = true;
    anim->_newAlignspecVert = newAlignspec;
    anim->start(duration, 0);
    return anim;
}

LayoutAnimation* LayoutAnimation::startPositional(View* view, ALIGNSPEC newAlignspecHorz, ALIGNSPEC newAlignspecVert, int duration, InterpolateFunc interpolator) {
    LayoutAnimation* anim = new LayoutAnimation(view, interpolator);
    anim->_affectsAlignspecHorz = true;
    anim->_affectsAlignspecVert = true;
    anim->_newAlignspecHorz = newAlignspecHorz;
    anim->_newAlignspecVert = newAlignspecVert;
    anim->start(duration, 0);
    return anim;
}

void LayoutAnimation::apply(float val) {
    if (val >= 1.0f) {
        if (_affectsAlignspecHorz) {
            _view->_alignspecHorz = _newAlignspecHorz;
        }
        if (_affectsAlignspecVert) {
            _view->_alignspecVert = _newAlignspecVert;
        }
        _view->setNeedsLayout();
        return;
    }
    if (!_valid) {
        if (_affectsAlignspecHorz) {
            _originHorzStart = _view->getAlignspecVal(_view->_alignspecHorz, false);
            _originHorzEnd = _view->getAlignspecVal(_newAlignspecHorz, false);
        }
        if (_affectsAlignspecVert) {
            _originVertStart = _view->getAlignspecVal(_view->_alignspecVert, true);
            _originVertEnd = _view->getAlignspecVal(_newAlignspecVert, true);
        }
        _valid = true;
    }
    if (_affectsAlignspecHorz || _affectsAlignspecVert) {
        POINT origin;
        if (_affectsAlignspecHorz) {
            origin.x = _originHorzStart + (_originHorzEnd - _originHorzStart) * val;
        } else {
            origin.x = _view->_rect.origin.x;
        }
        if (_affectsAlignspecVert) {
            origin.y = _originVertStart + (_originVertEnd - _originVertStart) * val;
        } else {
            origin.y = _view->_rect.origin.y;
        }
        _view->setRectOrigin(origin);
    }
}



//
// Tweening functions. The 4 parameters are :
//
//  t - time, ranges from 0 to d
//  b - begin, i.e. the initial value for the quantity being changed over time
//  c - change, the amount b will be changed by at the end
//  d - duration, of the transition, normally in milliseconds. 
//
// All were adapted from http://jstween.sourceforge.net/Tween.js 
//
float Animation::linear(float t, float b, float c, float d) {
    return b + c*(t/d);
}
float Animation::regularEaseInOut(float t, float b, float c, float d) {
	if ((t/=d/2) < 1) return c/2*t*t + b;
    t--;
	return -c/2 * ((t)*(t-2) - 1) + b;
}
float Animation::strongEaseInOut(float t, float b, float c, float d) {
    t/=d/2; 
    if (t < 1) return c/2*t*t*t*t*t + b;
    t-=2;
    return c/2*(t*t*t*t*t + 2) + b;
}
float Animation::regularEaseIn(float t, float b, float c, float d) {
    t/=d;
    return c*t*t + b;
}
float Animation::easeOut(float t, float b, float c, float d) {
    t/=d;
    return -c * t *(t-2) + b;
}
float Animation::strongEaseIn(float t, float b, float c, float d) {
    t/=d;
    return c*t*t*t*t*t + b;
}
float Animation::strongEaseOut(float t, float b, float c, float d) {
    t=t/d-1;
    return c*(t*t*t*t*t + 1) + b;
}
float Animation::nowhere(float t, float b, float c, float d) {
    t/=d/2; // t<0<=2
    if (t >= 1) t=1-(t-1);
    return c*t*t + b;
}
float Animation::bounceEaseOut(float t, float b, float c, float d) {
    if ((t/=d) < (1.0f/2.75f)) {
        return c*(7.5625f*t*t) + b;
    } else if (t < (2.0f/2.75f)) {
        t-=(1.5f/2.75f);
        return c*(7.5625f*(t)*t + .75f) + b;
    } else if (t < (2.5f/2.75f)) {
        t-=(2.25f/2.75f);
        return c*(7.5625f*(t)*t + .9375f) + b;
    } else {
        t-=(2.625/2.75);
        return c*(7.5625f*(t)*t + .984375f) + b;
    }
}
float Animation::bounceEaseIn (float t, float b, float c, float d) {
    return c - bounceEaseOut (d-t, 0, c, d) + b;
}
float Animation::bounceEaseInOut(float t, float b, float c, float d) {
    if (t < d/2) return bounceEaseIn (t*2, 0, c, d) * .5f + b;
    else return bounceEaseOut (t*2-d, 0, c, d) * .5f + c*.5f + b;
}
float Animation::elasticEaseIn (float t, float b, float c, float d) {
    if (t == 0)
        return b;
    if ((t /= d) == 1)
        return b + c;
    float p = d * .3f;
    float a = c;
    float s = p / 4;
    t -= 1;
    return -(a * (float) powf(2, 10 * t) * (float) sinf((t * d - s) * (2 * (float)M_PI) / p)) + b;
}
static int OVERSHOOT = -15; // increase magnitude => reduce overshoot
float Animation::elasticEaseOut(float t, float b, float c, float d) {
    if (t == 0)
        return b;
    if ((t /= d) == 1)
        return b + c;
    float p = d * .5f;
    float a = c;
    float s = p / 4;
    return (a * (float) powf(2, OVERSHOOT * t) * (float) sinf((t * d - s) * (2 * (float)M_PI) / p) + c + b);
}



