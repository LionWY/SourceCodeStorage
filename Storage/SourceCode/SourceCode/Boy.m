//
//  Boy.m
//  SourceCode
//
//  Created by FOODING on 16/10/1342.
//  Copyright © 2016年 FOODING. All rights reserved.
//

#import "Boy.h"


@interface Girl : NSObject

@end

@implementation Girl

+ (void)load {
    
//    Boy *b = [Boy new];
    
//    [b doSomething];
    
//    [Boy printSex];
}

@end

@interface Boy ()

@property (nonatomic, strong) NSString *sex;

@end

static NSString *boySex;

@implementation Boy

+ (void)load {
    
    boySex = @"boy";
}

+ (void)printSex {
    
    NSLog(@"sex:%@", boySex);

}

- (void)doSomething {
    NSLog(@"boy do something");
}

@end




