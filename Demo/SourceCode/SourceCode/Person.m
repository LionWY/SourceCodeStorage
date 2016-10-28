//
//  Person.m
//  SourceCode
//
//  Created by FOODING on 16/10/1342.
//  Copyright © 2016年 FOODING. All rights reserved.
//

#import "Person.h"
#import "SuperClass.h"
#import "Boy.h"

@implementation Person
+ (void)load {
    
//    Person * p = [[self alloc] init];
    
//    NSLog(@"=====person load");
    
//    People *p = [[People alloc] init];
//    [p run];
    
//    Boy * b = [[Boy alloc] init];
    
}

- (void)setName:(NSString *)name
{
    NSLog(@"====%@", name);
}

- (NSString *)name {
    return _name;
}


@end
