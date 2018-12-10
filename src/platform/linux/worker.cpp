//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//
#if PLATFORM_LINUX

#import <oaknut.h>


void Worker::start(const variant& config) {
    assert(!_queue); // already started!
    retain();
    auto config_copy = config;
    _queue = new PosixTaskQueue("Worker");
    _queue->enqueueTask([=]() {
        _impl->start_(config_copy);
    });
    _started = true;
}

void Worker::dispatchProcessResult(const variant& data_out) {
    // not used
}

void Worker::process(const variant& data_in, std::function<void(const variant&)> callback) {
    if (!_started) {
        app.log("Warning! process() called on stopped worker");
        return;
    }
    assert(_queue); // not started!
    variant data_copy = data_in;
    retain();
    _queue->enqueueTask([=]() {
        variant data_out = _impl->process_(data_copy);
        App::postToMainThread([=]() {
            callback(data_out);
            release();
        });
    });
}

void Worker::dispatchStopped() {
    // not used
}

void Worker::stop(std::function<void()> onStop) {
    _started = false;
    assert(_queue); // not started!
    retain();
    _queue->enqueueTask([=]() {
        _impl->stop_();
        App::postToMainThread([=]() {
            onStop();
            release();
        });
    });

}


#endif