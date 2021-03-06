//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>

static const float SCROLL_FRICTION = 0.015f;

static const float DECELERATION = 9.80665f         // Earth's gravity (m/s^2)
                                * 39.37f           // inch/meter
                                * 160              // pixels per inch
                                * SCROLL_FRICTION;

static int inline signum(float x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}



Fling::Fling(float start, float velocity, float min, float max) {
    _coeff = signum(velocity);
    velocity = hypotf(velocity, velocity);
    _velocity = velocity;
    _duration = (int) (1000 * velocity / DECELERATION);
    _startTime = app->currentMillis();
    _start = start;
    _min = min;
    _max = max;
    float totalDistance = ((velocity * velocity) / (2 * DECELERATION));
    _final = start + totalDistance * _coeff;
    _final = fminf(_final, max);
    _final = fmaxf(_final, min);
}

void Fling::update(float timePassedSeconds) {
    float distance = (_velocity * timePassedSeconds)
                   - (DECELERATION * timePassedSeconds * timePassedSeconds / 2.0f);
    
    _curr = _start + distance * _coeff;
    _curr = fminf(_curr, _max);
    _curr = fmaxf(_curr, _min);
}

class Bounce : public Fling {
public:
    Bounce(float start, float end) : Fling(0,0,0,0) {
        _startTime = app->currentMillis();
        _duration = 500;
        _start = start;
        _final = end;
    }
    void update(float timePassedSeconds) {
        _curr = Animation::strongEaseOut(timePassedSeconds, _start, _final-_start, _duration/1000.f);
    }
};


/*
 Called during layout and when contentOffset changes.
 */
bool ScrollInfo::canScroll(View* view, bool isVertical) {
    if (_disabled) {
        return false;
    }
    float visibleSize = isVertical ? view->_rect.size.height : view->_rect.size.width;
    float scrollableSize = isVertical
                        ? (view->_padding.top + view->_contentSize.height + view->_padding.bottom + (view->_scrollInsets.top+view->_scrollInsets.bottom))
                        :  view->_contentSize.width + (view->_scrollInsets.left+view->_scrollInsets.right);
    return scrollableSize>0 && visibleSize>0 && (scrollableSize - visibleSize)>=1; // must be at least 1 pixel scrollable
}

float ScrollInfo::maxScroll(View* view, bool isVertical) {
    float padPre = isVertical ? view->_padding.top : view->_padding.left;
    float padPost = isVertical ? view->_padding.bottom : view->_padding.right;
    float scrollInset = isVertical ? (view->_scrollInsets.top+view->_scrollInsets.bottom) : (view->_scrollInsets.left+view->_scrollInsets.right);
    float content = isVertical ? view->_contentSize.height : view->_contentSize.width;
    float visibleSize = isVertical ? view->_rect.size.height : view->_rect.size.width;
    float scrollableSize = padPre + content + scrollInset + padPost;
    float m = (scrollableSize) - visibleSize;
    //log_dbg("padPre:%f padPost:%f inset:%f content:%f vis:%f  max:%f", padPre, padPost, scrollInset, content, visibleSize, m);
    return (m>0) ? m : 0;
}
void ScrollInfo::updateVisibility(View* view, bool isVertical) {
    if (!view || !view->_window) {
        return;
    }
    if (canScroll(view, isVertical)) {
        if (!_renderOp) {
            _renderOp = new RectRenderOp();
            _renderOp->setFillColor(view->_window->_scrollbarColor);
            _renderOp->setCornerRadius(view->_window->_scrollbarCornerRadius);
            _renderOp->setAlpha(0);
            view->addDecorOp(_renderOp);
        }
        
        // Scroll bar should be visible. Fade it in.
        if (_showState==Invisible || _showState==FadingOut) {
            _showState = FadingIn;
            if (_fadeAnim) {
                _fadeAnim->stop();
            }
            _fadeAnim = Animation::start(view, view->_window->_scrollbarFadeDuration, [=](float val) {
                if (_renderOp) {
                    _renderOp->setAlpha(val);
                    view->invalidateRect(_renderOp->_rect);
                }
            });
        }

        // Schedule fade out
        if (_fadeOutTimer) {
            _fadeOutTimer->stop();
        }
        _fadeOutTimer = Timer::start([=]() {
            if (_fadeAnim) {
                _fadeAnim->stop();
            }
            _fadeAnim = Animation::start(view, view->_window->_scrollbarFadeDuration, [=](float val) {
                if (_renderOp) {
                    _renderOp->setAlpha(1.0f-val);
                    view->invalidateRect(_renderOp->_rect);
                }
            });
            _fadeAnim->onFinished = [=](class Animation* anim) {
                _showState = Invisible;
            };
            _showState = FadingOut;
        }, view->_window->_scrollbarFadeDuration + view->_window->_scrollbarFadeOutDelay, false);
        

        float content = isVertical ? view->_contentSize.height : view->_contentSize.width;
        float contentOffset = isVertical ? view->_contentOffset.y : view->_contentOffset.x;
        float contentOffsetPerp = isVertical ? view->_contentOffset.x : view->_contentOffset.y;
        float topLeftInset = isVertical ? view->_scrollInsets.top : view->_scrollInsets.left;
        float botRightInset = isVertical ? view->_scrollInsets.bottom : view->_scrollInsets.right;
        float insets = topLeftInset + botRightInset;
        float visibleContent = content - insets;
        float visibleSize = isVertical ? view->_rect.size.height : view->_rect.size.width;
        float visibleSizeOther = isVertical ? view->_rect.size.width : view->_rect.size.height;
        float scale = (visibleSize-insets) / visibleContent;
        RECT rect = view->getOwnRect();
        float& origin = isVertical ? rect.origin.y : rect.origin.x;
        float& originPerp = isVertical ? rect.origin.x : rect.origin.y;
        float& barLength = isVertical ? rect.size.height : rect.size.width;
        float& barThickness = isVertical ? rect.size.width : rect.size.height;
        origin = topLeftInset + contentOffset * scale
                              + contentOffset; // Add the unscaled contentOffset cos we have to correct for
                                               // it now being part of MVP translation!
        originPerp = contentOffsetPerp + visibleSizeOther - (view->_window->_scrollbarWidth+view->_window->_scrollbarInset);
        barLength = (visibleSize-insets) * scale;
        barLength = fmaxf(barLength, view->_window->_scrollbarMinLength);
        barThickness = view->_window->_scrollbarWidth;
        _renderOp->setRect(rect);
    } else {
        if (_renderOp) {
            view->removeDecorOp(_renderOp);
            _renderOp = NULL;
        }
        detach();
    }
}


void ScrollInfo::detach() {
    if (_fadeAnim) {
        _fadeAnim = NULL;
    }
    if (_fadeOutTimer) {
        _fadeOutTimer->stop();
        _fadeOutTimer = NULL;
    }
}



bool ScrollInfo::handleEvent(View* view, bool isVertical, INPUTEVENT* event) {
    if (_disabled) {
        return false;
    }
    bool rv = false;
    float offset = isVertical ? view->_contentOffset.y : view->_contentOffset.x;
    float contentSize = isVertical ? (view->_padding.top + view->_contentSize.height + view->_padding.bottom)
                                   : (view->_padding.left + view->_contentSize.width + view->_padding.right);
    float scrollableSize = contentSize + (isVertical
                            ?  (view->_scrollInsets.top+view->_scrollInsets.bottom)
                            :  (view->_scrollInsets.left+view->_scrollInsets.right));
    float viewSize = isVertical ? view->_rect.size.height : view->_rect.size.width;
    float maxScroll = fmax(0, scrollableSize - viewSize);

    float d = 0;
    
    if (event->type == INPUT_EVENT_DOWN) {
        flingCancel();
        rv = (scrollableSize - viewSize) >= 1;
    }
    if (event->type == INPUT_EVENT_DRAG_MOVE) {
        if (!_isDragging) {
            flingCancel();
            _isDragging = true;
            _offsetStart = offset;
            _dragTotal = 0;
        }
        d = isVertical ? event->delta.y : event->delta.x;
    }

    if (d != 0.0) {
        _dragTotal += d;
        bool canScroll = this->canScroll(view, isVertical);
        if (canScroll) {
            // Calculate where dragged offset WOULD be, if there was no overscroll handling
            POINT newContentOffset = view->_contentOffset;
            float& nco = isVertical ? newContentOffset.y : newContentOffset.x;
            nco = _offsetStart - _dragTotal;
            // Overscroll handling
            if (nco < 0) {
                nco /= fmax(1.0f, log10(_dragTotal));
            } else if (nco > maxScroll) {
                float overshoot = nco - maxScroll;
                nco = maxScroll + overshoot / fmax(1.0f, log10(overshoot));
            }
            offset = nco;
            view->setContentOffset(newContentOffset);
            rv = true;
        }
    }
    
    bool overscroll = false;
    if (offset < 0) {
        overscroll = true;
    } else if (offset > maxScroll) {
        overscroll = true;
    }


    if (event->type == INPUT_EVENT_UP) {
        _isDragging = false;
        if (overscroll) {
            flingCancel();
            _fling = new Bounce(offset, (offset<=0)?0:contentSize-viewSize);
            view->setNeedsFullRedraw();
        }
    }

    if (event->type == INPUT_EVENT_FLING && !overscroll) {
        bool canScroll = this->canScroll(view, isVertical);
        if (canScroll) {
            flingCancel();
            float val = isVertical ? event->velocity.y : event->velocity.x;
            if (fabsf(val)!=0.0f) {
                _fling = new Fling(offset, -val, 0, contentSize-viewSize);
                view->setNeedsFullRedraw();
                rv = true;
            }
        }
    }
    return rv;
}



void ScrollInfo::flingCancel() {
    if (_fling) {
        delete _fling;
        _fling = NULL;
    }
}

float ScrollInfo::flingUpdate() {
    TIMESTAMP timePassed = (app->currentMillis() - _fling->_startTime);
    float retval;
    if (timePassed < _fling->_duration) {
        _fling->update(timePassed / 1000.0f);
        retval = _fling->_curr;
    } else {
        retval = _fling->_final;
    }
    if (retval == _fling->_final) {
        flingCancel();
    }
    return retval;
}




