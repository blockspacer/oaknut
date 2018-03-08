//
// Copyright © 2017 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//
#if PLATFORM_APPLE

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import "oaknut.h"


long oakCurrentMillis() {
    return CACurrentMediaTime()*1000;
}

string oakGetAppHomeDir() {
    NSURL* url = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
    return string([[url.absoluteString substringFromIndex:7] UTF8String]);
    
}



#if TARGET_OS_IOS
static EAGLSharegroup* s_mainEAGLSharegroup;
#else
static CGLContextObj s_mainContext;
#endif

void Task::ensureSharedGLContext() {
#if TARGET_OS_IOS
    if (![EAGLContext currentContext]) {
        EAGLContext* eaglcontext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup: s_mainEAGLSharegroup];
        assert(eaglcontext);
        [EAGLContext setCurrentContext:eaglcontext];
    }
#else
    if (!CGLGetCurrentContext()) {
        CGLContextObj ctx;
        CGLCreateContext(CGLGetPixelFormat(s_mainContext), s_mainContext, &ctx);
        CGLSetCurrentContext(ctx);
    }
#endif
}


class AppleTask : public Task {
public:
    NSBlockOperation* _op;

    AppleTask(TASKFUNC func) : Task(func) {
        _op = [NSBlockOperation blockOperationWithBlock:^() {
            _func();
        }];
    }
};

Task* Task::create(TASKFUNC func) {
    return new AppleTask(func);
}

class AppleTaskQueue : public TaskQueue {
public:

    NSOperationQueue* _queue;

    AppleTaskQueue(const string& name) : TaskQueue(name) {
#if TARGET_OS_IOS
        if (!s_mainEAGLSharegroup) {
            s_mainEAGLSharegroup = [EAGLContext currentContext].sharegroup;
        }
#else
        if (!s_mainContext) {
             s_mainContext = CGLGetCurrentContext();
        }
#endif
        _queue = [NSOperationQueue new];
        _queue.name = [[NSString alloc] initWithUTF8String:name.data()];
    }
    ~AppleTaskQueue() {
        _queue = nil;
    }

    void enqueueTask(Task* task) {
        [_queue addOperation:((AppleTask*)task)->_op];
    }
    bool cancelTask(Task* atask) {
        AppleTask* task = (AppleTask*)atask;
        if (!task->_op.isCancelled && !task->_op.isFinished && !task->_op.isExecuting) {
            [task->_op cancel];
            return task->_op.isCancelled;
        }
        return false;
    }

};

TaskQueue* TaskQueue::create(const string& name) {
    return new AppleTaskQueue(name);
}

void TaskQueue::postToMainThread(function<void(void)> func) {
    dispatch_async(dispatch_get_main_queue(), ^() {
        func();
    });
}




Data* oakLoadAsset(const char* assetPath) {
    
    NSString* path = [NSBundle mainBundle].bundlePath;
    string str = string([path UTF8String]);
#if TARGET_OS_OSX
    str.append("/Contents/Resources/");
#endif
    str.append("/assets/");
    str.append(assetPath);
    FILE* asset = fopen(str.data(), "rb");
    if (!asset) {
        oakLog("Failed to open asset: %s", assetPath);
        return NULL;
    }
    
    Data* data = new Data();
    fseek (asset, 0, SEEK_END);
    data->cb = ftell(asset);
    data->data = (uint8_t*) malloc (sizeof(char)*data->cb);
    fseek ((FILE*)asset, 0, SEEK_SET);
    size_t read = fread(data->data, 1, data->cb, (FILE*)asset);
    assert(read == data->cb);
    fclose(asset);
    return data;
    
}

#endif