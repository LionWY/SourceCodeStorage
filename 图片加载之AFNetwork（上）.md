## UIImageView+AFNetworking
> This category adds methods to the UIKit framework’s UIImageView class. The methods in this category provide support for loading remote images asynchronously from a URL.

给UIImageView添加分类方法，通过一个URL异步加载远程图片

![research.jpg](http://oeb4c30x3.bkt.clouddn.com/research.jpg)

1. 核心方法就是下面很简单的方法：
```
- (void)setImageWithURL:(NSURL *)url;
```

2. 内部实现，创建图片请求，并在请求头添加参数,后续的所有操作都是跟request有关
```
- (void)setImageWithURL:(NSURL *)url
       placeholderImage:(UIImage *)placeholderImage
{
	    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
	    [request addValue:@"image/*" forHTTPHeaderField:@"Accept"];
	
	    [self setImageWithURLRequest:request placeholderImage:placeholderImage success:nil failure:nil];
}
```

3. 上面两个方法最终调用的都是下面的方法，下面就针对最核心的方法，一步步分析它的具体实现
```
- (void)setImageWithURLRequest:(NSURLRequest *)urlRequest
              placeholderImage:(nullable UIImage *)placeholderImage
                       success:(nullable void (^)(NSURLRequest *request, NSHTTPURLResponse * _Nullable response, UIImage *image))success
                       failure:(nullable void (^)(NSURLRequest *request, NSHTTPURLResponse * _Nullable response, NSError *error))failure;
```
4. 在任何请求发生之前，都要进行URLRequest的判断。如果URL为空，就取消图片下载任务，并直接设置占位图
```
if ([urlRequest URL] == nil) {
   
   // 对于当前的这个任务，取消所有正在执行的图片下载操作，并把下载回执置空
   [self cancelImageDownloadTask];
   
   self.image = placeholderImage;
   return;
}
```

5. 上面方法里面涉及到两个类`AFImageDownloadReceipt` `AFImageDownloader`
	* 首先需要明白一个规则：图片下载任务的取消不是由任务自己取消，而是通过“下载回执”取消
	
	* AFImageDownloader：用来处理图片下载的类，所有下载任务都由它处理

	* AFImageDownloadReceipt：“下载回执”，跟AFImageDownloader一一对应，主要是用来取消AFImageDownloader正在运行的任务，只有两个属性：
```
@property (nonatomic, strong) NSURLSessionDataTask *task;// 下载任务，即AFImageDownloader执行的任务
@property (nonatomic, strong) NSUUID *receiptID;//任务的唯一标识，用来区分两个任务是否相同
```

6. 根据`URLRequest`来判断两次请求是否同一个，即，阻止同一张图片进行多次相同的请求，优化请求。
```
if ([self isActiveTaskURLEqualToURLRequest:urlRequest]){
        return;
    }
```

7. `AFImageRequestCache`协议，是用来添加、删除、访问图片。这里把下载器存储图片的对象赋值给一个支持该协议的对象，用来获取缓存图片
```
id <AFImageRequestCache> imageCache = downloader.imageCache;
```

8. 通过URLrequest在缓存中查找图片，如果能找到，就返回缓存图片，操作结束。这里缓存见后者
```
UIImage *cachedImage = [imageCache imageforRequest:urlRequest withAdditionalIdentifier:nil];
if (cachedImage) {
   // 如果需要返回结果，就进行回调，在回调block中进行手动设置imageView.image
   if (success) {
       success(urlRequest, nil, cachedImage);
   } else {
       self.image = cachedImage;
   }
   // 操作已经完成，把下载回执置空，
   [self clearActiveDownloadInformation];   
}
```

9. 如果有占位图，就先设置占位图。`NSUUID`是用来创建唯一标识的，每次调用`UUID`返回结果都不一样，对应于下载回执的任务标识`receiptID`
```
if (placeholderImage) {
       self.image = placeholderImage;
   }
   // 弱引用，防止保留环
   __weak __typeof(self)weakSelf = self;
   
   NSUUID *downloadID = [NSUUID UUID];    
```
10. 根据`URLrequest`和`receiptID`进行图片下载，返回的是对应下载操作的回执`AFImageDownloadReceipt`，具体实现下一篇马上呈现
```
- (nullable AFImageDownloadReceipt *)downloadImageForURLRequest:(NSURLRequest *)request
                                                 withReceiptID:(NSUUID *)receiptID
                                                        success:(nullable void (^)(NSURLRequest *request, NSHTTPURLResponse  * _Nullable response, UIImage *responseObject))success
                                                        failure:(nullable void (^)(NSURLRequest *request, NSHTTPURLResponse * _Nullable response, NSError *error))failure;
```

11. 获取图片成功的情况下：
```
// 首先在block中，强引用下，避免在运行过程中，self被自动释放
   __strong __typeof(weakSelf)strongSelf = weakSelf;
   // 根据任务标识判断，避免返回结果对应的请求不是之前的那个请求
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
```
12. 失败情况下，返回错误信息，并清空下载信息
```
__strong __typeof(weakSelf)strongSelf = weakSelf;
if ([strongSelf.af_activeImageDownloadReceipt.receiptID isEqual:downloadID]) {
   if (failure) {
       failure(request, response, error);
   }
   [strongSelf clearActiveDownloadInformation];
}
```

13. 把返回的下载回执，赋值给图片对应的下载回执，对应最开始的第6点，也就是说，当一张图片正在进行下载操作，如果再进行一次相同的请求，那么第二次请求直接返回，继续执行第一次的请求，直到请求结束，然后赋值。
```
self.af_activeImageDownloadReceipt = receipt;
```


## AFAutoPurgingImageCache
> The AutoPurgingImageCache in an in-memory image cache used to store images up to a given memory capacity. When the memory capacity is reached, the image cache is sorted by last access date, then the oldest image is continuously purged until the preferred memory usage after purge is met. Each time an image is accessed through the cache, the internal access date of the image is updated.

这个类是用来在内存中进行图片缓存操作的，并且会根据图片使用时间排序，当内存快满的时候，先释放最久未使用的图片，然后再清除优先使用内存里面的图片。每次使用图片后，图片使用时间都会更新。
这个类遵循`AFImageRequestCache`协议，因此可以使用协议方法

1. 内存分类：默认内存100M，优先使用内存60M。并且使用`cachedImages`来存储所有的图片对象。
```
@property (nonatomic, assign) UInt64 memoryCapacity;//总内存
@property (nonatomic, assign) UInt64 preferredMemoryUsageAfterPurge;//优先使用内存
@property (nonatomic, assign, readonly) UInt64 memoryUsage;//内存已使用容量
@property (nonatomic, strong) NSMutableDictionary <NSString* , AFCachedImage*> *cachedImages;//可变字典来存储图片对象
@property (nonatomic, assign) UInt64 currentMemoryUsage;//当前内存使用情况，跟memoryUsage一样的
@property (nonatomic, strong) dispatch_queue_t synchronizationQueue;//同步线程
```

2. 这里涉及到一个类`AFCachedImage`，用来存储图片以及图片信息的类
```
@property (nonatomic, strong) UIImage *image;//图片
@property (nonatomic, strong) NSString *identifier;//图片标识
@property (nonatomic, assign) UInt64 totalBytes;//图片大小
@property (nonatomic, strong) NSDate *lastAccessDate;//最新使用日期
@property (nonatomic, assign) UInt64 currentMemoryUsage;//当前内存已使用容量
```

3. 这里有个方法可以学习下，如何计算图片所占内存
```
// 根据屏幕进行等比例的缩减/扩大图片size
CGSize imageSize = CGSizeMake(image.size.width * image.scale, image.size.height * image.scale);
CGFloat bytesPerPixel = 4.0;
CGFloat bytesPerSize = imageSize.width * imageSize.height;
self.totalBytes = (UInt64)bytesPerPixel * (UInt64)bytesPerSize;
```

4. 回到上面那个协议方法，返回一个对应于`request`和`identifier`的图片
```
- (nullable UIImage *)imageforRequest:(NSURLRequest *)request withAdditionalIdentifier:(nullable NSString *)identifier;
```

5. 图片的Identifier，直接拼接url和additionalIdentifier
```
- (NSString *)imageCacheKeyFromURLRequest:(NSURLRequest *)request withAdditionalIdentifier:(NSString *)additionalIdentifier {
	    NSString *key = request.URL.absoluteString;
	    if (additionalIdentifier != nil)
	    {
	        key = [key stringByAppendingString:additionalIdentifier];
	    }
	    return key;
}
```

6. 根据图片的Identifier，在可变字典中获取需要的图片，并更新使用时间。
```
- (nullable UIImage *)imageWithIdentifier:(NSString *)identifier {
	    __block UIImage *image = nil;
	    // 同步操作，获取缓存图片
	    dispatch_sync(self.synchronizationQueue, ^{
	        // 用来存储图片信息的类
	        AFCachedImage *cachedImage = self.cachedImages[identifier];
	        image = [cachedImage accessImage];
	    });
	    
	    return image;
}
```
7. 内存中添加图片，使用栅栏保证线程安全，栅栏即，把之前所有的事情处理完毕之后，再进行栅栏中的处理，栅栏处理完成之后，再进行后续的处理。
```
    dispatch_barrier_async(self.synchronizationQueue, ^{
        // 根据图片和标识，生成图片信息类
        AFCachedImage *cacheImage = [[AFCachedImage alloc] initWithImage:image identifier:identifier];
        // 根据标识查找是否已经存在对应的图片
        AFCachedImage *previousCachedImage = self.cachedImages[identifier];
        // 如果已经存在，先减少图片所占内存
        if (previousCachedImage != nil) {
            self.currentMemoryUsage -= previousCachedImage.totalBytes;
        }
        
        // 再更新图片，和内存使用情况
        self.cachedImages[identifier] = cacheImage;
        self.currentMemoryUsage += cacheImage.totalBytes;
    });
```

8. 如果内存满了：
```
    dispatch_barrier_async(self.synchronizationQueue, ^{
        if (self.currentMemoryUsage > self.memoryCapacity) {
            // 1. 需要清除的缓存大小
            UInt64 bytesToPurge = self.currentMemoryUsage - self.preferredMemoryUsageAfterPurge;
            
            // 2. 先把可变字典里面所有的图片信息类放入数组，然后根据最新使用时间进行排序 
            NSMutableArray <AFCachedImage*> *sortedImages = [NSMutableArray arrayWithArray:self.cachedImages.allValues];
            NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"lastAccessDate" ascending:YES];
            [sortedImages sortUsingDescriptors:@[sortDescriptor]];

            // 3. 默认已经清除的缓存
            UInt64 bytesPurged = 0;

            // 4. 一张张图片进行清除，然后更新已经清除的缓存容量，直到符合要求
            for (AFCachedImage *cachedImage in sortedImages) {
                [self.cachedImages removeObjectForKey:cachedImage.identifier];
                bytesPurged += cachedImage.totalBytes;
                if (bytesPurged >= bytesToPurge) {
                    break ;
                }
            }
            // 5. 更新当前内存已使用容量
            self.currentMemoryUsage -= bytesPurged;
        }
    });
```


## 总结：

***1. AFNetwork中图片的缓存，自定义了一个对象`AFAutoPurgingImageCache`，在对象中声明了一个可变数组来进行图片的增删改查***

***2. AFAutoPurgingImageCache并没有磁盘缓存，也没有本地缓存，程序一旦重启，就需要重新进行缓存的处理***

***3. 源码阅读，要找到一个入口，静下心来看***

