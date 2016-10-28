// UIImageView+AFNetworking.m
// Copyright (c) 2011–2016 Alamofire Software Foundation ( http://alamofire.org/ )
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#import "UIImageView+AFNetworking.h"

#import <objc/runtime.h>

#if TARGET_OS_IOS || TARGET_OS_TV

#import "AFImageDownloader.h"

@interface UIImageView (_AFNetworking)

// 设置方法添加前缀名，防止跟系统可能发生的冲突
@property (readwrite, nonatomic, strong, setter = af_setActiveImageDownloadReceipt:) AFImageDownloadReceipt *af_activeImageDownloadReceipt;
@end

@implementation UIImageView (_AFNetworking)

- (AFImageDownloadReceipt *)af_activeImageDownloadReceipt {
    return (AFImageDownloadReceipt *)objc_getAssociatedObject(self, @selector(af_activeImageDownloadReceipt));
}

- (void)af_setActiveImageDownloadReceipt:(AFImageDownloadReceipt *)imageDownloadReceipt {
    objc_setAssociatedObject(self, @selector(af_activeImageDownloadReceipt), imageDownloadReceipt, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end

#pragma mark -

@implementation UIImageView (AFNetworking)

// 如果没有绑定AFImageDownloader，就获取默认值，（单例）
+ (AFImageDownloader *)sharedImageDownloader {
    return objc_getAssociatedObject(self, @selector(sharedImageDownloader)) ?: [AFImageDownloader defaultInstance];
}

+ (void)setSharedImageDownloader:(AFImageDownloader *)imageDownloader {
    objc_setAssociatedObject(self, @selector(sharedImageDownloader), imageDownloader, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

#pragma mark -

- (void)setImageWithURL:(NSURL *)url {
    [self setImageWithURL:url placeholderImage:nil];
}

- (void)setImageWithURL:(NSURL *)url
       placeholderImage:(UIImage *)placeholderImage
{
    // 创建图片请求，并在请求头添加参数
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request addValue:@"image/*" forHTTPHeaderField:@"Accept"];

    [self setImageWithURLRequest:request placeholderImage:placeholderImage success:nil failure:nil];
}

- (void)setImageWithURLRequest:(NSURLRequest *)urlRequest
              placeholderImage:(UIImage *)placeholderImage
                       success:(void (^)(NSURLRequest *request, NSHTTPURLResponse * _Nullable response, UIImage *image))success
                       failure:(void (^)(NSURLRequest *request, NSHTTPURLResponse * _Nullable response, NSError *error))failure
{

    
    // 如果URL为空，就取消图片下载任务，并设置占位图
    if ([urlRequest URL] == nil) {
        
        [self cancelImageDownloadTask];
        
        self.image = placeholderImage;
        return;
    }

    // 阻止同一张图片进行多次相同的请求

    if ([self isActiveTaskURLEqualToURLRequest:urlRequest]){
        return;
    }
    
    
    
    // 下载开始之前，取消下载相关的所有操作，并把下载回执置空
    [self cancelImageDownloadTask];

    // AFImageDownloader 用来处理下载操作的类
    AFImageDownloader *downloader = [[self class] sharedImageDownloader];
    
    // 赋值一个支持AFImageRequestCache协议的对象，用来添加、移除、访问图片
    id <AFImageRequestCache> imageCache = downloader.imageCache;

    //Use the image from the image cache if it exists
    // 通过urlRequest来查找缓存，如果有缓存的话，使用缓存的图片，
    UIImage *cachedImage = [imageCache imageforRequest:urlRequest withAdditionalIdentifier:nil];
    if (cachedImage) {
        // 如果需要有成功返回结果，就进行回调，在回调block中进行手动设置imageView.image
        if (success) {
            success(urlRequest, nil, cachedImage);
        } else {
            self.image = cachedImage;
        }
        // 操作已经完成，把下载回执置空，
        [self clearActiveDownloadInformation];
        
        
    } else {
        // 如果没有缓存，就先设置占位图，然后再进行下面的操作
        if (placeholderImage) {
            self.image = placeholderImage;
        }

        // 弱引用，防止保留环
        __weak __typeof(self)weakSelf = self;
        
        /* Note: NSUUID is not toll-free bridged with CFUUID. Use UUID strings to convert between CFUUID and NSUUID, if needed. NSUUIDs are not guaranteed to be comparable by pointer value (as CFUUIDRef is); use isEqual: to compare two NSUUIDs. */
        
        // NSUUID 不是免费链接CFUUID的桥梁，如果需要的话，可以使用UUID 字符串在两者之间互相转换。NSUUID不能通过=来比较，使用 isEqual 来比较两个NSUUIDS
        // NSUUID 就是用来创建唯一标识的
        NSUUID *downloadID = [NSUUID UUID];
        
        
        // 图片下载回执，主要是用来处理下载任务的
        AFImageDownloadReceipt *receipt;
        
        // 进行的是图片下载操作
        receipt = [downloader
                   downloadImageForURLRequest:urlRequest
                   withReceiptID:downloadID
                   success:^(NSURLRequest * _Nonnull request, NSHTTPURLResponse * _Nullable response, UIImage * _Nonnull responseObject) {
                       // 成功情况下，在block中，强引用下，避免在运行过程中，self被自动释放
                       __strong __typeof(weakSelf)strongSelf = weakSelf;
                       // 根据任务标识判断，避免发生返回结果不是之前请求的结果
                       if ([strongSelf.af_activeImageDownloadReceipt.receiptID isEqual:downloadID]) {
                           // 如果有成功块，就返回成功块，并在block中手动设置图片
                           if (success) {
                               success(request, response, responseObject);
                           } else if(responseObject) {
                               strongSelf.image = responseObject;
                           }
                           // 操作完成，下载回执置空
                           [strongSelf clearActiveDownloadInformation];
                       }

                   }
                   failure:^(NSURLRequest * _Nonnull request, NSHTTPURLResponse * _Nullable response, NSError * _Nonnull error) {
                       // 失败情况下，返回错误信息，并清除下载信息
                       __strong __typeof(weakSelf)strongSelf = weakSelf;
                        if ([strongSelf.af_activeImageDownloadReceipt.receiptID isEqual:downloadID]) {
                            if (failure) {
                                failure(request, response, error);
                            }
                            [strongSelf clearActiveDownloadInformation];
                        }
                   }];
        
        // 下载回执赋值，
        
        
        self.af_activeImageDownloadReceipt = receipt;
    }
}

- (void)cancelImageDownloadTask {
    // 如果有下载回执的情况下，
    if (self.af_activeImageDownloadReceipt != nil) {
        
        [[self.class sharedImageDownloader] cancelTaskForImageDownloadReceipt:self.af_activeImageDownloadReceipt];
        [self clearActiveDownloadInformation];
     }
}

- (void)clearActiveDownloadInformation {
    self.af_activeImageDownloadReceipt = nil;
}

- (BOOL)isActiveTaskURLEqualToURLRequest:(NSURLRequest *)urlRequest {
    return [self.af_activeImageDownloadReceipt.task.originalRequest.URL.absoluteString isEqualToString:urlRequest.URL.absoluteString];
}

@end

#endif
