
# 从源码上认识RunLoop

版本： CF-1151.16
[官方文档](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html#//apple_ref/doc/uid/10000057i-CH16-SW12)
[参考](http://aaaboom.com/?p=34#wow12)
[源码下载](http://opensource.apple.com/tarballs/CF/CF-1151.16.tar.gz)

***墙裂建议“仔细看源码，才能深入理解”***

为了看着简洁，只摘抄部分易懂有用的代码
源码详细介绍在[这里]()

## CFRunLoop
```
struct __CFRunLoop {
    CFRuntimeBase _base;
    pthread_mutex_t _lock;			/* locked for accessing mode list */
    __CFPort _wakeUpPort;			// used for CFRunLoopWakeUp 用来唤醒runLoop的端口
    Boolean _unused;
    volatile _per_run_data *_perRunData;              // reset for runs of the run loop
    pthread_t _pthread;
    uint32_t _winthread;
    
    CFMutableSetRef _commonModes;       // 集合，所有标记为common的mode的集合
    
    CFMutableSetRef _commonModeItems;   // 集合，commonMode 的item（observers/sources/timers）的集合
    
    CFRunLoopModeRef _currentMode;      // 当前mode
    CFMutableSetRef _modes;             // 集合，mode的集合
    
    struct _block_item *_blocks_head;
    struct _block_item *_blocks_tail;
    CFAbsoluteTime _runTime;
    CFAbsoluteTime _sleepTime;
    CFTypeRef _counterpart;
};
```













## 目的：
1. 管理线程，让线程在没有消息处理时休眠以避免资源占用、有消息时立刻被唤醒，进行消息处理

2. RunLoop是一个对象，该对象管理了其需要处理的事件和消息，并提供了入口函数来管理逻辑。

3. 线程执行了该函数后，就会处于接收消息->等待->处理的循环中，直至循环结束，函数返回

## NSRunLoop && CFRunLoopRef
> CFRunLoopRef 是在coreFoundation框架内的，提供了纯C函数的API，并且是线程安全的
> NSRunLoop 是基于CFRunLoopRef的封装，提供了面向对象的API，但不是线程安全的




```
ForFoundationOnly.h
// ---- Thread-specific data --------------------------------------------

// Get some thread specific data from a pre-assigned slot.
CF_EXPORT void *_CFGetTSD(uint32_t slot);

// Set some thread specific data in a pre-assigned slot. Don't pick a random value. Make sure you're using a slot that is unique. Pass in a destructor to free this data, or NULL if none is needed. Unlike pthread TSD, the destructor is per-thread.
CF_EXPORT void *_CFSetTSD(uint32_t slot, void *newVal, void (*destructor)(void *));
```

## 源码
### 向rl的_commonModes中添加名为modeName的mode

```
void CFRunLoopAddCommonMode(CFRunLoopRef rl, CFStringRef modeName)
```

1. 判断rl的_commonModes中是否已经存在该mode，存在的话，不做任何处理

2. 如果不存在：
	1. _commonModes 添加 modeName
	2. 把rl的_commonModeItems中每个item都添加到modeName里面

### 为runLoop中名字为modeName的mode添加runLoopSource，rls

```
void CFRunLoopAddSource(CFRunLoopRef rl, CFRunLoopSourceRef rls, CFStringRef modeName)
```

1. 判断，如果是commonMode的话，那么runLoop中，commonModes中的所有mode都需要更新
	1. 获取到commonModes和commonModeItems

	2. commonModeItems 添加rls
	3. commonMode添加rls，跳转到下面的第二步

2. 如果不是commonMode，获取到对应的mode，找不到的话进行初始化创建
	1. 创建`_sources0`、`_sources1`和`_portToV1SourceMap`

	2. 判断rls属于0还是1，然后把它添加到rlm对应的source集合中
	3. 如果是1，还需要获取端口src_port，并且端口跟rls一一对应，存储字典_portToV1SourceMap，最后端口存入rlm的_portSet中

	4. 把runLoop加入到rls的_runLoops中

3. 如果rls是0，设置rls的schedule

### 添加observer


