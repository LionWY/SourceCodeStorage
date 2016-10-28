# 图片加载之SDWebImage（上）
> Asynchronous image downloader with cache support as a UIImageView category

支持图片异步下载和缓存的UIImageView分类


![](http://oeb4c30x3.bkt.clouddn.com/light.jpg)


## UIView+WebCache

1. 最基本的方法是`UIImageView+WebCache`中这个方法
```
 - (void)sd_setImageWithURL:(nullable NSURL *)url;
```

2. 一步步走下来，会发现实际运用的是`UIView+WebCache`中的方法，包括`UIButton+WebCache`内部核心方法也是调用的下面的方法，其中`SDWebImageOptions`策略详细介绍可以看[这里]()
```
- (void)sd_internalSetImageWithURL:(nullable NSURL *)url
                  placeholderImage:(nullable UIImage *)placeholder
                           options:(SDWebImageOptions)options
                      operationKey:(nullable NSString *)operationKey
                     setImageBlock:(nullable SDSetImageBlock)setImageBlock
                          progress:(nullable SDWebImageDownloaderProgressBlock)progressBlock
                         completed:(nullable SDExternalCompletionBlock)completedBlock;
```

3. 进入方法内部：先取消相关的所有下载
```
    //  operationKey  用来描述当前操作的关键字标识，默认值是类名字，即 @"UIImageView"
    NSString *validOperationKey = operationKey ?: NSStringFromClass([self class]);
    
    // 取消当前view下 跟validOperationKey有关的所有下载操作，以保证不会跟下面的操作有冲突
    [self sd_cancelImageLoadOperationWithKey:validOperationKey];
    
    // 通过runtime的关联对象给分类添加属性，设置图片地址
    objc_setAssociatedObject(self, &imageURLKey, url, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
```
其中取消操作方法内部涉及到一个协议`<SDWebImageOperation>`，这个协议只有一个`cancel`方法，可见，这个协议就是用来取消操作的，只要遵守该协议的类，必定会有`cancel`方法。
```
@protocol SDWebImageOperation <NSObject>

	- (void)cancel;

	@end
```

	取消方法的具体实现：
	涉及到一个字典`SDOperationsDictionary`类型为`NSMutableDictionary<NSString *, id>`，也是通过关联对象添加为UIView的属性，用来存储UIView的所有下载操作，方便之后的取消/移除
```
- (void)sd_cancelImageLoadOperationWithKey:(nullable NSString *)key {
	    
	    // 从队列中取消跟key有关的所有下载操作
	    // 任何实现协议的对象都执行取消操作
	    SDOperationsDictionary *operationDictionary = [self operationDictionary];
	    id operations = operationDictionary[key];
	    if (operations) {
	        if ([operations isKindOfClass:[NSArray class]]) {
		        
	            for (id <SDWebImageOperation> operation in operations) {
	                if (operation) {
	                    [operation cancel];
	                }
	            }
	        } else if ([operations conformsToProtocol:@protocol(SDWebImageOperation)]){
	            [(id<SDWebImageOperation>) operations cancel];
	        }
	        // 最后从字典中移除key
	        [operationDictionary removeObjectForKey:key];
	    }
	}
```

4. 如果没有设置延迟加载占位图`SDWebImageDelayPlaceholder`，就会先进行加载占位图，
```
if (!(options & SDWebImageDelayPlaceholder)) {
	   dispatch_main_async_safe(^{
     	       
            // 返回主线程中进行UI设置，把占位图当成image进行图片设置，在方法内部会进行UIButton和UIImageView的判断区分
            [self sd_setImage:placeholder imageData:nil basedOnClassOrViaCustomSetImageBlock:setImageBlock];
        });
    }
```
其中有一个宏定义，通过字符串的比较来获取主线程
```
#ifndef dispatch_main_async_safe
#define dispatch_main_async_safe(block)\
	    if (strcmp(dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL), dispatch_queue_get_label(dispatch_get_main_queue())) == 0) {\
	        block();\
	    } else {\
	        dispatch_async(dispatch_get_main_queue(), block);\
	    }
#endif
```

5. 判断url，url为空的情况下，直接返回错误信息
```
	if (url) {
        // check if activityView is enabled or not
        // 检查菊花
        if ([self sd_showActivityIndicatorView]) {
            [self sd_addActivityIndicator];
        }
        // url 存在的情况下进行的操作...
	} else {
        // url 为nil的情况下，生成错误信息，并返回         
        dispatch_main_async_safe(^{
            // 移除菊花
            [self sd_removeActivityIndicator];
            if (completedBlock) {
                NSError *error = [NSError errorWithDomain:SDWebImageErrorDomain code:-1 userInfo:@{NSLocalizedDescriptionKey : @"Trying to load a nil url"}];
                completedBlock(nil, error, SDImageCacheTypeNone, url);
            }
        });
    }
```

6. url不为nil的情况下，获取图片信息，并生成`operation`，然后存储。
```
// 返回的是一个遵从了SDWebImageOperation协议的NSObject的子类，目的是方便之后的取消/移除操作
id <SDWebImageOperation> operation = [SDWebImageManager.sharedManager loadImageWithURL:url options:options progress:progressBlock completed:^(UIImage *image, NSData *data, NSError *error, SDImageCacheType cacheType, BOOL finished, NSURL *imageURL) { /*完成之后的操作*/ }];
        
	// 根据validOperationKey把生成的operation放入字典`SDOperationsDictionary`中，这个字典也是通过关联对象，作为UIView的一个属性。
	[self sd_setImageLoadOperation:operation forKey:validOperationKey];
```

7. `SDInternalCompletionBlock`是在UIView内部使用的`completedBlock`，在block中，返回获取到的图片，以及相关信息。最后在主线程中，进行UI更新并更新布局。
```
// weak 避免 保留环
   __weak __typeof(self)wself = self;
   
   id <SDWebImageOperation> operation = [SDWebImageManager.sharedManager loadImageWithURL:url options:options progress:progressBlock completed:^(UIImage *image, NSData *data, NSError *error, SDImageCacheType cacheType, BOOL finished, NSURL *imageURL) {
       
       // block 中强引用替换，避免使用过程中被系统自动释放
       __strong __typeof (wself) sself = wself;
       // 加载完成移除菊花
       [sself sd_removeActivityIndicator];
       
       if (!sself) {
           return;
       }
       dispatch_main_async_safe(^{
           if (!sself) {
               return;
           }
           // SDWebImageAvoidAutoSetImage, 对图片进行手动设置，开发者在外面的complete里面可以对图片设置特殊效果，然后赋值ImageView.image
           if (image && (options & SDWebImageAvoidAutoSetImage) && completedBlock) {
               completedBlock(image, error, cacheType, url);
               return;
           } else if (image) {
               
               // 更新图片，内部会进行imageView或者button的判断
               [sself sd_setImage:image imageData:data basedOnClassOrViaCustomSetImageBlock:setImageBlock];
               // 更新布局Layout
               [sself sd_setNeedsLayout];
           } else {
               // SDWebImageDelayPlaceholder 延迟加载占位图，下载完成后才会进行设置
               if ((options & SDWebImageDelayPlaceholder)) {
                   [sself sd_setImage:placeholder imageData:nil basedOnClassOrViaCustomSetImageBlock:setImageBlock];
                   [sself sd_setNeedsLayout];
               }
           }
           // 如果有返回block，返回block和其它信息
           if (completedBlock && finished) {
               completedBlock(image, error, cacheType, url);
           }
       });
   }];
```
### 总结：
![UIView+WebCache流程图](http://oeb4c30x3.bkt.clouddn.com/UIView+WebCache%E6%B5%81%E7%A8%8B%E5%9B%BE.png)

## SDWebImageManager
> The SDWebImageManager is the class behind the UIImageView+WebCache category and likes. It ties the asynchronous downloader (SDWebImageDownloader) with the image cache store (SDImageCache). You can use this class directly to benefit from web image downloading with caching in another context than a UIView.

`SDWebImageManager`起一个承上启下的作用，紧密连接图片下载`SDWebImageDownloader`和图片缓存`SDImageCache`，可以直接通过这个类获取缓存中的图片。

核心方法（也是`UIView+WebCache`的第6步）：
```
- (nullable id <SDWebImageOperation>)loadImageWithURL:(nullable NSURL *)url
                                              options:(SDWebImageOptions)options
                                             progress:(nullable SDWebImageDownloaderProgressBlock)progressBlock
                                            completed:(nullable SDInternalCompletionBlock)completedBlock;
```
内部实现：

1. 如果`completedBlock`为空，直接闪退并抛出错误信息。即，`completedBlock`不能为空。
	* `NSAssert`只有在`debug`状态下有效

			// Invoking this method without a completedBlock is pointless
			    NSAssert(completedBlock != nil, @"If you mean to prefetch the image, use -[SDWebImagePrefetcher prefetchURLs] instead");


2. 确保`url`是正确的，加安全验证，虽然`url`偶尔在字符串的情况下不报警告，但最好还是转换成`NSURL`类型，
```
if ([url isKindOfClass:NSString.class]) {
        url = [NSURL URLWithString:(NSString *)url];
    }

    // 防止url在某些特殊情况下（eg：NSNull）导致app闪退
    if (![url isKindOfClass:NSURL.class]) {
        url = nil;
    }
```

3. 首先方法要返回的是遵从了`<SDWebImageOperation>`协议的对象，所以声明了一个对象`SDWebImageCombinedOperation`，该对象遵从了协议，下面会对其属性进行一一设置。
而`cancelled`属性是在`UIView+WebCache`第3点设置的。
```
@property (assign, nonatomic, getter = isCancelled) BOOL cancelled;
@property (copy, nonatomic, nullable) SDWebImageNoParamsBlock cancelBlock;
@property (strong, nonatomic, nullable) NSOperation *cacheOperation;
```
最后需要返回`operation`，所以进行创建、赋值、返回。
```
// 创建一个SDWebImageCombinedOperation，加上 __block，可以让它在后续block内进行修改，
    __block SDWebImageCombinedOperation *operation = [SDWebImageCombinedOperation new];
    // 加上__weak 避免保留环
    __weak SDWebImageCombinedOperation *weakOperation = operation;
    
	/*
	对operation 进行赋值操作，最后返回
	*/
	
	return operation;
```

4. 再次对`url`进行判断，`failedURLs`类型是`NSMutableSet<NSURL *>`，是用来存储错误`url`的集合
```
	 // 声明一个BOOL值，isFailedUrl
    BOOL isFailedUrl = NO;
    if (url) {
        // 创建一个同步锁，@synchronized{}它防止不同的线程同时执行同一段代码
        @synchronized (self.failedURLs) {
            // 错误的url都会放在failedURLs 中，判断该url是否在里面,返回并赋值isFailedUrl
            isFailedUrl = [self.failedURLs containsObject:url];
        }
    }
    
    // 如果url长度为0，或者 options中没有 SDWebImageRetryFailed（一直进行下载）， 并且是错误的url
    if (url.absoluteString.length == 0 || (!(options & SDWebImageRetryFailed) && isFailedUrl)) {
        
        // 不再向下执行，直接回调completeBlock，并传递错误信息，url不存在，NSURLErrorFileDoesNotExist
        [self callCompletionBlockForOperation:operation completion:completedBlock error:[NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorFileDoesNotExist userInfo:nil] url:url];
        return operation;
    }
```

5. `runningOperations`类型为`NSMutableArray<SDWebImageCombinedOperation *>`存储所有待执行的操作任务
```
@synchronized (self.runningOperations) {
        // 把operation 存储起来
        [self.runningOperations addObject:operation];
    }
```

6. 在缓存中查找图片，并将找到的图片的相关信息返回，
同时对`operation.cacheOperation`属性赋值。
（该方法是`SDImageCache`类的实例方法，明天再分析）
```
// 根据url 返回一个本地用来缓存的标志 key
    NSString *key = [self cacheKeyForURL:url];
    
    operation.cacheOperation = [self.imageCache queryCacheOperationForKey:key done:^(UIImage *cachedImage, NSData *cachedData, SDImageCacheType cacheType) {	
		 // 查询结束后执行操作   
     }];
```

7. 缓存中是否查找到图片，分别处理：
   a. 找不到图片，并且可以从网络下载，就进行网络下载
```
// 如果执行过程中操作取消，安全移除操作
// return 是跳出这个block
   if (operation.isCancelled) {
       [self safelyRemoveOperationFromRunning:operation];
       return;
   }
   // 1. 如果不存在缓存图片，或者需要刷新缓存 2. 代理可以响应方法，或者代理直接执行该方法，即从网络下载图片
   // 1 和 2 是并且关系
   if ((!cachedImage || options & SDWebImageRefreshCached) && (![self.delegate respondsToSelector:@selector(imageManager:shouldDownloadImageForURL:)] || [self.delegate imageManager:self shouldDownloadImageForURL:url])) { 
    		//  网络下载图片
}
```
b. 如果找到了缓存图片，回调图片及相关信息，操作结束，安全移除操作
``` 
else if (cachedImage) {
       __strong __typeof(weakOperation) strongOperation = weakOperation;
       [self callCompletionBlockForOperation:strongOperation completion:completedBlock image:cachedImage data:cachedData error:nil cacheType:cacheType finished:YES url:url];
       [self safelyRemoveOperationFromRunning:operation];
   } 
   ```
c. 缓存中找不到图片，也不允许网络下载图片：
```
else {
		  // Image not in cache and download disallowed by delegate
		  __strong __typeof(weakOperation) strongOperation = weakOperation;
		  [self callCompletionBlockForOperation:strongOperation completion:completedBlock image:nil data:nil error:nil cacheType:SDImageCacheTypeNone finished:YES url:url];
		  [self safelyRemoveOperationFromRunning:operation];
}
```

8. 对a步骤一步步分析：如果有缓存图片，同时还要求刷新缓存，那么界面先加载缓存图片，然后网络下载，下载成功之后界面加载网络图片，然后在缓存中刷新之前的缓存图片
```
if (cachedImage && options & SDWebImageRefreshCached) {
           
           // If image was found in the cache but SDWebImageRefreshCached is provided, notify about the cached image
           // AND try to re-download it in order to let a chance to NSURLCache to refresh it from server.
           // 如果在缓存中找到了图片，但是设置了SDWebImageRefreshCached，因此要NSURLCache重新从服务器下载
           // 先调用completeBlock后续进行网络下载
           [self callCompletionBlockForOperation:weakOperation completion:completedBlock image:cachedImage data:cachedData error:nil cacheType:cacheType finished:YES url:url];
       }
```

9. 根据`SDWebImageOptions`的选项对`SDWebImageDownloaderOptions`进行对接，一一对应，协调处理。`|=` 可以理解为`添加`
  `SDWebImageDownloaderOptions`的详细介绍点[这里]()
```
downloaderOptions |= SDWebImageDownloaderLowPriority
// 等同于
downloaderOptions = downloaderOptions | SDWebImageDownloaderLowPriority
```
```
// downloaderOptions 默认为0
 SDWebImageDownloaderOptions downloaderOptions = 0;
  if (options & SDWebImageLowPriority)  downloaderOptions |= SDWebImageDownloaderLowPriority;
  
    // 如果需要刷新缓存，downloaderOptions强制解除SDWebImageDownloaderProgressiveDownload，并且添加SDWebImageDownloaderIgnoreCachedResponse选项
  if (cachedImage && options & SDWebImageRefreshCached) {
	      // force progressive off if image already cached but forced refreshing
	      downloaderOptions &= ~SDWebImageDownloaderProgressiveDownload;
	      // ignore image read from NSURLCache if image if cached but force refreshing
	      downloaderOptions |= SDWebImageDownloaderIgnoreCachedResponse;
  }
```

10. 通过`url`进行网络下载图片：
 每一个下载`SDWebImageDownloader`对象对应于一个`SDWebImageDownloadToken`对象，目的是用于取消/移除`SDWebImageDownloader`对象。
通过`SDWebImageDownloader`的实例方法生成一个`SDWebImageDownloadToken`对象。（该方法明天分析）
```
SDWebImageDownloadToken *subOperationToken = [self.imageDownloader downloadImageWithURL:url options:downloaderOptions progress:progressBlock completed:^(UIImage *downloadedImage, NSData *downloadedData, NSError *error, BOOL finished) {
// 图片下载完成之后的操作。
}];
```
对`operation.cancelBlock`赋值。
通过上面生成的`subOperationToken`来进行取消`SDWebImageDownloader`操作
```
operation.cancelBlock = ^{
                [self.imageDownloader cancel:subOperationToken];
                __strong __typeof(weakOperation) strongOperation = weakOperation;
                [self safelyRemoveOperationFromRunning:strongOperation];
  };
```

11. 操作取消或者存在网络错误的情况下：
```
// 操作不存在或者操作取消的情况下不做任何处理。
__strong __typeof(weakOperation) strongOperation = weakOperation;
if (!strongOperation || strongOperation.isCancelled) {
		// https://github.com/rs/SDWebImage/pull/699
 } else if (error) {
	     [self callCompletionBlockForOperation:strongOperation completion:completedBlock error:error url:url];
			// 在下面情况下（不是因为网络问题），url本身有问题的情况下，才会添加进failedURLs
	     if (   error.code != NSURLErrorNotConnectedToInternet
	         && error.code != NSURLErrorCancelled
	         && error.code != NSURLErrorTimedOut
	         && error.code != NSURLErrorInternationalRoamingOff
	         && error.code != NSURLErrorDataNotAllowed
	         && error.code != NSURLErrorCannotFindHost
	         && error.code != NSURLErrorCannotConnectToHost) {
	         
	         // 跟前面第4点对应，failedURLs是否包含url
	         @synchronized (self.failedURLs) {
	             [self.failedURLs addObject:url];
	         }
	     }
 }
```

12. 成功情况下：
a. 对应于第8点，刷新缓存，但是没有下载图片的情况下：
```
// 下载选项，允许失败后重新下载，
     if ((options & SDWebImageRetryFailed)) {
         // 重新下载，得保证 url 是正确的，不在failedURLs里面
         @synchronized (self.failedURLs) {
             [self.failedURLs removeObject:url];
         }
     }
     
     // 是否允许磁盘缓存
     BOOL cacheOnDisk = !(options & SDWebImageCacheMemoryOnly);

     // 没有下载图片的情况下，不能刷新缓存
     if (options & SDWebImageRefreshCached && cachedImage && !downloadedImage) {
         // Image refresh hit the NSURLCache cache, do not call the completion block
         // 对应于第8点，已经返回completeBlock，这里不做任何处理。
         
     }
```
b. 1. 有下载图片 2. 界面上下载图片尚未赋值，或者策略允许图片变换 3. 代理响应了图片变换操作
1，2，3 是并且关系。
图片先进行变换，然后缓存，最后回调
```
else if (downloadedImage && (!downloadedImage.images || (options & SDWebImageTransformAnimatedImage)) && [self.delegate respondsToSelector:@selector(imageManager:transformDownloadedImage:withURL:)]) {
	    // 在全局队列（并发）中，开启一个子线程，异步执行，优先级比较高
	    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
	        // 在缓存之前，就对图片进行处理变换，外层要手动实现代理方法
	        UIImage *transformedImage = [self.delegate imageManager:self transformDownloadedImage:downloadedImage withURL:url];
				// 变换图片处理完成
	        if (transformedImage && finished) {
	        		// 判断图片是否变换
	            BOOL imageWasTransformed = ![transformedImage isEqual:downloadedImage];
	            // pass nil if the image was transformed, so we can recalculate the data from the image
	            // 如果图片变换成功，imageData传nil，这样在缓存图片的时候，可以重新计算data大小，反之，就传downloadedData
	            
	            [self.imageCache storeImage:transformedImage imageData:(imageWasTransformed ? nil : downloadedData) forKey:key toDisk:cacheOnDisk completion:nil];
	        }
	        // 回调信息
	        [self callCompletionBlockForOperation:strongOperation completion:completedBlock image:transformedImage data:downloadedData error:nil cacheType:SDImageCacheTypeNone finished:finished url:url];
	    });
	}
```
c. 不对图片进行处理，直接缓存图片并回调。
```
else {
         if (downloadedImage && finished) {
             [self.imageCache storeImage:downloadedImage imageData:downloadedData forKey:key toDisk:cacheOnDisk completion:nil];
         }
         [self callCompletionBlockForOperation:strongOperation completion:completedBlock image:downloadedImage data:downloadedData error:nil cacheType:SDImageCacheTypeNone finished:finished url:url];
     }
```

### 总结：
![SDWebImageManager流程图](http://oeb4c30x3.bkt.clouddn.com/SDWebImageManager.png)

----


