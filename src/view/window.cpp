//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>


Window::Window() : _rootViewController(NULL) {
    _renderer = Renderer::create();
    _surface = _renderer->createSurface(false);
    _window = this;
    _effectiveAlpha = 1;
}

void Window::show() {
    auto winstyle = getStyle("Window");
    _backgroundColor = winstyle->colorVal("background-color");
    _animationDuration = winstyle->intVal("animation-duration");
    _fadeInThreshold = winstyle->intVal("image-fade-in.threshold");
    _fadeInDuration = winstyle->intVal("image-fade-in.duration");
    auto scrollstyle = getStyle("Window.scrollbars");
    _scrollbarColor = scrollstyle->colorVal("color");
    _scrollbarCornerRadius = scrollstyle->floatVal("corner-radius");
    _scrollbarWidth = scrollstyle->floatVal("width");
    _scrollbarMinLength = scrollstyle->floatVal("min-length");
    _scrollbarInset = scrollstyle->floatVal("inset");
    _scrollbarFadeInDelay = scrollstyle->intVal("fade-in-delay");
    _scrollbarFadeDuration = scrollstyle->intVal("fade-duration");
    _scrollbarFadeOutDelay = scrollstyle->intVal("fade-out-delay");
}


void Window::setRootViewController(ViewController* viewController) {
    if (viewController == _rootViewController) {
        return;
    }
    if (_rootViewController) {
        detachViewController(_rootViewController);
    }
    _rootViewController = viewController;
    attachViewController(_rootViewController);
}


void Window::attachViewController(ViewController* viewController) {
    assert(!viewController->getView()->getWindow());
    _viewControllers.push_back(viewController);
    addSubview(viewController->getView());
    viewController->applySafeInsets(getSafeInsets());
    viewController->onWindowAttached();
}
void Window::detachViewController(ViewController* viewController) {
    assert(viewController->getView()->getWindow() == this);
    removeSubview(viewController->getView());
    for (int i=0 ; i<_viewControllers.size() ; i++) {
        if (_viewControllers[i] == viewController) {
            _viewControllers.erase(_viewControllers.begin()+i);
            break;
        }
    }
    viewController->onWindowDetached();
}

void Window::layout(RECT constraint) {
    _rect = constraint;
    layoutSubviews(_rect);
}
void Window::resizeSurface(int width, int height) {
    if (_surface->_size.width==width && _surface->_size.height==height) {
        return;
    }
	//log_info("Window::resize %d %d", width, height);
    _rect = RECT(0,0,width,height);
    _surface->setSize({(float)width, (float)height});
    applySafeInsetsChange(false);
    setNeedsLayout();
}
void Window::destroySurface() {
    _renderer->reset();
}

Window::MotionTracker::MotionTracker(int source) {
    this->source = source;
    isDragging = false;
    touchedView = NULL;
    timeOfDownEvent = 0;
    pastIndex = pastCount = 0;
    numClicks = 0;
    _didSendLongpressEvent = false;
}

void Window::MotionTracker::dispatchInputEvent(INPUTEVENT& event, ViewController* topVC) {
    if (touchedView && !touchedView->_window) {
        touchedView = nullptr; // avoid sending events to detached views
    }
    if (event.type == INPUT_EVENT_DOWN) {
        isDragging = false;
        if (multiclickTimer) {
            multiclickTimer->stop();
            multiclickTimer = nullptr;
            numClicks++;
        }
        pastIndex = pastCount = 0;
        ptDown = event.ptSurface;
        timeOfDownEvent = event.time;
        touchedView = topVC->getView()->dispatchInputEvent(&event);
        if (!touchedView) {
            touchedView = topVC->getView();
        }
        //log_info("Window DOWN! touchedView=%X", touchedView._obj);
        _didSendLongpressEvent = false;
        if (event.deviceType != INPUTEVENT::ScrollWheel) {
            _longpressTimer = Timer::start([=] {
                if (touchedView && touchedView->_window) {
                    INPUTEVENT lpEv = event;
                    lpEv.type = INPUT_EVENT_LONG_PRESS;
                    lpEv.ptSurface += touchedView->_surfaceOrigin;
                    touchedView->dispatchInputEvent(&lpEv);
                    _didSendLongpressEvent = true;
                }
                _longpressTimer = nullptr;
            }, LONG_PRESS_THRESHOLD, false);
        }
        pastTime[pastIndex] = event.time;
        pastPts[pastIndex] = event.ptSurface;
        pastIndex = (pastIndex + 1) % NUM_PAST;
        pastCount++;
        ptPrev = event.ptSurface;
    }

    if (event.type == INPUT_EVENT_MOVE) {
        
        // Filter out spurious move events (seen on iOS 10)
        if (event.deviceType != INPUTEVENT::ScrollWheel) {
            int prevIndex = pastIndex - 1;
            if (prevIndex < 0) prevIndex += NUM_PAST;
            if (pastPts[prevIndex].equals(event.ptSurface)) {
                pastTime[prevIndex] = event.time;
                return;
            }
        }
        
        // Store the event in the history (used for fling velocity calculations)
        pastTime[pastIndex] = event.time;
        pastPts[pastIndex] = event.ptSurface;
        pastIndex = (pastIndex + 1) % NUM_PAST;
        pastCount++;
        event.delta = event.ptSurface - ptPrev;
        ptPrev = event.ptSurface;
        
        // If not dragging, test to see if a drag might have started
        if (timeOfDownEvent && !isDragging) {
            float dx,dy;
            dx = event.ptSurface.x - ptDown.x;
            dy = event.ptSurface.y - ptDown.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (app->idp(dist) >= TOUCH_SLOP) {
                // log_info("Drag started");
                _dragIsVertical = fabsf(dy) >= fabsf(dx);
                
                // Dispatch a tap-cancel event to the current touch target
                // (e.g. button that changes colour when pressed)
                if (touchedView) {
                    // log_info("Sending INPUT_EVENT_TAP_CANCEL to %X", touchedView._obj);
                    INPUTEVENT cancelEvent = event;
                    cancelEvent.type = INPUT_EVENT_TAP_CANCEL;
                    touchedView->dispatchInputEvent(&cancelEvent);
                }
                
                // Cancel the longpress timer
                if (_longpressTimer) {
                    _longpressTimer->stop();
                    _longpressTimer = nullptr;
                }
                
                // Dispatch a drag-start event and track the view that handles it
                // log_info("Sending INPUT_EVENT_DRAG_START to top");
                INPUTEVENT dragStartEvent = event;
                dragStartEvent.delta = event.ptSurface - ptDown;
                dragStartEvent.type = INPUT_EVENT_DRAG_START;
                touchedView = topVC->getView()->dispatchInputEvent(&dragStartEvent);
 
                isDragging = true;
            }
        }
        
        // Hover events
        View* hoverView = topVC->getView()->hitTest(&event);
        if (hoverView != this->hoverView) {
            INPUTEVENT hoverEvent = event;
            if (this->hoverView) {
                hoverEvent.type = INPUT_EVENT_MOUSE_EXIT;
                this->hoverView->dispatchInputEvent(&hoverEvent);
            }
            this->hoverView = hoverView;
            if (this->hoverView) {
                hoverEvent.type = INPUT_EVENT_MOUSE_ENTER;
                this->hoverView->dispatchInputEvent(&hoverEvent);
            }
        }
        
        // If dragging, dispatch a drag-move event
        if (isDragging && touchedView) {
            INPUTEVENT dragEvent = event;
            dragEvent.type = INPUT_EVENT_DRAG_MOVE;
            if (touchedView->_directionalLockEnabled) {
                if (_dragIsVertical) {
                    dragEvent.delta.x = 0;
                } else {
                    dragEvent.delta.y = 0;
                }
            }
            //log_info("Window sending INPUT_EVENT_DRAG_MOVE to %X", touchedView._obj);
            touchedView->dispatchInputEvent(&dragEvent);
        }
        
    } else if (event.type == INPUT_EVENT_UP) {
        if (_longpressTimer) {
            _longpressTimer->stop();
            _longpressTimer = nullptr;
        }
        //log_info("Window sending INPUT_EVENT_UP to %X", touchedView._obj);
        if (touchedView) {
            touchedView->dispatchInputEvent(&event);
        }
        if (!_didSendLongpressEvent) {
            if (!isDragging && (event.deviceType!=INPUTEVENT::ScrollWheel)) {
                // TAP!
                if (touchedView) {
                    INPUTEVENT tapEvent = event;
                    tapEvent.type = INPUT_EVENT_TAP;
                    touchedView->dispatchInputEvent(&tapEvent);
                    // log_info("tap %d", numClicks);
                }
                auto touchedViewCopy = touchedView;
                multiclickTimer = Timer::start([=] {
                    if (touchedViewCopy && touchedViewCopy->_window) {
                        INPUTEVENT tapEvent = event;
                        tapEvent.type = INPUT_EVENT_TAP_CONFIRMED;
                        touchedViewCopy->dispatchInputEvent(&tapEvent);
                    }
                    multiclickTimer = nullptr;
                    // log_info("tap confirmed at %d", numClicks);
                    numClicks = 0;
                }, MULTI_CLICK_THRESHOLD, false);
            } else {
                // FLING!

                // Work out the drag velocity by getting the distance travelled from the oldest historical point
                // and extrapolating an average distance per second.
                int numPoints = MIN(pastCount, NUM_PAST);
                float dx=0.f,dy=0.f;
                int dTime = 1;
                for (int i=1 ; i<=numPoints ; i++) {
                    int index = pastIndex - i;
                    if (index < 0) index += NUM_PAST;
                    POINT ptFrom = pastPts[index];
                    TIMESTAMP timeFrom = pastTime[index];
                    if (event.time - timeFrom >= 333) { // ignore historical points that are too old
                        continue;
                    }
                    dTime = (int) (event.time - timeFrom);
                    if (dTime <= 0) continue;
                    dx = event.ptSurface.x - ptFrom.x;
                    dy = event.ptSurface.y - ptFrom.y;
                }
                float thisVeloX = dx * 1000.0f / dTime;
                float thisVeloY = dy * 1000.0f / dTime;
                POINT velocity = {0, 0};
                velocity.x = (velocity.x == 0) ? thisVeloX : ((velocity.x + thisVeloX) / 2);
                velocity.y = (velocity.y == 0) ? thisVeloY : ((velocity.y + thisVeloY) / 2);
                if (touchedView) {
                    INPUTEVENT flingEvent = event;
                    flingEvent.type = INPUT_EVENT_FLING;
                    flingEvent.velocity = velocity;
                    if (touchedView->_directionalLockEnabled) {
                        if (_dragIsVertical) {
                            flingEvent.delta.x = 0;
                            flingEvent.velocity.x = 0;
                        } else {
                            flingEvent.delta.y = 0;
                            flingEvent.velocity.y = 0;
                        }
                    }
                    touchedView->dispatchInputEvent(&flingEvent);
                }
            }
        }
        touchedView = nullptr;
        timeOfDownEvent = 0;

    }
}

void Window::dispatchInputEvent(INPUTEVENT event) {

    //if (event->deviceType == INPUTEVENT::Mouse || event->deviceType == INPUTEVENT::Touch) {

        // Start or lookup motion tracker
        MotionTracker *tracker = nullptr;
        for (auto it=_motionTrackers.begin() ; it!=_motionTrackers.end() ; it++) {
            if ((*it)->source == event.deviceIndex) {
                tracker = *it;
                break;
            }
        }
        if (!tracker) {
            tracker = new MotionTracker(event.deviceIndex);
            //glInsertEventMarkerEXT(0, "com.apple.GPUTools.event.debug-frame");
            _motionTrackers.push_back(tracker);
        }

        // Let the tracker process the new input event
        tracker->dispatchInputEvent(event,  *_viewControllers.rbegin());

    //}
}

static int numFrames;
static time_t timeBase;

void incFrames() {
    time_t now = time(NULL);
    if (now == timeBase) {
        numFrames++;
    } else {
#ifdef EMSCRIPTEN
		EM_ASM_({
			updateFps($0);
		}, numFrames);
#endif
        printf("%d fps\n", numFrames);
        timeBase = now;
        numFrames = 1;
    }
}

void Window::draw() {
    _redrawNeeded = false;

    if (!_layoutValid) {
        _layoutValid = true;
        layout(_rect);
        ensureFocusedViewIsInSafeArea();
    }

    // Draw the window to it's surface
    _renderCounter++;
	sp<RenderTask> renderTask = _renderer->createRenderTask();
    if (_backgroundColor) {
        _surface->_clearColor = _backgroundColor;
        _surface->_clearNeeded = true;
    }
    renderTask->bindToNativeSurface(_surface);
    _surface->render(this, renderTask);
    renderTask->commit(nullptr);
	
	incFrames();

    // Tick animations
    TIMESTAMP now = app->currentMillis();
    auto it = _animations.begin();
    _animationsModified = false;
    while (it != _animations.end()) {
        sp<Animation> anim = *it++;
        if (!anim->_paused) {
            bool stillGoing = anim->tick(now);
            if (!stillGoing) {
                stopAnimation(anim);
            }
            if (_animationsModified) {
                _animationsModified = false;
                it = _animations.begin(); // bit crap
            }
        }
    }
    
    // If there are any animations running, request a redraw immediately
    if (_animations.size()) {
        requestRedraw();
    }
    
}

void Window::startAnimation(Animation* animation, int duration) {
    startAnimation(animation, duration, 0);
}
void Window::startAnimation(Animation* animation, int duration, int delay) {
    if (animation->_window) {
        return;
    }
    animation->_duration = duration;
    animation->_delay = delay;
    animation->_paused = false;
    animation->_timeStarted = app->currentMillis();
    animation->_window = this;
    animation->_windowAnimationsListIterator = _animations.insert(_animations.end(), animation);
    if (animation->_view) {
        animation->_view->_animationCount++;
    }
    
    // If animation has zero duration then it's effectively finished before it starts.
    // Applying the final value now rather than on next frame avoids unwanted in-between frames.
    if (delay<=0 && duration<=0) {
        animation->apply(1.0);
    } else {
        animation->apply(0.0);
    }
    requestRedraw();
}

void Window::stopAnimation(Animation* animation) {
    if (animation->_window) {
        assert(animation->_window == this);
        animation->retain();
        //animation->apply(1.0);
        _animations.erase(animation->_windowAnimationsListIterator);
        _animationsModified = true;
        animation->_window = NULL;
        animation->_timeStarted = 0;
        if (animation->_view) {
            animation->_view->_animationCount--;
        }
        animation->release();
    }
}

void Window::detachView(View* view) {
    if (_textInputReceiver && _textInputReceiver==view->getTextInputReceiver()) {
        setFocusedView(NULL);
        _textInputReceiver = NULL;
        _keyboardHandler = NULL;
    }
    if (view->_animationCount > 0) {
        for (auto it=_animations.begin() ; it!=_animations.end(); ) {
            auto anim = *it++;
            if (anim->_view == view) {
                stopAnimation(anim);
            }
        }
        assert(view->_animationCount == 0);
    }
}

void Window::requestRedraw() {
	if (_redrawNeeded) {
		return;
	}
	_redrawNeeded = true;
    requestRedrawNative();
}

void Window::requestRedrawNative() {
    // no-op
}

POINT Window::offsetToView(View* view) {
	POINT pt = {0,0};
	ViewController* rootVC = _rootViewController;
	while (view && view != rootVC->getView()) {
		pt.x += view->_rect.origin.x;
		pt.y += view->_rect.origin.y;
		view = view->_parent;
	}
	return pt;
}

void Window::keyboardShow(bool show) {
    // no-op
}
void Window::keyboardNotifyTextChanged() {
    // no-op
}
void Window::keyboardNotifyTextSelectionChanged() {
    // no-op
}

bool Window::setFocusedView(View* view) {
    bool settingToExisting = (_focusedView == view);
    if (_focusedView && !settingToExisting) {
        _focusedView->setFocused(false);
    }
    _focusedView = view;
    if (view) {
        if (!settingToExisting) {
            view->setFocused(true);
            _keyboardHandler = view->getKeyboardInputHandler();
            auto newTextInputReceiver = view->getTextInputReceiver();
            if (newTextInputReceiver != _textInputReceiver) {
                _textInputReceiver = newTextInputReceiver;
                keyboardNotifyTextChanged();
            }
        }
        ensureFocusedViewIsInSafeArea();
        keyboardShow(_textInputReceiver != NULL);
    } else {
        keyboardShow(false);
        _keyboardHandler = NULL;
    }
    return true;
}









void Window::setSafeInsets(const EDGEINSETS& insets) {
    if (_systemSafeInsets != insets) {
        _systemSafeInsets = insets;
        applySafeInsetsChange(true);
    }
}

void Window::setSoftKeyboardInsets(const EDGEINSETS& insets) {
    if (_softKeyboardInsets != insets) {
        _softKeyboardInsets = insets;
        applySafeInsetsChange(true);
    }
}

EDGEINSETS Window::getSafeInsets() const {
    EDGEINSETS insets;
    insets.left = MAX(_systemSafeInsets.left, _softKeyboardInsets.left);
    insets.top = MAX(_systemSafeInsets.top, _softKeyboardInsets.top);
    insets.right = MAX(_systemSafeInsets.right, _softKeyboardInsets.right);
    insets.bottom = MAX(_systemSafeInsets.bottom, _softKeyboardInsets.bottom);
    return insets;
}

void Window::applySafeInsetsChange(bool updateFocusedView) {
    
    EDGEINSETS insets = getSafeInsets();

    // Window decor
    updateDecorOp(true, insets.bottom);
    updateDecorOp(false, insets.top);
    
    for (auto vc : _viewControllers) {
        vc->applySafeInsets(insets);
    }
    
    if (updateFocusedView) {
        ensureFocusedViewIsInSafeArea();
        requestRedraw();
    }
}


void Window::updateDecorOp(bool bottom, float height) {
    RectRenderOp*& op = bottom?_renderOpDecorBottom:_renderOpDecorTop;
    COLOR color = getStyleColor(bottom ? "Window.safeInsetBackgrounds.bottom"_S : "Window.safeInsetBackgrounds.top"_S);
    if (color!=0 && height>0) {
        if (!op) {
            op = new RectRenderOp();
            op->setFillColor(color);
            addDecorOp(op);
        }
        RECT rect = {0,bottom?(_rect.size.height-height):0,_rect.size.width, height};
        op->setRect(rect);
    } else  {
        if (op) {
            removeDecorOp(op);
            op = NULL;
        }
    }
}



// Permissions. By default there is no runtime permissions system.
// (NB: Of course iOS and Android implement these differently...)
bool Window::hasPermission(Permission permission) {
    return true;
}
void Window::runWithPermission(Permission permission, std::function<void(bool)> callback) {
    runWithPermissions({permission}, [=](vector<bool> callback2) {
        callback(callback2[0]);
    });
}
void Window::runWithPermissions(vector<Permission> permissions, std::function<void(vector<bool>)> callback) {
    vector<bool> results;
    for (auto p : permissions) {
        results.push_back(true);
    }
    callback(results);
}


void Window::presentModalViewController(ViewController *viewController) {
    
    // Fade in a 'scrim' view that also prevents the UI underneath being touchable
    View* scrimView = new View();
    scrimView->setLayoutSize(MEASURESPEC::Fill(), MEASURESPEC::Fill());
    addSubview(scrimView);
    setNeedsLayout();
    COLOR scrimColor = getStyleColor("Window.scrim");
    Animation::start(scrimView, 333, [=](float val) {
        scrimView->setBackgroundColor(COLOR::interpolate(0, scrimColor, val));
    });

    // Attach the new VC and animate it in from below
    attachViewController(viewController);
    //viewController->getView()->applyTranslate(0, _rect.size.height); // i.e. start off below screen
    viewController->getView()->animateInFromBottom(333);
}

void Window::dismissModalViewController(ViewController* viewController, std::function<void()> onComplete) {
    auto currentTop = _viewControllers.rbegin();
    assert(*currentTop == viewController);
    View* view = viewController->getView();
    View* scrimView = getSubview(indexOfSubview(view)-1);
    
    // Animate out
    COLOR scrimColor = getStyleColor("Window.scrim");
    Animation::start(view, 333, [=](float val) {
        scrimView->setBackgroundColor(COLOR::interpolate(scrimColor, 0, val));
        if (val >= 1.0f) {
            if (onComplete) {
                onComplete();
            }
            detachViewController(viewController);
            scrimView->removeFromParent();
            requestRedraw();
        }
    });
    view->animateOutToBottom(333);
    
}

void Window::ensureFocusedViewIsInSafeArea() {
    if (_focusedView != nullptr) {
        
        // Get the focused view's window rect
        RECT rect = _focusedView->getOwnRect();
        rect.origin = _focusedView->mapPointToWindow(rect.origin);
        
        // Calculate the extent it is below the safe area
        float dx=0,dy=0;
        RECT safeArea = _rect;
        EDGEINSETS safeInsets = getSafeInsets();
        safeInsets.applyToRect(safeArea);
        float d = rect.bottom() - safeArea.bottom();
        if (d > 0) {
            dy = -d;
        }
        d = rect.top() - safeArea.top();
        if (d < 0) {
            dy = -d;
        }
        d = rect.right() - safeArea.right();
        if (d > 0) {
            dx = -d;
        }
        d = rect.left() - safeArea.left();
        if (d < 0) {
            dx = -d;
        }
        if (dx!=0 || dy!=0) {
            _rootViewController->requestScroll(-dx, -dy);
        }
    }
}
