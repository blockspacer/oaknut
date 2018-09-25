//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//
#if PLATFORM_IOS

#import "AppDelegate.h"
#import "NativeView.h"
#if OAKNUT_WANT_CAMERA || OAKNUT_WANT_AUDIO_INPUT
#import <AVFoundation/AVFoundation.h>
#endif

@interface NativeViewController : UIViewController
@end
@implementation NativeViewController
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self=[super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
        //[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onKeyboardFrameChanged:) name:UIKeyboardWillChangeFrameNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onKeyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onKeyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];

    }
    return self;
}
- (UIStatusBarStyle) preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}
- (void)onKeyboardWillShow:(NSNotification*)notification {
    CGRect keyboardFrame = ((NSValue*)notification.userInfo[UIKeyboardFrameEndUserInfoKey]).CGRectValue;
    RECT rect;
    float scale = [UIScreen mainScreen].scale;
    rect.origin.x = keyboardFrame.origin.x * scale;
    rect.origin.y = keyboardFrame.origin.y * scale;
    rect.size.width = keyboardFrame.size.width * scale;
    rect.size.height = keyboardFrame.size.height * scale;
    app._window->setSoftKeyboardRect(rect);
}
- (void)onKeyboardWillHide:(NSNotification*)notification {
    app._window->setSoftKeyboardRect(RECT_Zero);
}

@end

static UIWindow* _nativeWindow;
static bool _nativeWindowOwnedByOaknut;

void ensureNativeWindowExists() {
    if (!_nativeWindow) {
        _nativeWindow = [UIApplication sharedApplication].keyWindow;
        if (!_nativeWindow) {
            _nativeWindowOwnedByOaknut = true;
            _nativeWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
            _nativeWindow.backgroundColor = [UIColor whiteColor];
            [_nativeWindow makeKeyAndVisible];
        }

    }
}

class WindowIOS : public Window {
public:
    
    WindowIOS() {
        ensureNativeWindowExists();
        _nativeView = [[NativeView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
        _nativeView->_window = this;
        _viewController = [[NativeViewController alloc] initWithNibName:nil bundle:nil];
        _viewController.view = _nativeView;
        _scale = [UIScreen mainScreen].scale;
        _safeAreaInsets.top = [UIApplication sharedApplication].statusBarFrame.size.height * _scale;

    }
    
    void show() override {
        if (_nativeWindowOwnedByOaknut) {
            _nativeWindow.rootViewController = _viewController;
        } else {
            [_nativeWindow.rootViewController presentViewController:_viewController animated:YES completion:nil];
        }
    }
    
    void requestRedrawNative() override {
        if (!_nativeView->_renderNeeded) {
            _nativeView->_renderNeeded = YES;
            [_nativeView setNeedsDisplay];
        }
    }
    
    void keyboardShow(bool show) override {
        if (show) {
            [_nativeView becomeFirstResponder];
        } else {
            [_nativeView resignFirstResponder];
        }
    }
    void keyboardNotifyTextChanged() override {
        if (_nativeView->_textInputDelegate) {
            [_nativeView->_textInputDelegate selectionWillChange: _nativeView];
            [_nativeView->_textInputDelegate textWillChange: _nativeView];
            [_nativeView->_textInputDelegate selectionDidChange: _nativeView];
            [_nativeView->_textInputDelegate textDidChange: _nativeView];
        }
    }
    
    /**
     Permissions
     */
    virtual bool hasPermission(Permission permission) override {
        if (permission == PermissionCamera) {
#ifdef OAKNUT_WANT_CAMERA
            AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
            return status == AVAuthorizationStatusAuthorized;
#else
            return false;
#endif
        }
        if (permission == PermissionMic) {
#ifdef OAKNUT_WANT_AUDIO_INPUT
            AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
            return status == AVAuthorizationStatusAuthorized;
#else
            return false;
#endif
        }
        assert(0); // unknown permission
        return false;
    }
    
    class PermissionRequest {
    public:
        vector<Permission> _permissions;
        std::function<void(vector<bool>)> _callback;
        int _index;
        vector<bool> _results;
        PermissionRequest(const vector<Permission>& permissions, const std::function<void(vector<bool>)>& callback) {
            _permissions = permissions;
            _callback = callback;
            _index = 0;
        }
        void addResult(BOOL result) {
            _results.push_back(result);
            if (_results.size() >= _permissions.size()) {
                _callback(_results);
                delete this;
            } else {
                drain();
            }
        }
        void drain() {
            Permission permission = _permissions[_index++];
            if (permission == PermissionCamera) {
#ifdef OAKNUT_WANT_CAMERA
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        addResult(granted);
                    });
                }];
#else
                addResult(false);
#endif
            }
            else if (permission == PermissionMic) {
#ifdef OAKNUT_WANT_AUDIO_INPUT
                [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        addResult(granted);
                    });
                }];
#else
                addResult(false);
#endif
            }
            else {
                assert(0); // unknown permission
            }
        }
    };
    
    virtual void runWithPermissions(vector<Permission> permissions, std::function<void(vector<bool>)> callback) override {
        PermissionRequest* req = new PermissionRequest(permissions, callback);
        req->drain();
    }

    UIViewController* _viewController;
    NativeView* _nativeView;
    
};


Window* Window::create() {
    return new WindowIOS();
}


@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    app._window = Window::create();
    app.loadStyleAsset("styles.res");
    app.main();
    app._window->show();
    return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
}

- (void)applicationWillTerminate:(UIApplication *)application {
}

@end

#endif
