//
//  AppDelegate.m
//  RunLoopTest
//
//  Created by FOODING on 16/11/145.
//  Copyright © 2016年 FOODING. All rights reserved.
//

#import "AppDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate
- (void)doFireTimer:(NSTimer *)timer
{
    NSLog(@"=====%@", timer);
}

- (void)threadMain

{
    
    // The application uses garbage collection, so no autorelease pool is needed.
    
    NSRunLoop* myRunLoop = [NSRunLoop currentRunLoop];
    
    
    
    // Create a run loop observer and attach it to the run loop.
    
    CFRunLoopObserverContext  context = {0, (__bridge void *)(self), NULL, NULL, NULL};
    
    // CF_EXPORT CFRunLoopObserverRef CFRunLoopObserverCreate(CFAllocatorRef allocator, CFOptionFlags activities, Boolean repeats, CFIndex order, CFRunLoopObserverCallBack callout, CFRunLoopObserverContext *context);

    
    CFRunLoopObserverRef observer = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopAllActivities, YES, 0, NULL, &context);
    

    
    
    
    if (observer)
        
    {
        
        CFRunLoopRef    cfLoop = [myRunLoop getCFRunLoop];
        
        CFRunLoopAddObserver(cfLoop, observer, kCFRunLoopDefaultMode);
        
    }
    
    
    
    // Create and schedule the timer.
    
    [NSTimer scheduledTimerWithTimeInterval:0.1 target:self
     
                                   selector:@selector(doFireTimer:) userInfo:nil repeats:YES];
    
    
    
    NSInteger    loopCount = 10;
    
    do
        
    {
        
        // Run the run loop 10 times to let the timer fire.
        
        [myRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1]];
        
        loopCount--;
        
    }
    
    while (loopCount);
    
}


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    
    

   
    
    [self threadMain];
    
    
    
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
