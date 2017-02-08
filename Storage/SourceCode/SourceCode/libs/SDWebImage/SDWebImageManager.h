/*
 * This file is part of the SDWebImage package.
 * (c) Olivier Poitrey <rs@dailymotion.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#import "SDWebImageCompat.h"
#import "SDWebImageOperation.h"
#import "SDWebImageDownloader.h"
#import "SDImageCache.h"

typedef NS_OPTIONS(NSUInteger, SDWebImageOptions) {
    /**
     * By default, when a URL fail to be downloaded, the URL is blacklisted so the library won't keep trying.
     * This flag disable this blacklisting.
     
     // blacklisting 黑名单
     // 默认情况下，当一个URL下载失败，URL就会被放入黑名单，library就不会尝试继续下载，这个标志就是阻止放入黑名单的行为
     // 当你选择了这个参数，系统会一直下载图片，直至下载完成
     */
    SDWebImageRetryFailed = 1 << 0,

    /**
     * By default, image downloads are started during UI interactions, this flags disable this feature,
     * leading to delayed download on UIScrollView deceleration for instance.
     
     // interactions 交互， deceleration 减速， 
     // 默认情况下，系统在UI交互（比如滑动scrollview）的过程中，也会进行图片下载，这个标志就是阻止这个行为的
     // 选择这个参数，在scrollview 滑动过程中，系统不会进行下载图片，会暂停，直到滑动减速的时候（即手离开scrollview），优先级比较低
     */
    SDWebImageLowPriority = 1 << 1,

    /**
     * This flag disables on-disk caching
     
     // 字面意思，只在内存中缓存，不会缓存在硬盘里
     */
    SDWebImageCacheMemoryOnly = 1 << 2,

    /**
     * This flag enables progressive download, the image is displayed progressively during download as a browser would do.
     * By default, the image is only displayed once completely downloaded.
     
     // 默认情况下，图片只会在下载完成后进行显示
     // 选择这个参数，就能够使图片像浏览器一样，图片下载一部分，就显示一部分，一节一节的显示
     */
    SDWebImageProgressiveDownload = 1 << 3,

    /**
     * Even if the image is cached, respect the HTTP response cache control, and refresh the image from remote location if needed.
     * The disk caching will be handled by NSURLCache instead of SDWebImage leading to slight performance degradation.
     * This option helps deal with images changing behind the same request URL, e.g. Facebook graph api profile pics.
     * If a cached image is refreshed, the completion block is called once with the cached image and again with the final image.
     *
     * Use this flag only if you can't make your URLs static with embedded cache busting parameter.
     
     
     
     // degradation 下降， embedded 深入的， busting 破坏的
     // 尽管图片已经缓存，但在某些必要情况下，出于对HTTP的响应缓存控制的尊重，也要从本地再次刷新图片
     // 由NSURLCache代替SDWebImage来控制硬盘缓存，会导致轻微的性能退化
     // 选择这个参数，会有助于处理同一个请求后图片的更改（即图片改变了，但请求没变）比如，脸谱网图形的应用程序的配置文件
     // 只有当你不能通过嵌入式缓存和破坏参数来保证url是静态的，才需要用这个参数
     // 找到另一种解释 是正常情况下 只会根据url做key去判断图片是否加载过 但是 有些网页是动态网站 图片的url就会变 但是文件的签名 如 e-tag （http header里面）是用来判断图片是否是同一个 默认sd 没有看http header 如果加上这个标记 就会访问http header .
     */
    SDWebImageRefreshCached = 1 << 4,

    /**
     * In iOS 4+, continue the download of the image if the app goes to background. This is achieved by asking the system for
     * extra time in background to let the request finish. If the background task expires the operation will be cancelled.
     
     // 据了解，在iOS4以后，iOS才有了后台的概念，系统在进入后台后，如果有后台任务的话，会有一定的时间（好像是3-10分钟）来执行后台任务。（具体可以了解喵大大的这篇文章（https://onevcat.com/2013/08/ios7-background-multitask/））
     // 这个参数能够让应用程序进入后台后继续下载图片，就是通过向系统请求额外的后台执行时间来完成的，但是，一旦后台任务过期，操作就会被取消
     */
    SDWebImageContinueInBackground = 1 << 5,

    /**
     * Handles cookies stored in NSHTTPCookieStore by setting
     * NSMutableURLRequest.HTTPShouldHandleCookies = YES;
     
     // 通过设置一些属性，可以控制存储在NSHTTPCookieStore的cookies
     */
    SDWebImageHandleCookies = 1 << 6,

    /**
     * Enable to allow untrusted SSL certificates.
     * Useful for testing purposes. Use with caution in production.
     
     // 允许不安全的SSL证书， 在测试中有用，在正式产品中慎用
     */
    SDWebImageAllowInvalidSSLCertificates = 1 << 7,

    /**
     * By default, images are loaded in the order in which they were queued. This flag moves them to
     * the front of the queue.
     
     // 默认情况下，图片是在它们所处的线程中按顺序加载的，
     // 这个参数，能够把它们移动到线程的前面，优先加载
     */
    SDWebImageHighPriority = 1 << 8,
    
    /**
     * By default, placeholder images are loaded while the image is loading. This flag will delay the loading
     * of the placeholder image until after the image has finished loading.
     
     // 默认情况下，占位图会在图片加载过程中显示，
     // 这个参数，会延迟占位图的加载，直到图片下载完成，就是说，图片下载完成了，才会加载占位图，然后立马加载下载完成的图片
     */
    SDWebImageDelayPlaceholder = 1 << 9,

    /**
     * We usually don't call transformDownloadedImage delegate method on animated images,
     * as most transformation code would mangle it.
     * Use this flag to transform them anyway.
     
     // mangle 损坏
     // 一般情况下，对于动态图片，我们都不会调用transformDownloadedImage代理方法，因为可能会损坏图片
     // 这个参数会导致图片变换（transform）
     */
    SDWebImageTransformAnimatedImage = 1 << 10,
    
    /**
     * By default, image is added to the imageView after download. But in some cases, we want to
     * have the hand before setting the image (apply a filter or add it with cross-fade animation for instance)
     * Use this flag if you want to manually set the image in the completion when success
     
     // cross-fade  (声音或影像)同时淡出及淡入
     // 默认情况下，图片下载完成就会被添加到imageView上，但有一些特殊情况，我们想在设置图片前有些特殊效果，（应用一个过滤器或添加一些交叉淡出动画）
     // 图片下载成功后如果你想手动设置图片，可以使用这个参数
     */
    SDWebImageAvoidAutoSetImage = 1 << 11,
    
    /**
     * By default, images are decoded respecting their original size. On iOS, this flag will scale down the
     * images to a size compatible with the constrained memory of devices.
     * If `SDWebImageProgressiveDownload` flag is set the scale down is deactivated.
     
     // 默认情况下，图片应该按照它们原本的大小解码。在iOS上，这个参数会根据设置内存限制进行相对比例的减小图片大小
     // 如果设置了SDWebImageProgressiveDownload这个参数，缩减效果会失效
     */
    SDWebImageScaleDownLargeImages = 1 << 12
};

typedef void(^SDExternalCompletionBlock)(UIImage * _Nullable image, NSError * _Nullable error, SDImageCacheType cacheType, NSURL * _Nullable imageURL);

typedef void(^SDInternalCompletionBlock)(UIImage * _Nullable image, NSData * _Nullable data, NSError * _Nullable error, SDImageCacheType cacheType, BOOL finished, NSURL * _Nullable imageURL);

typedef NSString * _Nullable (^SDWebImageCacheKeyFilterBlock)(NSURL * _Nullable url);


@class SDWebImageManager;

@protocol SDWebImageManagerDelegate <NSObject>

@optional

/**
 // 在缓存中找不到的图片从网络请求下载
 * Controls which image should be downloaded when the image is not found in the cache.
 *
 * @param imageManager The current `SDWebImageManager`
 * @param imageURL     The url of the image to be downloaded
 *
 * @return Return NO to prevent the downloading of the image on cache misses. If not implemented, YES is implied.
 */
- (BOOL)imageManager:(nonnull SDWebImageManager *)imageManager shouldDownloadImageForURL:(nullable NSURL *)imageURL;

/**
 * Allows to transform the image immediately after it has been downloaded and just before to cache it on disk and memory.
 * NOTE: This method is called from a global queue in order to not to block the main thread.
 *
 * @param imageManager The current `SDWebImageManager`
 * @param image        The image to transform
 * @param imageURL     The url of the image to transform
 *
 * @return The transformed image object.
 */
- (nullable UIImage *)imageManager:(nonnull SDWebImageManager *)imageManager transformDownloadedImage:(nullable UIImage *)image withURL:(nullable NSURL *)imageURL;

@end

/**
 * The SDWebImageManager is the class behind the UIImageView+WebCache category and likes.
 * It ties the asynchronous downloader (SDWebImageDownloader) with the image cache store (SDImageCache).
 * You can use this class directly to benefit from web image downloading with caching in another context than
 * a UIView.
 *
 * Here is a simple example of how to use SDWebImageManager:
 *
 * @code

SDWebImageManager *manager = [SDWebImageManager sharedManager];
[manager loadImageWithURL:imageURL
                  options:0
                 progress:nil
                completed:^(UIImage *image, NSError *error, SDImageCacheType cacheType, BOOL finished, NSURL *imageURL) {
                    if (image) {
                        // do something with image
                    }
                }];

 * @endcode
 */
@interface SDWebImageManager : NSObject

@property (weak, nonatomic, nullable) id <SDWebImageManagerDelegate> delegate;

@property (strong, nonatomic, readonly, nullable) SDImageCache *imageCache;
@property (strong, nonatomic, readonly, nullable) SDWebImageDownloader *imageDownloader;

/**
 * The cache filter is a block used each time SDWebImageManager need to convert an URL into a cache key. This can
 * be used to remove dynamic part of an image URL.
 *
 * The following example sets a filter in the application delegate that will remove any query-string from the
 * URL before to use it as a cache key:
 *
 * @code

[[SDWebImageManager sharedManager] setCacheKeyFilter:^(NSURL *url) {
    url = [[NSURL alloc] initWithScheme:url.scheme host:url.host path:url.path];
    return [url absoluteString];
}];

 * @endcode
 */
@property (nonatomic, copy, nullable) SDWebImageCacheKeyFilterBlock cacheKeyFilter;

/**
 * Returns global SDWebImageManager instance.
 *
 * @return SDWebImageManager shared instance
 */
+ (nonnull instancetype)sharedManager;

/**
 * Allows to specify instance of cache and image downloader used with image manager.
 * @return new instance of `SDWebImageManager` with specified cache and downloader.
 */
- (nonnull instancetype)initWithCache:(nonnull SDImageCache *)cache downloader:(nonnull SDWebImageDownloader *)downloader NS_DESIGNATED_INITIALIZER;

/**
 // 如果缓存中不存在该图片，那么就根据给定的URL下载图片，否则就返回缓存的图片
 * Downloads the image at the given URL if not present in cache or return the cached version otherwise.
 * 
 * @param url            The URL to the image
 * @param options        A mask to specify options to use for this request
 * @param progressBlock  A block called while image is downloading
 *                       @note the progress block is executed on a background queue
 // 图片加载完成后调用这个block，这个是内部的block，比外部的block多一个data参数
 
 typedef void(^SDInternalCompletionBlock)(UIImage * _Nullable image, NSData * _Nullable data, NSError * _Nullable error, SDImageCacheType cacheType, BOOL finished, NSURL * _Nullable imageURL);
 
 * @param completedBlock A block called when operation has been completed. 
 *
 *   This parameter is required.
 * 
 *   This block has no return value and takes the requested UIImage as first parameter and the NSData representation as second parameter.
 *   In case of error the image parameter is nil and the third parameter may contain an NSError.
 *
 *   The forth parameter is an `SDImageCacheType` enum indicating if the image was retrieved from the local cache
 *   or from the memory cache or from the network.
 *
 *   The fith parameter is set to NO when the SDWebImageProgressiveDownload option is used and the image is
 *   downloading. This block is thus called repeatedly with a partial image. When image is fully downloaded, the
 *   block is called a last time with the full image and the last parameter set to YES.
 *
 *   The last parameter is the original image URL
 *
 * @return Returns an NSObject conforming to SDWebImageOperation. Should be an instance of SDWebImageDownloaderOperation
 */
- (nullable id <SDWebImageOperation>)loadImageWithURL:(nullable NSURL *)url
                                              options:(SDWebImageOptions)options
                                             progress:(nullable SDWebImageDownloaderProgressBlock)progressBlock
                                            completed:(nullable SDInternalCompletionBlock)completedBlock;

/**
 * Saves image to cache for given URL
 *
 * @param image The image to cache
 * @param url   The URL to the image
 *
 */

- (void)saveImageToCache:(nullable UIImage *)image forURL:(nullable NSURL *)url;

/**
 * Cancel all current operations
 */
- (void)cancelAll;

/**
 * Check one or more operations running
 */
- (BOOL)isRunning;

/**
 *  Async check if image has already been cached
 *
 *  @param url              image url
 *  @param completionBlock  the block to be executed when the check is finished
 *  
 *  @note the completion block is always executed on the main queue
 */
- (void)cachedImageExistsForURL:(nullable NSURL *)url
                     completion:(nullable SDWebImageCheckCacheCompletionBlock)completionBlock;

/**
 *  Async check if image has already been cached on disk only
 *
 *  @param url              image url
 *  @param completionBlock  the block to be executed when the check is finished
 *
 *  @note the completion block is always executed on the main queue
 */
- (void)diskImageExistsForURL:(nullable NSURL *)url
                   completion:(nullable SDWebImageCheckCacheCompletionBlock)completionBlock;


/**
 *Return the cache key for a given URL
 */
- (nullable NSString *)cacheKeyForURL:(nullable NSURL *)url;

@end
