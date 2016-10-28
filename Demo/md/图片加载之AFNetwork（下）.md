## AFImageDownloader
> The AFImageDownloader class is responsible for downloading images in parallel on a prioritized queue. Incoming downloads are added to the front or back of the queue depending on the download prioritization. Each downloaded image is cached in the underlying NSURLCache as well as the in-memory image cache. By default, any download request with a cached image equivalent in the image cache will automatically be served the cached image representation.

AFImageDownloader类是负责下载图片的，并且根据下载优先级，把新传入的下载添加在队列的前面或后面。每个下载好的图片不仅被缓存在底层的NSURLCache中（NSURLCache只是被用来自动缓存网络请求，并没有进行图片缓存），也缓存在内存中的图片缓存中（上篇写的AFAutoPurgingImageCache类）。默认情况下，如果请求的图片有缓存的话，会直接返回缓存图片。

![research_2.jpg](http://oeb4c30x3.bkt.clouddn.com/research_2.jpg)

1、简单了解下`AFImageDownloader`的主要属性

```
.h 中可以被外部使用的属性
@property (nonatomic, strong, nullable) id <AFImageRequestCache> imageCache;// 储存下载图片的对象
@property (nonatomic, strong) AFHTTPSessionManager *sessionManager;// 进行数据请求的对象
@property (nonatomic, assign) AFImageDownloadPrioritization downloadPrioritizaton;// 下载优先级
.m 中不让外部调用的属性有：
@property (nonatomic, assign) NSInteger maximumActiveDownloads;// 同时处理线程的最大值，默认为4
@property (nonatomic, assign) NSInteger activeRequestCount;// 正在进行的请求数量，
```

2、`AFImageDownloader` 有两个属性`NSMutableArray *queuedMergedTasks`，`NSMutableDictionary *mergedTasks`用来存储另一个对象`AFImageDownloaderMergedTask`（用来操作下载任务合并的对象）

```
@property (nonatomic, strong) NSString *URLIdentifier;// 数据请求的URL地址
@property (nonatomic, strong) NSUUID *identifier;// 下载任务的唯一标识
@property (nonatomic, strong) NSURLSessionDataTask *task; // 处理数据请求任务的对象
```

3、`AFImageDownloaderMergedTask`有一个属性`NSMutableArray <AFImageDownloaderResponseHandler*> *responseHandlers;`来存储返回结果操作的对象`AFImageDownloaderResponseHandler`

```
@property (nonatomic, strong) NSUUID *uuid;// 合并任务的唯一标识
@property (nonatomic, copy) void (^successBlock)(NSURLRequest*, NSHTTPURLResponse*, UIImage*);// 成功块
@property (nonatomic, copy) void (^failureBlock)(NSURLRequest*, NSHTTPURLResponse*, NSError*);// 失败块
```

4、返回到昨天那个处理图片下载的核心方法

```
- (nullable AFImageDownloadReceipt *)downloadImageForURLRequest:(NSURLRequest *)request
                                                 withReceiptID:(NSUUID *)receiptID
                                                        success:(nullable void (^)(NSURLRequest *request, NSHTTPURLResponse  * _Nullable response, UIImage *responseObject))success
                                                        failure:(nullable void (^)(NSURLRequest *request, NSHTTPURLResponse * _Nullable response, NSError *error))failure;
```
返回值是`AFImageDownloadReceipt`，而它的声明方法只有一个，因此需要一个NSURLSessionDataTask对象（iOS 7.0 以后，代替`NSURLConnection`用来处理数据请求的类）
```
@implementation AFImageDownloadReceipt
- (instancetype)initWithReceiptID:(NSUUID *)receiptID task:(NSURLSessionDataTask *)task
```

5、声明一个NSURLSessionDataTask对象，用来创建`AFImageDownloadReceipt`。`__block`修饰，用来在block中更改task值，

```
__block NSURLSessionDataTask *task = nil;
```

6、在一个串行队列里中同步执行，直至结束，如果task存在，创建`AFImageDownloadReceipt`并返回，否则返回nil

```
dispatch_sync(self.synchronizationQueue, ^{ 
	// 在block中对task进行赋值
};
if (task) {
        return [[AFImageDownloadReceipt alloc] initWithReceiptID:receiptID task:task];
} else {
   return nil;
}
```

7、再次对URLRequest进行判断

```
NSString *URLIdentifier = request.URL.absoluteString;
   // 如果请求地址有误，直接就进行失败回调
   if (URLIdentifier == nil) {
       if (failure) {
           NSError *error = [NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorBadURL userInfo:nil];
           // 因为会有UI的交互，所以返回主线程处理failure
           dispatch_async(dispatch_get_main_queue(), ^{
               failure(request, nil, error);
           });
       }
       // return 是跳出这个同步线程
       return;
   }
```

8、如果请求已经存在，并且根据url地址能够找到AFImageDownloaderMergedTask，直接返回对应的task，然后外层赋值。

* 这里可以看出来`AFImageDownloaderMergedTask`类的作用：
	1. 当请求地址是同一个的时候，并且第一个请求正在进行中，后续就不再向服务器发送请求。这也是AFNetwork比较优化的一点，主要用来多张不同的imageView同时请求一个URL地址的情况。跟上篇那个同一张图片多次请求一个URL地址的情况比较一下（第6点）。
	2. 不同请求对应的成功失败块也没有统一处理，根据下载任务对应的唯一标识`receiptID`和成功失败块，生成`AFImageDownloaderResponseHandler`，来进行分别处理。
	3. 而合并任务的task就是外层需要的task，即创建`AFImageDownloadReceipt`的task

* 如果是首次请求，后面肯定会有对应的赋值操作，往下看

```
AFImageDownloaderMergedTask *existingMergedTask = self.mergedTasks[URLIdentifier];
        
   if (existingMergedTask != nil) {
       // AFImageDownloaderResponseHandler 用来处理返回结果的类
       
       AFImageDownloaderResponseHandler *handler = [[AFImageDownloaderResponseHandler alloc] initWithUUID:receiptID success:success failure:failure];
       [existingMergedTask addResponseHandler:handler];
       task = existingMergedTask.task;
       return;
   }
```

9、 根据缓存策略在缓存中是否进行第二次的查找

```
switch (request.cachePolicy) {
  // 对特定的 URL 请求使用网络协议中实现的缓存逻辑。这是默认的策略
  case NSURLRequestUseProtocolCachePolicy:
  // 无论缓存是否过期，先使用本地缓存数据。如果缓存中没有请求所对应的数据，那么从原始地址加载数据
  case NSURLRequestReturnCacheDataElseLoad:
  // 无论缓存是否过期，先使用本地缓存数据。如果缓存中没有请求所对应的数据，那么放弃从原始地址加载数据，请求视为失败（即：“离线”模式）
  case NSURLRequestReturnCacheDataDontLoad: {
      UIImage *cachedImage = [self.imageCache imageforRequest:request withAdditionalIdentifier:nil];
      if (cachedImage != nil) {
          if (success) {
              dispatch_async(dispatch_get_main_queue(), ^{
                  success(request, nil, cachedImage);
              });
          }
          return;
      }
      break;
  }
  default:
      break;
}
```

10、创建网络下载任务，进行图片下载，并处理返回结果
```
// 合并任务的唯一标识，
NSUUID *mergedTaskIdentifier = [NSUUID UUID];
// 声明一个数据请求任务
NSURLSessionDataTask *createdTask;
__weak __typeof__(self) weakSelf = self;
// 返回数据请求任务
createdTask = [self.sessionManager
             dataTaskWithRequest:request
             uploadProgress:nil
             downloadProgress:nil
             completionHandler:^(NSURLResponse * _Nonnull response, id  _Nullable responseObject, NSError * _Nullable error) { 
             // 数据下载完成后的处理
             }];
```

11、对应第8点中的取值操作，这里进行赋值操作。注意一个关键字`mergedTaskIdentifier`，是合并任务的唯一标识

```
AFImageDownloaderResponseHandler *handler = [[AFImageDownloaderResponseHandler alloc] initWithUUID:receiptID success:success failure:failure];
        
AFImageDownloaderMergedTask *mergedTask = [[AFImageDownloaderMergedTask alloc] initWithURLIdentifier:URLIdentifier identifier:mergedTaskIdentifier task:createdTask];
   
[mergedTask addResponseHandler:handler];
self.mergedTasks[URLIdentifier] = mergedTask;

```

12、当前任务创建已经完成，根据线程中正在进行的请求数量来决定，是进行下一个任务，还是等待。

```
if ([self isActiveRequestCountBelowMaximumLimit]) {
  [self startMergedTask:mergedTask];
} else {
  [self enqueueMergedTask:mergedTask];
}
// 最后数据请求任务赋值
task = mergedTask.task;
```

13、下面线深入12中的方法，最后再啃硬骨头10中的方法
```
// 正在进行的请求是否已经达到最大值（之前默认为4）
- (BOOL)isActiveRequestCountBelowMaximumLimit {
    return self.activeRequestCount < self.maximumActiveDownloads;
}

// 如果没有，任务就开始发起请求，并且更新当前请求数量的值
- (void)startMergedTask:(AFImageDownloaderMergedTask *)mergedTask {
    [mergedTask.task resume];
    ++self.activeRequestCount;
}

// 暂时挂起数据任务，然后按顺序放入数组中，直到有空余的线程来处理数据请求任务
- (void)enqueueMergedTask:(AFImageDownloaderMergedTask *)mergedTask {
    switch (self.downloadPrioritizaton) {
        case AFImageDownloadPrioritizationFIFO:// 先进先出，放入数组后面
            [self.queuedMergedTasks addObject:mergedTask];
            break;
        case AFImageDownloadPrioritizationLIFO:// 后进先出，放入数组的第一个，优先处理
            [self.queuedMergedTasks insertObject:mergedTask atIndex:0];
            break;
    }
}
```

14、最后来处理当图片下载任务（即网络请求任务）已经完成，之后的操作

1. 首先请求结果的处理都是在异步线程中执行的，避免线程阻塞，一直在等待网络请求完成 
```
dispatch_async(self.responseQueue, ^{ });
```

2. 针对第8点，第11点，所有针对返回结果的操作都是跟`AFImageDownloaderMergedTask`绑定的，所以找到对应于url地址的合并任务，并且根据合并任务的唯一标识判断是否同一个，
```
AFImageDownloaderMergedTask *mergedTask = self.mergedTasks[URLIdentifier];
                               
	if ([mergedTask.identifier isEqual:mergedTaskIdentifier]) {
}
```

3. 跟之前从字典获取的其实是同一个合并任务，这句话主要用来任务完成了，在字典中移除键值对
```
mergedTask = [strongSelf safelyRemoveMergedTaskWithURLIdentifier:URLIdentifier];
```

4. 遍历同一个url对应的所有的失败块，都失败了，回调失败信息
```
// 失败处理
if (error) {

         for (AFImageDownloaderResponseHandler *handler in mergedTask.responseHandlers) {
             if (handler.failureBlock) {
                 dispatch_async(dispatch_get_main_queue(), ^{
                     handler.failureBlock(request, (NSHTTPURLResponse*)response, error);
                 });
             }
         }
     }
```

5. 成功情况下，先进行图片缓存，再对所有的成功块，进行成功信息回调
```
[strongSelf.imageCache addImage:responseObject forRequest:request withAdditionalIdentifier:nil];

	for (AFImageDownloaderResponseHandler *handler in mergedTask.responseHandlers) {
	   // 成功回调处理
	   if (handler.successBlock) {
	       dispatch_async(dispatch_get_main_queue(), ^{
	           handler.successBlock(request, (NSHTTPURLResponse*)response, responseObject);
	       });
	   }
	}
```

6. 最后，请求结束了，当前执行的请求数量-1，
```
- (void)safelyDecrementActiveTaskCount {
	    dispatch_sync(self.synchronizationQueue, ^{
	        if (self.activeRequestCount > 0) {
	            self.activeRequestCount -= 1;
	        }
	    });
}
```
7. 如果还有需要执行的线程，就启动下一个线程
```
- (void)safelyStartNextTaskIfNecessary {
	    dispatch_sync(self.synchronizationQueue, ^{
	        if ([self isActiveRequestCountBelowMaximumLimit]) {
	            while (self.queuedMergedTasks.count > 0) {
	                AFImageDownloaderMergedTask *mergedTask = [self dequeueMergedTask];
	                if (mergedTask.task.state == NSURLSessionTaskStateSuspended) {
	                    [self startMergedTask:mergedTask];
	                    break;
	                }
	            }
	        }
	    });
}
```

---

目前这就是AFNetwork中加载图片的所有流程，当然数据请求还没有涉及，想研究这个入口的时候再进行深入
```
- (NSURLSessionDataTask *)POST:(NSString *)URLString
                    parameters:(id)parameters
                      progress:(void (^)(NSProgress * _Nonnull))uploadProgress
                       success:(void (^)(NSURLSessionDataTask * _Nonnull, id _Nullable))success
                       failure:(void (^)(NSURLSessionDataTask * _Nullable, NSError * _Nonnull))failure
```

## 总结：

***1. AFNetwork把加载图片的任务都放入一个字典中，然后把任务对应的网络请求放入数组中，然后按顺序执行***

***2. 在图片加载过程中，除了在针对请求结果处理的时候是异步进行的，其他全是同步进行的***

***3. 一家之言，请多指教***




