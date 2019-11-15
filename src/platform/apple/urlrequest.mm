//
// Copyright © 2018 Sandcastle Software Ltd. All rights reserved.
//
// This file is part of 'Oaknut' which is released under the MIT License.
// See the LICENSE file in the root of this installation for details.
//
#if PLATFORM_APPLE

#import "oaknut.h"

static struct loader {
    loader() {
        NSURLCache* cache = [[NSURLCache alloc] initWithMemoryCapacity:16*1024*1024 diskCapacity:32*1024*1024 diskPath:nil];
        NSURLCache.sharedURLCache = cache;
    }
} s_loader;

class URLRequestApple : public URLRequest {
public:
    URLRequestApple(const string& url, const string& method, const bytearray& body, int flags)
        : URLRequest(url, method, body, flags) {
    }
    ~URLRequestApple() {
        _dataTask = NULL;
    }
    
    void run() override {
        // Effectively use cache data ONLY, if available,
        // fall back to network: NSURLRequestReturnCacheDataElseLoad
        // Normal NSURLRequestReloadRevalidatingCacheData
        NSString* urlstr = [NSString stringWithCString:_url.c_str() encoding:[NSString defaultCStringEncoding]];
        NSMutableURLRequest* req = [[NSMutableURLRequest alloc] initWithURL:[NSURL URLWithString:urlstr] cachePolicy:NSURLRequestReturnCacheDataElseLoad timeoutInterval:15];
        req.HTTPMethod = [NSString stringWithUTF8String:_method.c_str()];
        if (_body.length() > 0) {
            req.HTTPBody = [NSData dataWithBytes:_body.data() length:_body.size()];
        }
        for (auto& header : _headers) {
            NSString* headerName = [NSString stringWithUTF8String:header.first.c_str()];
            NSString* headerValue = [NSString stringWithUTF8String:header.second.c_str()];
            [req setValue:headerValue forHTTPHeaderField:headerName];
        }
        _dataTask = [[NSURLSession sharedSession] dataTaskWithRequest:req completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            if (_cancelled || error.code == -999) { // cancelled, just abort.
                dispatchResult(0, {});
            } else {
                NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
                
                // Get all response headers into a generic container
                map<string, string> responseHeaders;
                for (NSString* headerName in httpResponse.allHeaderFields.allKeys) {
                    responseHeaders[[[headerName lowercaseString] cStringUsingEncoding:NSUTF8StringEncoding]] = [httpResponse.allHeaderFields[headerName] cStringUsingEncoding:NSUTF8StringEncoding];
                }
                
                // Slurp the data
                [data enumerateByteRangesUsingBlock:^(const void * _Nonnull bytes, NSRange byteRange, BOOL * _Nonnull stop) {
                    _responseData.append((uint8_t*)bytes, (int32_t)byteRange.length);
                }];

                dispatchResult((int)httpResponse.statusCode, responseHeaders);
            }
        }];
        [_dataTask resume];

    }
    
    void cancel() override {
        _cancelled = true;
        if (_dataTask.state == NSURLSessionTaskStateRunning) {
            [_dataTask cancel];
        }
    }
    
    NSURLSessionDataTask* _dataTask;
};

URLRequest* URLRequest::create(const string& url, const string& method, const bytearray& body, int flags) {
    return new URLRequestApple(url, method, body, flags);
}



#endif
