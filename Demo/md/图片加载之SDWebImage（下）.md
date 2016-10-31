# 图片加载之SDWebImage（下）

上篇留下的两个入口接着深入分析，图片缓存`SDImageCache`和图片下载`SDWebImageDownloader`

![](http://oeb4c30x3.bkt.clouddn.com/work.jpg)

## SDImageCache
> SDImageCache maintains a memory cache and an optional disk cache. Disk cache write operations are performed asynchronous so it doesn’t add unnecessary latency to the UI.

`SDImageCache`包含内存缓存以及磁盘缓存。其中，磁盘缓存写入操作是异步执行的，因此不会给UI增加不必要的延迟

### 查找缓存图片

```
- (nullable NSOperation *)queryCacheOperationForKey:(nullable NSString *)key done:(nullable SDCacheQueryCompletedBlock)doneBlock;
```

1. 确定`key`值，实际上就是`url.absoluteString`
```
 NSString *key = [self cacheKeyForURL:url];
 
 // 如果key不存在，直接返回
    if (!key) {
        if (doneBlock) {
            doneBlock(nil, nil, SDImageCacheTypeNone);
        }
        return nil;
    }
```

2. 第一次在内存中查找：
 内部其实是通过`NSCache`来获取和存储的
```
 UIImage *image = [self imageFromMemoryCacheForKey:key];
 // 方法内部实现：memCache 是 NSCache类型
 // return [self.memCache objectForKey:key];
```

3. 查找到图片之后的操作
 其中，内存很快，几乎不需要时间，所以不需要一个执行任务，返回`operation`为nil
```
if (image) {
        NSData *diskData = nil;
        // 是否是gif图片，实际判断图片对应的数组是否为空
        if ([image isGIF]) {
            // 在磁盘缓存中根据key获取图片的data
            diskData = [self diskImageDataBySearchingAllPathsForKey:key];
        }
        if (doneBlock) {
            // 查询完成，缓存设置为SDImageCacheTypeMemory
            doneBlock(image, diskData, SDImageCacheTypeMemory);
        }
        return nil;
    }
```

4. 内存找不到的情况下，第二次在磁盘中查询
 磁盘查找比较耗时，所以需要创建一个执行任务`operation`
 `ioQueue`是一个串行队列，这里新开线程，异步执行,不会阻塞UI
```
NSOperation *operation = [NSOperation new];
    dispatch_async(self.ioQueue, ^{
        if (operation.isCancelled) {
            // do not call the completion if cancelled
            return;
        }
        // 磁盘文件IO会增大内存消耗，放在自动释放池中，降低内存峰值
        @autoreleasepool {
            // 通过key获取文件路径，然后通过文件路径获取data，其中文件路径内部是通过md5加密的
            NSData *diskData = [self diskImageDataBySearchingAllPathsForKey:key];
            
            // 内部实现也是先获取data，然后转换成Image，其中image是经过解压缩，根据屏幕同比例增大，甚至在必要情况下，解码重绘得到的，
            UIImage *diskImage = [self diskImageForKey:key];
            
            if (diskImage && self.config.shouldCacheImagesInMemory) {
                // 获取图片所占内存大小
                NSUInteger cost = SDCacheCostForImage(diskImage);
                // 在磁盘中找到图片，先放入内存中，以便下次直接使用
                [self.memCache setObject:diskImage forKey:key cost:cost];
            }

            // 查询结束返回更新UI
            if (doneBlock) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    // 查询完成，返回缓存设置为SDImageCacheTypeDisk
                    doneBlock(diskImage, diskData, SDImageCacheTypeDisk);
                });
            }
        }
        
   });

    return operation;
```

### 缓存存储图片

```
- (void)storeImage:(nullable UIImage *)image
         imageData:(nullable NSData *)imageData
            forKey:(nullable NSString *)key
            toDisk:(BOOL)toDisk
        completion:(nullable SDWebImageNoParamsBlock)completionBlock;
```

1. 首先进行内存缓存，通过`NSCache`的`- (void)setObject:(ObjectType)obj forKey:(KeyType)key cost:(NSUInteger)g;
`方法，
```
// 图片或者key不存在的情况下，直接返回
    if (!image || !key) {
        if (completionBlock) {
            completionBlock();
        }
        return;
    }
    // if memory cache is enabled
    // 内存缓存
    if (self.config.shouldCacheImagesInMemory) {
        NSUInteger cost = SDCacheCostForImage(image);
        
        [self.memCache setObject:image forKey:key cost:cost];
    }
```

2. 磁盘缓存，通过`NSFileManager`把文件存储磁盘
```
if (toDisk) {
        dispatch_async(self.ioQueue, ^{
            NSData *data = imageData;
            
            if (!data && image) {
                // 1. 根据data确定图片的格式，png/jpeg
                SDImageFormat imageFormatFromData = [NSData sd_imageFormatForImageData:data];
                // 2. 格式不同，转换data的方式不同，
                data = [image sd_imageDataAsFormat:imageFormatFromData];
            }
            // 3. 磁盘缓存，内部会做很多工作，是否io队列，创建文件夹，图片名字加密，是否存储iCloud，
            [self storeImageDataToDisk:data forKey:key];
            
            // 磁盘缓存需要时间，异步执行completionBlock，通知存储已经结束
            if (completionBlock) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    completionBlock();
                });
            }
        });
    } 
```

3. 回调`completeBlock`通知外部，存储已经完成
```
if (completionBlock) {
  completionBlock();
}
```

### 缓存清除
通过上面已经知道，内存缓存是使用`NSCache`，磁盘缓存是使用`NSFileManager`，所以缓存清除，对应的内存清除
```
[self.memCache removeObjectForKey:key]
```
磁盘清除
```
[_fileManager removeItemAtPath:[self defaultCachePathForKey:key] error:nil]
```

## SDWebImageDownloader
> Asynchronous downloader dedicated and optimized for image loading

优化过的专门用于加载图片的异步下载器
核心方法是：
```
- (nullable SDWebImageDownloadToken *)downloadImageWithURL:(nullable NSURL *)url
                                                   options:(SDWebImageDownloaderOptions)options
                                                  progress:(nullable SDWebImageDownloaderProgressBlock)progressBlock
                                                 completed:(nullable SDWebImageDownloaderCompletedBlock)completedBlock;
```

1. 方法需要返回一个`SDWebImageDownloadToken`对象，它跟下载器一一对应，可以被用来取消对应的下载。它有两个属性：
```
@property (nonatomic, strong, nullable) NSURL *url; // 当前下载器对应的url地址
@property (nonatomic, strong, nullable) id downloadOperationCancelToken; // 一个任意类型的对象，
```

2. 深入方法，内部实现其实是调用另一个方法。
该方法，用来添加各个回调方法block的。
而调用该一个方法，需要一个参数`SDWebImageDownloaderOperation`对象，block内部就是创建这个对象的。
```
return [self addProgressCallback:progressBlock completedBlock:completedBlock forURL:url createCallback:^SDWebImageDownloaderOperation *{ 
		// 内部返回一个SDWebImageDownloaderOperation对象
		// 创建
		SDWebImageDownloaderOperation *operation = [[sself.operationClass alloc] initWithRequest:request inSession:sself.session options:options];
		// 返回
		return operation;
};
```

3. `SDWebImageDownloaderOperation`继承于`NSOperation`，用来执行下载操作的。下面创建`SDWebImageDownloaderOperation`对象
```
 // 设置请求时间限制
   NSTimeInterval timeoutInterval = sself.downloadTimeout;
   if (timeoutInterval == 0.0) {
       timeoutInterval = 15.0;
   }

   // In order to prevent from potential duplicate caching (NSURLCache + SDImageCache) we disable the cache for image requests if told otherwise
   // 这里防止重复缓存，默认是阻止网络请求的缓存。
   // 创建网络请求request，并设置网络请求的缓存策略 ，是否使用网络缓存
   NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:url cachePolicy:(options & SDWebImageDownloaderUseNSURLCache ? NSURLRequestUseProtocolCachePolicy : NSURLRequestReloadIgnoringLocalCacheData) timeoutInterval:timeoutInterval];
   // 是否发送cookie
   request.HTTPShouldHandleCookies = (options & SDWebImageDownloaderHandleCookies);
   
   // 是否等待之前的返回响应，然后再发送请求
   // YES, 表示不等待
   request.HTTPShouldUsePipelining = YES;
   
   // 请求头
   if (sself.headersFilter) {
       request.allHTTPHeaderFields = sself.headersFilter(url, [sself.HTTPHeaders copy]);
   }
   else {
       request.allHTTPHeaderFields = sself.HTTPHeaders;
   }
   
   // 初始化一个图片下载操作，只有放入线程，或者调用start才会真正执行请求
   // 这里真正创建SDWebImageDownloaderOperation，
   SDWebImageDownloaderOperation *operation = [[sself.operationClass alloc] initWithRequest:request inSession:sself.session options:options];
```

4. 对下载操作`operation`的属性设置，并安排下载操作的优先级以及执行顺序
```
// 压缩图片
   operation.shouldDecompressImages = sself.shouldDecompressImages;
   
   // 对应网络请求设置请求凭证
   if (sself.urlCredential) {
       operation.credential = sself.urlCredential;
   } else if (sself.username && sself.password) {
       operation.credential = [NSURLCredential credentialWithUser:sself.username password:sself.password persistence:NSURLCredentialPersistenceForSession];
   }
   
   // 操作执行的优先级
   if (options & SDWebImageDownloaderHighPriority) {
       operation.queuePriority = NSOperationQueuePriorityHigh;
   } else if (options & SDWebImageDownloaderLowPriority) {
       operation.queuePriority = NSOperationQueuePriorityLow;
   }

   // 下载队列添加下载操作
   [sself.downloadQueue addOperation:operation];
   
   // 根据执行顺序，添加依赖
   if (sself.executionOrder == SDWebImageDownloaderLIFOExecutionOrder) {
       // Emulate LIFO execution order by systematically adding new operations as last operation's dependency
       [sself.lastAddedOperation addDependency:operation];
       
       sself.lastAddedOperation = operation;
   }
```

5. 分析第2步中，添加各个回调block的方法。
这个方法最终操作，是返回 `SDWebImageDownloadToken`对象
```
// 所有的下载操作都是以一个真实的url为前提，一旦url为nil，直接返回nil
    if (url == nil) {
        if (completedBlock != nil) {
            completedBlock(nil, nil, nil, NO);
        }
        return nil;
    }
    
    // 方法最终是要返回SDWebImageDownloadToken对象，这里声明，并使用__block修饰，以便在后续block中进行修改赋值
    __block SDWebImageDownloadToken *token = nil;
```

6. 使用GCD中的栅栏，来保证字典写入操作不会发生冲突。其中涉及到一个属性`URLOperations`，类型为`NSMutableDictionary<NSURL *, SDWebImageDownloaderOperation *>`，用来存储url对应的下载操作。
**注意其中的a点，这里是把对应于同一个`url`的多个下载操作，合并为一个，就是说，如果有多张`ImageView`对应于一个`url`，实际上执行一个下载操作，但他们的进度和完成block还是分开处理的，后续才有数组`callbackBlocks`来存储所有的blocks，当下载完成后，所有的block执行回调。当然，_前提是操作未结束_，还没执行`completionBlock`。**
```
// 多线程中的栅栏，barrierQueue 是一个并行队列
    dispatch_barrier_sync(self.barrierQueue, ^{
        
        // a. 根据url判断是否有对应的operation
        SDWebImageDownloaderOperation *operation = self.URLOperations[url];
        
        if (!operation) {
            
            // 如果没有，就赋值，createCallback()就是之前第3步创建的SDWebImageDownloaderOperation对象
            operation = createCallback();
            
            // 对应于a操作，赋值，存储
            self.URLOperations[url] = operation;

            __weak SDWebImageDownloaderOperation *woperation = operation;
            // 设置完成之后的回调
            operation.completionBlock = ^{
              
                SDWebImageDownloaderOperation *soperation = woperation;
              
                if (!soperation) return;
            
                // 操作已经结束了，移除该操作
                if (self.URLOperations[url] == soperation) {
                  
                    [self.URLOperations removeObjectForKey:url];
              
                }
            
            };
        }
        // 创建最终需要返回的对象，内部实现往下看
        id downloadOperationCancelToken = [operation addHandlersForProgress:progressBlock completed:completedBlock];

        token = [SDWebImageDownloadToken new];
        token.url = url;
        token.downloadOperationCancelToken = downloadOperationCancelToken;
    });
```


## SDWebImageDownloaderOperation
> `SDWebImageDownloaderOperation`继承于`NSOperation`，并实现了`<SDWebImageDownloaderOperationInterface>`协议，专门用来执行下载操作任务的。

1. 对于上述第6点中的，添加存储下载进度的回调block和完成回调block。
涉及到一个隐藏属性`callbackBlocks`，类型为`NSMutableArray<SDCallbacksDictionary *>`，用来存储进度回调和完成回调的block
所有对应于url的执行任务的回调block，都存储其中。
```
- (nullable id)addHandlersForProgress:(nullable SDWebImageDownloaderProgressBlock)progressBlock
                            completed:(nullable SDWebImageDownloaderCompletedBlock)completedBlock {
		 // 可变字典 typedef NSMutableDictionary<NSString *, id> SDCallbacksDictionary;
	    SDCallbacksDictionary *callbacks = [NSMutableDictionary new];
	    if (progressBlock) callbacks[kProgressCallbackKey] = [progressBlock copy];
	    if (completedBlock) callbacks[kCompletedCallbackKey] = [completedBlock copy];
	    // 使用栅栏，安全添加回调字典，
	    dispatch_barrier_async(self.barrierQueue, ^{
	        [self.callbackBlocks addObject:callbacks];
	    });
	    return callbacks;
}
```

2. 当`operation`添加进队列`downloadQueue`中后，会自动调用`start`方法，下面分析该方法
一旦操作取消，立马重置所有属性
```
if (self.isCancelled) {
       self.finished = YES;
       // 内部会移除所有回调block，属性置空
       [self reset];
       return;
   }
```

3. 进入后台后继续执行网络请求任务。
```
#if SD_UIKIT 
        Class UIApplicationClass = NSClassFromString(@"UIApplication");
        BOOL hasApplication = UIApplicationClass && [UIApplicationClass respondsToSelector:@selector(sharedApplication)];
        // 进入后台后也允许继续执行请求
        if (hasApplication && [self shouldContinueWhenAppEntersBackground]) {
            __weak __typeof__ (self) wself = self;
            UIApplication * app = [UIApplicationClass performSelector:@selector(sharedApplication)];
            // 开启后台执行任务
            self.backgroundTaskId = [app beginBackgroundTaskWithExpirationHandler:^{
                __strong __typeof (wself) sself = wself;

                // 后台执行是有时间限制的，当时间到期时，取消所有任务，关闭后台任务，并使之失效。
                if (sself) {
                    [sself cancel];

                    [app endBackgroundTask:sself.backgroundTaskId];
                    sself.backgroundTaskId = UIBackgroundTaskInvalid;
                }
            }];
        }
#endif
```

4. 创建数据请求任务
```
// iOS 7 以后，使用NSURLSession 来进行网络请求
   NSURLSession *session = self.unownedSession;
   if (!self.unownedSession) {
       NSURLSessionConfiguration *sessionConfig = [NSURLSessionConfiguration defaultSessionConfiguration];
       // 请求时间
       sessionConfig.timeoutIntervalForRequest = 15;
       
       /**
        *  Create the session for this task
        *  We send nil as delegate queue so that the session creates a serial operation queue for performing all delegate
        *  method calls and completion handler calls.
        */
       // 针对当前任务，创建session，
       // 这里代理队列为nil，所以，session创建一个串行操作队列，同步执行所有的代理方法和完成block回调
       self.ownedSession = [NSURLSession sessionWithConfiguration:sessionConfig
                                                         delegate:self
                                                    delegateQueue:nil];
       session = self.ownedSession;
   }
   
   // 创建数据请求任务
   self.dataTask = [session dataTaskWithRequest:self.request];
   self.executing = YES;
```

5. 任务开始执行
这里使用`@synchronized`，防止其他线程同时进行访问、处理。
`self.dataTask`每次只能创建一个，不能同时创建多个
```
@synchronized (self) { };
// 任务执行，请求发送
[self.dataTask resume];
```

6. 任务刚开始执行时候的处理
```
if (self.dataTask) {
        // 任务刚开始执行，同一个url对应的所有progressBlocks，进行一次信息回调。
        for (SDWebImageDownloaderProgressBlock progressBlock in [self callbacksForKey:kProgressCallbackKey]) {
            progressBlock(0, NSURLResponseUnknownLength, self.request.URL);
        }
        // 返回主线程发送通知，任务开始执行
        dispatch_async(dispatch_get_main_queue(), ^{
            [[NSNotificationCenter defaultCenter] postNotificationName:SDWebImageDownloadStartNotification object:self];
        });
        // 如果任务不存在，回调错误信息，“请求链接不存在”
    } else {
        [self callCompletionBlocksWithError:[NSError errorWithDomain:NSURLErrorDomain code:0 userInfo:@{NSLocalizedDescriptionKey : @"Connection can't be initialized"}]];
    }
```

7. 任务已经开始执行，后台任务就没必要存在了，关闭后台任务并使之失效
```
#if SD_UIKIT
    Class UIApplicationClass = NSClassFromString(@"UIApplication");
    if(!UIApplicationClass || ![UIApplicationClass respondsToSelector:@selector(sharedApplication)]) {
        return;
    }
    if (self.backgroundTaskId != UIBackgroundTaskInvalid) {
        UIApplication * app = [UIApplication performSelector:@selector(sharedApplication)];
        [app endBackgroundTask:self.backgroundTaskId];
        self.backgroundTaskId = UIBackgroundTaskInvalid;
    }
#endif
```

8. 数据请求，任务执行过程中，代理方法的各种回调
a. 任务已经获取完整的返回数据
```
- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
didReceiveResponse:(NSURLResponse *)response
 completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
	 	
	 	/*
	 	 1. 进度回调一次，把图片完整大小传出去
	 	 2. 发送通知，已经收到图片数据了
	 	 3. 如果失败，取消任务，并重置，发送通知，以及回调错误信息
	 	*/ 
 }
```
b. 网络数据接收过程中 
```
- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveData:(NSData *)data {
	  
	  /*
		1. data 拼接
		2. 如果需要，图片一节一节的显示
		3. 进度不断回调
		*/

 }
```
c. 主要用来进行网络数据缓存的处理
```
- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
 willCacheResponse:(NSCachedURLResponse *)proposedResponse
 completionHandler:(void (^)(NSCachedURLResponse *cachedResponse))completionHandler {
 
		 /*
		 	 进行非网络缓存的处理，或者进行特定的网络缓存处理
		 */
 
 }
```
d. 刚接收完最后一条数据时调用的方法
```
- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error { 

		/*
		1. 发送通知，任务完成，停止任务
		2. 图片处理并回调completionBlock
		3. 任务完成
		*/
}
```

---
![SDWebImageDownloader && SDWebImageDownloaderOperation](http://oeb4c30x3.bkt.clouddn.com/SDWebImageDownload.jpeg)

## 总结：

***1. `SDImageCache`主要是用来管理所有图片缓存相关方法的类，包括存储、获取、移除等***

***2. `SDWebImageDownloader`主要是用来处理生成`SDWebImageDownloaderOperation`的类，管理图片下载对应的操作，以及操作的一些属性设置。***

***3. `SDWebImageDownloaderOperation`用来管理数据网络请求的类，并把请求结果进行处理回调。***


