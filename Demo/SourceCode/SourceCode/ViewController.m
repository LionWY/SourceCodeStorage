//
//  ViewController.m
//  SourceCode
//
//  Created by FOODING on 16/10/1242.
//  Copyright © 2016年 FOODING. All rights reserved.
//

#import "ViewController.h"
#import "SDWebImageManager.h"
#import "UIImageView+WebCache.h"
#import "UIView+WebCache.h"
//#import "UIImageView+AFNetworking.h"

@interface ViewController ()

@property (weak, nonatomic) IBOutlet UIImageView *imgView1;
@property (weak, nonatomic) IBOutlet UIImageView *imgView2;

@property (weak, nonatomic) IBOutlet UIImageView *imgView3;

@end

@implementation ViewController
- (IBAction)btn1Click:(id)sender {
    [_imgView2 sd_setImageWithURL:[NSURL URLWithString:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg"] placeholderImage:nil options:SDWebImageCacheMemoryOnly];
    
    
}
- (IBAction)btn2Click:(id)sender {
    [_imgView3 sd_setImageWithURL:[NSURL URLWithString:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg"]];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
//    NSAssert(NO, @"NO");
    
    
//    SDWebImageDownloaderOptions downloaderOptions = 0;
//    
//    downloaderOptions = SDWebImageDownloaderLowPriority | downloaderOptions;
//    
//    downloaderOptions = SDWebImageDownloaderProgressiveDownload | downloaderOptions;
//    
//    downloaderOptions |= SDWebImageDownloaderUseNSURLCache;
    
    
//    downloaderOptions &= SDWebImageDownloaderProgressiveDownload;
//    downloaderOptions &= ~SDWebImageDownloaderLowPriority;
    
//    [_imgView1 sd_setImageWithURL:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg" completed:nil];
    
//    [[SDImageCache sharedImageCache] queryCacheOperationForKey:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg" done:^(UIImage * _Nullable image, NSData * _Nullable data, SDImageCacheType cacheType) {
//        
//        NSLog(@"done");
//    }];
    
//    [_imgView1 sd_setImageWithURL:[NSURL URLWithString:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg"] ];
//    [_imgView1 sd_setImageWithURL:[NSURL URLWithString:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg"]];
//    [_imgView1 sd_setImageWithURL:[NSURL URLWithString:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg"]];
//    
//    [_imgView2 setImageWithURL:[NSURL URLWithString:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg"]];
//    
    
    

    
    
    /*** 
    UIImageView *imgView = [[UIImageView alloc] initWithFrame:CGRectMake(100, 100, 200, 300)];
    
    
    [imgView sd_internalSetImageWithURL:[NSURL URLWithString:@"http://oeb4c30x3.bkt.clouddn.com/door_two.jpg"] placeholderImage:[UIImage imageNamed:@"8.jpg"] options:SDWebImageRetryFailed operationKey:nil setImageBlock:^(UIImage * _Nullable image, NSData * _Nullable imageData) {
        
        NSLog(@"image:%@\r\n", image);
        
    } progress:^(NSInteger receivedSize, NSInteger expectedSize, NSURL * _Nullable targetURL) {
        
        NSLog(@"====receivedSize:%ld\r\n====expectedSize:%ld\r\n====targetURL:%@", (long)receivedSize, (long)expectedSize, targetURL);
        
    } completed:^(UIImage * _Nullable image, NSError * _Nullable error, SDImageCacheType cacheType, NSURL * _Nullable imageURL) {
        NSLog(@"---------image:%@\r\n---------error:%@\r\n---------cacheType:%ld\r\n---------imageURL:%@", image, error, (long)cacheType, imageURL);
    }];
    
    [self.view addSubview:imgView];
     
     **/
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
