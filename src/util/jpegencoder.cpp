//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//

#include <oaknut.h>


void JpegEncoder::encode(class Bitmap* bitmap, std::function<void(const bytearray&)> resultCallback) {
    if (!_worker) {
        _worker = new Worker("JpegEncoderWorker");
    }
    variant data_in;
    data_in.set("quality", 90);
    data_in.set("width", bitmap->_width);
    data_in.set("height", bitmap->_height);
    data_in.set("format", bitmap->_format);
    PIXELDATA pixelData;
    bitmap->lock(&pixelData, false);
    data_in.set("data", bytearray((uint8_t*)pixelData.data, pixelData.cb));

    _worker->enqueue(0, data_in, [=](const variant& data_out) {
        resultCallback(data_out.bytearrayRef());
    });
}
