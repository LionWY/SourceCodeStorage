//
//  AppDelegate.m
//  SourceCode
//
//  Created by FOODING on 16/10/1242.
//  Copyright © 2016年 FOODING. All rights reserved.
//

#import "AppDelegate.h"
#import "AFNetworking.h"
#import "Person.h"
#import "AFNetworkActivityIndicatorManager.h"
#import "SuperClass.h"




@interface AppDelegate ()

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    
    
    
    
    
//     
//    SuperClass *s = [[SuperClass alloc] init];
//    
//    ChildClass *c = [[ChildClass alloc] init];
//    
//    
//   BOOL a = [s isKindOfClass:[SuperClass class]];
//    
//   a = [c isKindOfClass:[SuperClass class]];
//    
//    a = [c isKindOfClass:[ChildClass class]];
//    
//    a = [s isMemberOfClass:[SuperClass class]];
//    
//    a = [c isMemberOfClass:[SuperClass class]];
//    
//    a = [c isMemberOfClass:[ChildClass class]];
    
    
    
    
    
    
    
    /*
    NSArray *arr = @[@1, @2, @3, @4, @5];
    dispatch_queue_t queue = dispatch_get_global_queue(0, 0);
    
    dispatch_async(queue, ^{
       
        dispatch_apply([arr count], queue, ^(size_t index) {
            
            NSLog(@"===%zu===%@", index, [arr objectAtIndex:index]);
            
        });
        
        
    });
    
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"done");        
    });
    
    */
    
    
    
    
    
//    dispatch_apply(10, <#dispatch_queue_t  _Nonnull queue#>, ^(size_t i) {
//        <#code#>
//    })
    
//    dispatch_group_wait(<#dispatch_group_t  _Nonnull group#>, <#dispatch_time_t timeout#>)
    
//    dispatch_time(<#dispatch_time_t when#>, <#int64_t delta#>)
//    
//    dispatch_walltime(<#const struct timespec * _Nullable when#>, <#int64_t delta#>)
//    
//    
//    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2ull * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
//        
//    });
    
//    dispatch_queue_t queue1 = dispatch_queue_create("123", NULL);
    
//    dispatch_get_global_queue(<#long identifier#>, <#unsigned long flags#>)
    
    // Override point for customization after application launch.
    
    /*** 添加缓存
     
    [AFNetworkActivityIndicatorManager sharedManager].enabled = YES;
    //网络请求时状态栏网络状态小转轮
    
    NSURLCache *URLCache = [[NSURLCache alloc] initWithMemoryCapacity:4 * 1024 * 1024
                                                         diskCapacity:20 * 1024 * 1024
                                                             diskPath:nil];
    //内存4M，硬盘20M
    [NSURLCache setSharedURLCache:URLCache];

    
    */
    
    
    
    
    return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    
    
    
    
    
    
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}


- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}


@end
