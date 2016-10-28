/*
 * This file is part of the SDWebImage package.
 * (c) Olivier Poitrey <rs@dailymotion.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#import "SDWebImageCompat.h"

#if SD_UIKIT || SD_MAC

#import "SDWebImageManager.h"

typedef void(^SDSetImageBlock)(UIImage * _Nullable image, NSData * _Nullable imageData);

@interface UIView (WebCache)

/**
 * Get the current image URL.//获取当前image的url
 *
 * Note that because of the limitations of categories this property can get out of sync
 * if you use setImage: directly.
 */
- (nullable NSURL *)sd_imageURL;

/**
 // 通过url对imageView进行设置image，并且可以选择性的添加占位图
 * Set the imageView `image` with an `url` and optionally a placeholder image.
 * 
 * The download is asynchronous and cached.// 下载服务是异步并且可以缓存的
 *
 * @param url            The url for the image.
 * @param placeholder    The image to be set initially, until the image request finishes.
 * @param options        The options to use when downloading the image. @see SDWebImageOptions for the possible values.
 * @param operationKey   A string to be used as the operation key. If nil, will use the class name
 * @param setImageBlock  Block used for custom set image code
 * @param progressBlock  A block called while image is downloading
 *                       @note the progress block is executed on a background queue
 * @param completedBlock A block called when operation has been completed. This block has no return value
 *                       and takes the requested UIImage as first parameter. In case of error the image parameter
 *                       is nil and the second parameter may contain an NSError. The third parameter is a Boolean
 *                       indicating if the image was retrieved from the local cache or from the network.
 *                       The fourth parameter is the original image url.
 
 // receivedSize    接收到的数据大小，
 // expectedSize    图片总的数据大小
 // targetURL       图片的URL
 typedef void(^SDWebImageDownloaderProgressBlock)(NSInteger receivedSize, NSInteger expectedSize, NSURL * _Nullable targetURL);
 
 // image       下载完成之后的图片
 // error       下载过程中是否发生错误，即错误信息
 // cacheType   枚举值，指明图片是以那种方式（网络、磁盘、内存）获取的
 // imageURL    图片的URL
 typedef void(^SDExternalCompletionBlock)(UIImage * _Nullable image, NSError * _Nullable error, SDImageCacheType cacheType, NSURL * _Nullable imageURL);

 // retrieved 恢复，找回
 */
- (void)sd_internalSetImageWithURL:(nullable NSURL *)url
                  placeholderImage:(nullable UIImage *)placeholder
                           options:(SDWebImageOptions)options
                      operationKey:(nullable NSString *)operationKey
                     setImageBlock:(nullable SDSetImageBlock)setImageBlock // 在block中自定义设置image 
                          progress:(nullable SDWebImageDownloaderProgressBlock)progressBlock // 图片下载进度，
                         completed:(nullable SDExternalCompletionBlock)completedBlock; // 下载完成后再block中进行设置 

/**
 * Cancel the current download
 */
- (void)sd_cancelCurrentImageLoad;

#if SD_UIKIT

#pragma mark - Activity indicator

/**
 *  Show activity UIActivityIndicatorView
 */
- (void)sd_setShowActivityIndicatorView:(BOOL)show;

/**
 *  set desired UIActivityIndicatorViewStyle
 *
 *  @param style The style of the UIActivityIndicatorView
 */
- (void)sd_setIndicatorStyle:(UIActivityIndicatorViewStyle)style;

- (BOOL)sd_showActivityIndicatorView;
- (void)sd_addActivityIndicator;
- (void)sd_removeActivityIndicator;

#endif

@end

#endif
