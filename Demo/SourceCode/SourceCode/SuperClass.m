//
//  SuperClass.m
//  SourceCode
//
//  Created by FOODING on 16/10/1342.
//  Copyright © 2016年 FOODING. All rights reserved.
//





#import "SuperClass.h"
#import "Person.h"

@implementation SuperClass

+ (void)load
{
    
}

+ (NSString *)test
{
    
    NSLog(@"test");
    return @"test";
}

+ (void)initialize
{
    NSLog(@"SuperClass initialize====%@", [self class]);
}

@end

@implementation ChildClass


+ (void)initialize
{
    NSLog(@"ChildClass initialize====%@", [self class]);
    
    People *p = [[People alloc] init];
    
    [p run];
}




@end

@implementation People

- (void)run {
    NSLog(@"People run===%@", [self class]);
    
   
}

+ (void)load
{
//    NSLog(@"People load====%@", [self class]);
    
}

+ (void)initialize
{
    NSLog(@"People initialize====%@", [self class]);
    
    Person *p = [[Person alloc] init];
    p.name = @"person";
}



@end
