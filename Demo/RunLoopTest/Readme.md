
> 源码比文字更令人深刻

版本： CF-1151.16

[官方文档](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html#//apple_ref/doc/uid/10000057i-CH16-SW12)

[相关API](https://developer.apple.com/reference/corefoundation)

[源码下载](http://opensource.apple.com/tarballs/CF/CF-1151.16.tar.gz)

为了文章简洁，只摘抄部分主要的代码
源码详细介绍在[这里](https://github.com/LionWY/SourceCodeStorage/blob/master/Demo/RunLoopTest/RunLoopTest/CF-1151.16/CFRunLoop.c)

![](http://oeb4c30x3.bkt.clouddn.com/runLoop.jpg)


## 邂逅runLoop
应该是一个美丽的下午，在一场面试上，遇见了runLoop，可惜擦肩而过。。。

## 认识runLoop

### CFRunLoop
```
struct __CFRunLoop {
    pthread_t _pthread;            		 // runLoop 对应的线程
    
    __CFPort _wakeUpPort;				// 用来唤醒runLoop的端口，接收消息，执行CFRunLoopWakeUp方法
    
    CFMutableSetRef _commonModes;       // 集合，所有标记为common的mode的集合
    
    CFMutableSetRef _commonModeItems;   // 集合，commonMode的item（observers/sources/timers）的集合
    
    CFRunLoopModeRef _currentMode;      // 当前runLoop运行的mode
    
    CFMutableSetRef _modes;             // 集合，mode的集合
};
```

从源码可以看出一部分内容 ：
一个runLoop对象，主要包含一个线程`_pthread`，一个用来被唤醒的端口`_wakeUpPort`，一个当前运行的mode`_currentMode`，以及若干个`_modes`、`_commonModes`、`_commonModeItems`。
runLoop有很多mode，即`_modes`，但是只有一个`_currentMode`，runLoop一次只能运行在一个mode下，不可能在多个mode下同时运行。

### CFRunLoopMode

```
struct __CFRunLoopMode {
	CFStringRef _name;      // mode的名字，唯一标识
    
    Boolean _stopped;       // mode的状态，是否停止
    
    CFMutableSetRef _sources0;  // sources0 的集合
    
    CFMutableSetRef _sources1;  // sources1 的集合
    
    CFMutableArrayRef _observers;   // 存储所有观察者（observers）的数组
    
    CFMutableArrayRef _timers;      // 存储所有定时器（timers）的数组
    
    // 源码中有一段代码，可以看出字典的存储对象
    // CFDictionarySetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)src_port, rls);
    CFMutableDictionaryRef _portToV1SourceMap;  // 字典 key是__CFPort，value是CFRunLoopSourceRef
    
    // __CFPortSetInsert(src_port, rlm->_portSet);
    __CFPortSet _portSet;           // 端口的集合
}
```

从mode的组成可以看出来：mode管理了所有的事件（sources/timers/observers），而runLoop是管理mode的

### CFRunLoopSource

```
struct __CFRunLoopSource {
	CFMutableBagRef _runLoops; 				// 一个Source 对应多个runLoop
	
	union {
        
        CFRunLoopSourceContext version0; 	// source0
        
        CFRunLoopSourceContext1 version1; 	//source1	
        
    } _context;
	
}
// source0
typedef struct {
    CFIndex	version; 	// 版本号，用来区分是source1还是source0

    void *	info;
    
    // schedule cancel 是对应的，
    void	(*schedule)(void *info, CFRunLoopRef rl, CFStringRef mode);
    void	(*cancel)(void *info, CFRunLoopRef rl, CFStringRef mode);

    void	(*perform)(void *info); // 用来回调的指针
   
} CFRunLoopSourceContext;

// source1
typedef struct {
    CFIndex	version; 	// 版本号
    void *	info;
    
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
    mach_port_t	(*getPort)(void *info); // 端口
    void *	(*perform)(void *msg, CFIndex size, CFAllocatorRef allocator, void *info);
#else
    void *	(*getPort)(void *info);
    
    void	(*perform)(void *info); // 用来回调的指针
#endif
} CFRunLoopSourceContext1;
```
源码中看出来，source0和source1的区别，source1比source0多一个接收消息的端口`mach_port_t`

### CFRunLoopObserver

```
struct __CFRunLoopObserver {
   
    CFRunLoopRef _runLoop;         // observer对应的runLoop, 一一对应
    
    CFIndex _rlCount;              //  observer当前监测的runLoop数量，主要在安排/移除runLoop的时候用到
    
    CFOptionFlags _activities;      // observer观测runLoop的状态，枚举类型，
    
    CFIndex _order;                 // mode使用数组存储observers，根据_order添加observer
    
    CFRunLoopObserverCallBack _callout; 
};
```
`_activities`状态值：
```
typedef CF_OPTIONS(CFOptionFlags, CFRunLoopActivity) {
    kCFRunLoopEntry = (1UL << 0),               // 即将进入Loop
    kCFRunLoopBeforeTimers = (1UL << 1),        // runLoop即将处理 Timers
    kCFRunLoopBeforeSources = (1UL << 2),       // runLoop即将处理 Sources
    kCFRunLoopBeforeWaiting = (1UL << 5),       // runLoop即将进入休眠
    kCFRunLoopAfterWaiting = (1UL << 6),        // runLoop刚从休眠中唤醒
    kCFRunLoopExit = (1UL << 7),                // 即将退出RunLoop
    kCFRunLoopAllActivities = 0x0FFFFFFFU       
};
```

### CFRunLoopTimer

```
struct __CFRunLoopTimer {
    
    CFRunLoopRef _runLoop;          // timer 对应的runLoop
    CFMutableSetRef _rlModes;       // 集合，存放对应的modes，猜测一个timer 可以有多个modes，即可以被加入到多个modes中
    
    CFRunLoopTimerCallBack _callout;
};
```


## 了解runLoop
> 5个类之间的主要方法，来详细了解类之间的相互关系

### CFRunLoopCopyCurrentMode
>  获取runLoop正在运行的mode（即`_currentMode`）的name。

```
CFStringRef CFRunLoopCopyCurrentMode(CFRunLoopRef rl) {

	    CFStringRef result = NULL;

       result = (CFStringRef)CFRetain(rl->_currentMode->_name);
	    return result;
}
```

### CFRunLoopCopyAllModes
> 返回一个数组，其中包含了runLoop所有定义过的mode（即`_modes`）的name

```
CFArrayRef CFRunLoopCopyAllModes(CFRunLoopRef rl) {

	    CFMutableArrayRef array;
	    
	    array = CFArrayCreateMutable(kCFAllocatorSystemDefault, CFSetGetCount(rl->_modes), &kCFTypeArrayCallBacks);
	    
	    // CFSetApplyFunction 三个参数a，b，c，
	    // 表示:对a里面的每个对象，都执行一次b方法，b方法的参数是a和c，后面会多次遇到
	    CFSetApplyFunction(rl->_modes, (__CFRunLoopGetModeName), array);
	    
	    return array;
}

  // 把mode的name添加进数组array
static void __CFRunLoopGetModeName(const void *value, void *context) {
	    CFRunLoopModeRef rlm = (CFRunLoopModeRef)value;
	    CFMutableArrayRef array = (CFMutableArrayRef)context;
	    CFArrayAppendValue(array, rlm->_name);
}
```

### CFRunLoopAddCommonMode
>  向runLoop的commonModes添加一个mode 

```
void CFRunLoopAddCommonMode(CFRunLoopRef rl, CFStringRef modeName) {
	    
	    // 判断 modeName 是否在_commonModes 中，如果已经存在，else中不做任何处理
	    if (!CFSetContainsValue(rl->_commonModes, modeName)) {
	        
	        // set 是 runLoop 的 _commonModeItems一份拷贝
	        CFSetRef set = rl->_commonModeItems ? CFSetCreateCopy(kCFAllocatorSystemDefault, rl->_commonModeItems) : NULL;
	        // 1. _commonModes 添加 modeName,
	        // 可见_commonModes存储的其实是CFStringRef类型的modeName
	        CFSetAddValue(rl->_commonModes, modeName);
	        
	        // 如果items 存在
	        if (NULL != set) {
	            CFTypeRef context[2] = {rl, modeName};
	            // 2. 为modeName对应的Mode添加items中的每个item(timer/source/observer)
	            // 为set中的每个item，调用一次__CFRunLoopAddItemsToCommonMode方法
	            CFSetApplyFunction(set, (__CFRunLoopAddItemsToCommonMode), (void *)context);
	        }
	    } else {
	    }
}

 // 把一个item添加到指定的mode中
static void __CFRunLoopAddItemsToCommonMode(const void *value, void *ctx) {
    
	    CFTypeRef item = (CFTypeRef)value;
	    
	    CFRunLoopRef rl = ()(((CFTypeRef *)ctx)[0]);
	    
	    CFStringRef modeName = (CFStringRef)(((CFTypeRef *)ctx)[1]);
	    
	    // 判断item具体是哪种类型，然后进行添加
	    if (CFGetTypeID(item) == CFRunLoopSourceGetTypeID()) {
	        CFRunLoopAddSource(rl, (CFRunLoopSourceRef)item, modeName);
	    } else if (CFGetTypeID(item) == CFRunLoopObserverGetTypeID()) {
	        CFRunLoopAddObserver(rl, (CFRunLoopObserverRef)item, modeName);
	    } else if (CFGetTypeID(item) == CFRunLoopTimerGetTypeID()) {
	        CFRunLoopAddTimer(rl, (CFRunLoopTimerRef)item, modeName);
	    }
}
```
 
### CFRunLoopAddSource
> 添加一个source到指定的runLoopMode

```
void CFRunLoopAddSource(CFRunLoopRef rl, CFRunLoopSourceRef rls, CFStringRef modeName) {	/* DOES CALLOUT */
	    
	    // 声明一个bool值的标识,后续用来source0 添加source
	    Boolean doVer0Callout = false;
	    
	    // 1. 如果是commonMode，那么commonModes中的所有mode都要更新
	    if (modeName == kCFRunLoopCommonModes) {
		    /*
		    这里获取rl->_commonModes并赋值set，如果没有为NULL
		    同时获取rl->_commonModeItems，如果不存在就初始化创建
		    */
	        // 1.1 先把 rls 添加进_commonModeItems
	        CFSetAddValue(rl->_commonModeItems, rls);
            // 1.2 为set中其他的mode，添加rls 
            CFSetApplyFunction(set, (__CFRunLoopAddItemToCommonModes), (void *)context);  

	    }
	    // 2. 非commonMode的添加 
	    else {
	        // 2.1 在runLoop的_modes中查找名字为modeName的mode，找不到会在内部进行初始化创建（true决定是否创建）
	        CFRunLoopModeRef rlm = __CFRunLoopFindMode(rl, modeName, true);
	        
	        // 2.2 获取mode的跟source有关的_sources0，_sources1以及端口_portToV1SourceMap
	        if (NULL != rlm && NULL == rlm->_sources0) {
	            rlm->_sources0 = CFSetCreateMutable(kCFAllocatorSystemDefault, 0, &kCFTypeSetCallBacks);
	            rlm->_sources1 = CFSetCreateMutable(kCFAllocatorSystemDefault, 0, &kCFTypeSetCallBacks);
	            rlm->_portToV1SourceMap = CFDictionaryCreateMutable(kCFAllocatorSystemDefault, 0, NULL, NULL);
	        }
	        
	        
            // 2.3 判断rls属于哪种类型，并针对性的添加 
            // 2.3.1 source0的情况
            if (0 == rls->_context.version0.version) {
                CFSetAddValue(rlm->_sources0, rls);
                // 下面这段代码是后面的，放在这里便于理解，source0 有个schedule指针，把rl和rlm关联起来
                rls->_context.version0.schedule(rls->_context.version0.info, rl, modeName);
            }
            // 2.3.2 source1的情况 
            else if (1 == rls->_context.version0.version) {
                CFSetAddValue(rlm->_sources1, rls);
                // 获取rls的端口
                __CFPort src_port = rls->_context.version1.getPort(rls->_context.version1.info);
                // rls和端口一一对应,并存储在mode的字典_portToV1SourceMap中
                CFDictionarySetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)src_port, rls);
               // 把source1 的端口添加进mode的端口集合_portSet中
                __CFPortSetInsert(src_port, rlm->_portSet);
            }
            // 2.4 把rl 加入到rls的_runLoops中，即一个resources可以对应多个runLoop
            if (NULL == rls->_runLoops) {
                
                rls->_runLoops = CFBagCreateMutable(kCFAllocatorSystemDefault, 0, &kCFTypeBagCallBacks); // sources retain run loops!
            }
            CFBagAddValue(rls->_runLoops, rl);
       }
}
```

### CFRunLoopAddObserver
> 添加rlo到指定的rlm

```
CF_EXPORT void CFRunLoopAddObserver(CFRunLoopRef rl, CFRunLoopObserverRef observer, CFRunLoopMode mode);
```
内部实现`CFRunLoopSource`跟差不多，都是根据mode是否commonMode分两种情况，差别在于：

* 关联mode：mode有一个数组`_observers`，添加是根据rlo的`_order`进行添加的

* 关联rl：根据`_rlCount`是否为0。只有当rlo的`_rlCount`为0时，其`_runLoop`才是rl。

### CFRunLoopAddTimer
> 添加rlt到指定的rlm

```
CF_EXPORT void CFRunLoopAddTimer(CFRunLoopRef rl, CFRunLoopTimerRef timer, CFRunLoopMode mode);
```
内部实现同上，区别：

1. rlt只能添加到其`_runLoop`的mode中，如果rl不是其`_runLoop`，直接返回
```
if (NULL == rlt->_runLoop) {
           rlt->_runLoop = rl;
       } else if (rl != rlt->_runLoop) {
           __CFRunLoopTimerUnlock(rlt);
           __CFRunLoopModeUnlock(rlm);
           __CFRunLoopUnlock(rl);
           return;
       }
```

2. rlt有一个变量`_rlModes`，其存储的是rlt所在的mode的name
```
CFSetAddValue(rlt->_rlModes, rlm->_name);
```

3. rlm有一个变量`_timers`，其存储timer是根据timer的启动时间，即`_fireTSR`，进行排序的


## 获取runLoop
> runLoop跟其所在线程是一一对应的

1. API提供了两个获取runLoop的方法
```
CFRunLoopRef CFRunLoopGetMain(void) {
	    static CFRunLoopRef __main = NULL; // no retain needed
	    
	    // pthread_main_thread_np() 主线程
	    if (!__main) __main = _CFRunLoopGet0(pthread_main_thread_np()); // no CAS needed
	    return __main;
}

 CFRunLoopRef CFRunLoopGetCurrent(void) {
	   
	    CFRunLoopRef rl = (CFRunLoopRef)_CFGetTSD(__CFTSDKeyRunLoop);
	    if (rl) return rl;
	    // pthread_self() 当前线程
	    return _CFRunLoopGet0(pthread_self());
	}
```
其中，`TSD`是thread special data，表示线程私有数据，在 C++ 中，全局变量可以被所有线程访问，局部变量只有函数内部可以访问。而 TSD 的作用就是能够在同一个线程的不同函数中被访问。（找到的资料）
`__CFTSDKeyRunLoop`是一个枚举类型的关键字。
`pthread_self()`可以得知，如果要获取非主线程的runLoop，必须在该线程内部调用`CFRunLoopGetCurrent`才能获取。

2. 根据线程t获取对应的runLoop
```
// 一个内部全局的字典
static CFMutableDictionaryRef __CFRunLoops = NULL;
CF_EXPORT CFRunLoopRef _CFRunLoopGet0(pthread_t t) {
    
	    // 1. 保证t不为空	   
	    if (pthread_equal(t, kNilPthreadT)) {
	        
	        t = pthread_main_thread_np();
	    }
	    
	    // 2. 创建全局字典,并存储主线程的runLoop
	    if (!__CFRunLoops) {
	      
	        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorSystemDefault, 0, NULL, &kCFTypeDictionaryValueCallBacks);
	    
	        // 通过pthread_main_thread_np()创建CFRunLoopRef类型的mainLoop，内部对其所有变量进行初始化，并且赋值_pthread为pthread_main_thread_np()
	        CFRunLoopRef mainLoop = __CFRunLoopCreate(pthread_main_thread_np());
	        
	        // key是主线程的指针， value 是刚创建的mainLoop
	        CFDictionarySetValue(dict, pthreadPointer(pthread_main_thread_np()), mainLoop);
	        
	        // 比较并交换指针，
	        // 这里比较第一个参数NULL和第三个参数 (void * volatile *)&__CFRunLoops全局字典，如果相等，系统会自动把第二参数的值赋给第三个参数，
	        // volatile的作用是 每次取得数值的方式是直接从内存中读取
	        if (!OSAtomicCompareAndSwapPtrBarrier(NULL, dict, (void * volatile *)&__CFRunLoops)) {
	            CFRelease(dict);
	        }
	        
	        // coreFoundation 要手动管理内存， create 对应 release
	        CFRelease(mainLoop);
	    }
	    
	    // 3. 全局字典已经存在，从中获取对应线程t的runLoop
	    CFRunLoopRef loop = (CFRunLoopRef)CFDictionaryGetValue(__CFRunLoops, pthreadPointer(t));
	    
	    // 如果获取不到loop，
	    if (!loop) {
	        
	        // 根据 t 创建 一个newLoop
	        CFRunLoopRef newLoop = __CFRunLoopCreate(t);
	       
	        // 再一次进行获取
	        loop = (CFRunLoopRef)CFDictionaryGetValue(__CFRunLoops, pthreadPointer(t));
	        
	        // 如果还不存在，就直接赋值，
	        if (!loop) {
	            CFDictionarySetValue(__CFRunLoops, pthreadPointer(t), newLoop);
	            loop = newLoop;
	        }
	    }
	    // 4. 注册TSD
	    if (pthread_equal(t, pthread_self())) {
	        
	        // 注册回调，当线程销毁时，顺便也销毁其对应的 RunLoop
	        _CFSetTSD(__CFTSDKeyRunLoop, (void *)loop, NULL);
	        
	        if (0 == _CFGetTSD(__CFTSDKeyRunLoopCntr)) {
	            _CFSetTSD(__CFTSDKeyRunLoopCntr, (void *)(PTHREAD_DESTRUCTOR_ITERATIONS-1), (void (*)(void *))__CFFinalizeRunLoop);
	        }
	    }
	    return loop;
}
```
线程和runLoop是一一对应，保存在一个全局字典里，主线程的runLoop是在初始化字典时已经创建好了，其他线程的runLoop只有在获取的时候才会创建。

## 运行runLoop

### CFRunLoopRun

> 默认情况下，运行当前线程的runLoop

```
void CFRunLoopRun(void) {	
    int32_t result;
    do {
        result = CFRunLoopRunSpecific(CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 1.0e10, false);
    } while (kCFRunLoopRunStopped != result && kCFRunLoopRunFinished != result);
}
```
源码得知：
1. `kCFRunLoopDefaultMode`，默认情况下，runLoop是在这个mode下运行的，
2. runLoop的运行主体是一个do..while循环，除非停止或者结束，否则runLoop会一直运行下去

### CFRunLoopRunInMode

> 在指定的mode下运行当前线程的runLoop

```
SInt32 CFRunLoopRunInMode(CFStringRef modeName, CFTimeInterval seconds, Boolean returnAfterSourceHandled) {     
    return CFRunLoopRunSpecific(CFRunLoopGetCurrent(), modeName, seconds, returnAfterSourceHandled);
}
```
该方法，可以设置runLoop运行在哪个mode下`modeName`，超时时间`seconds`，以及是否处理完事件就返回`returnAfterSourceHandled`。
这两个方法实际调用的是同一个方法`CFRunLoopRunSpecific`，其返回是一个`SInt32`类型的值，根据返回值，来决定runLoop的运行状况。

### CFRunLoopRunSpecific

> 在指定的mode下，运行指定的runLoop

```
SInt32 CFRunLoopRunSpecific(CFRunLoopRef rl, CFStringRef modeName, CFTimeInterval seconds, Boolean returnAfterSourceHandled) {    
    // 根据rl，modeName获取指定的currentMode
    CFRunLoopModeRef currentMode = __CFRunLoopFindMode(rl, modeName, false);
    
    // 1. 如果当前mode 不存在，或者当前mode中事件为空，runLoop 结束，返回 kCFRunLoopRunFinished
    if (NULL == currentMode || __CFRunLoopModeIsEmpty(rl, currentMode, rl->_currentMode)) {
        // 声明一个标识did，默认false
        Boolean did = false;
        // did 为 false，返回 kCFRunLoopRunFinished
        return did ? kCFRunLoopRunHandledSource : kCFRunLoopRunFinished;
    }
    
    // 初始化一个返回结果，值为kCFRunLoopRunFinished
    int32_t result = kCFRunLoopRunFinished;

	// 2. kCFRunLoopEntry， 通知observers 即将开始循环
    if (currentMode->_observerMask & kCFRunLoopEntry ) __CFRunLoopDoObservers(rl, currentMode, kCFRunLoopEntry);
    
    // runLoop运行主体
	result = __CFRunLoopRun(rl, currentMode, seconds, returnAfterSourceHandled, previousMode);
    
    // 3. kCFRunLoopExit， 通知 observers 即将退出循环runLoop
	if (currentMode->_observerMask & kCFRunLoopExit ) __CFRunLoopDoObservers(rl, currentMode, kCFRunLoopExit);

    return result;
}
```
这里有3点：
1. kCFRunLoopRunFinished mode中没有事件处理，直接返回
2. kCFRunLoopEntry 	runLoop即将开始运行，通知observers
3. kCFRunLoopExit runLoop 即将退出，通知observers

### __CFRunLoopRun
> 这里处理了runLoop从开始运行到退出的所有逻辑

```
static int32_t __CFRunLoopRun(CFRunLoopRef rl, CFRunLoopModeRef rlm, CFTimeInterval seconds, Boolean stopAfterHandle, CFRunLoopModeRef previousMode) {
    
    // 1. 如果runLoop停止或者runLoopMode为停止状态，直接返回 kCFRunLoopRunStopped
    if (__CFRunLoopIsStopped(rl)) {
        __CFRunLoopUnsetStopped(rl);
        return kCFRunLoopRunStopped;
    } else if (rlm->_stopped) {
	rlm->_stopped = false;
	   return kCFRunLoopRunStopped;
    }
    
    // 获取主线程用来接收消息的端口
    dispatchPort = _dispatch_get_main_queue_port_4CF();
   
    // 获取执行timers对应的线程的端口
    modeQueuePort = _dispatch_runloop_root_queue_get_port_4CF(rlm->_queue);
    
    // GCD 管理的定时器，用于实现runLoop的超时机制
    dispatch_source_t timeout_timer = NULL;    
    struct __timeout_context *timeout_context = (struct __timeout_context *)malloc(sizeof(*timeout_context));
    
    // 处理timer 三种情况 ：timer1 立即超时
    if (seconds <= 0.0) { // instant timeout
        seconds = 0.0;
        timeout_context->termTSR = 0ULL;
        
        // timer2 即将超时
    } else if (seconds <= TIMER_INTERVAL_LIMIT) {
			// 判断在哪个线程中执行
        dispatch_queue_t queue = pthread_main_np() ? __CFDispatchQueueGetGenericMatchingMain() : __CFDispatchQueueGetGenericBackground();
        
        timeout_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);

        // 事件一一对应，
        dispatch_source_set_event_handler_f(timeout_timer, __CFRunLoopTimeout);
        dispatch_source_set_cancel_handler_f(timeout_timer, __CFRunLoopTimeoutCancel);
        dispatch_source_set_timer(timeout_timer, dispatch_time(1, ns_at), DISPATCH_TIME_FOREVER, 1000ULL);
        // 定时器执行
        dispatch_resume(timeout_timer);
        
    } else {
        // timer3 永不超时
        seconds = 9999999999.0;
        timeout_context->termTSR = UINT64_MAX;
    }

    // 声明一个标识，默认true，用于执行消息处理
    Boolean didDispatchPortLastTime = true;
    // 声明一个返回值，用于最后的结果返回
    int32_t retVal = 0;
    
    // do..while循环主体，处理runLoop的逻辑
    do {

        // 获取rlm的端口集合
        __CFPortSet waitSet = rlm->_portSet;
       // runLoop设置为可被唤醒的状态
        __CFRunLoopUnsetIgnoreWakeUps(rl);
        
        // 2. kCFRunLoopBeforeTimers runLoop即将处理Timers， 通知observers
        if (rlm->_observerMask & kCFRunLoopBeforeTimers) __CFRunLoopDoObservers(rl, rlm, kCFRunLoopBeforeTimers);
        // 3. kCFRunLoopBeforeSources runLoop即将处理Sources，通知observers
        if (rlm->_observerMask & kCFRunLoopBeforeSources) __CFRunLoopDoObservers(rl, rlm, kCFRunLoopBeforeSources);
        
        // 4. runLoop开始处理source0事件
        // sourceHandledThisLoop 是否处理完Source0事件
        // 内部实现是，只有被标记Signaled的source0事件才会被处理，但在处理之前会去除标记__CFRunLoopSourceUnsetSignaled
        Boolean sourceHandledThisLoop = __CFRunLoopDoSources0(rl, rlm, stopAfterHandle);
        
        if (sourceHandledThisLoop) {
            // 处理完Source0之后的回调
            __CFRunLoopDoBlocks(rl, rlm);
        }

        // 处理完source0事件，且没有超时 poll 为false, 
        // 没有处理完source0 事件，或者超时，为true
        Boolean poll = sourceHandledThisLoop || (0ULL == timeout_context->termTSR);

        // didDispatchPortLastTime 初始化为true，即第一次循环的时候不会走if方法，
        // 5. 消息处理，source1 事件，goto 第9步
        if (MACH_PORT_NULL != dispatchPort && !didDispatchPortLastTime) {

            // 从消息缓冲区获取消息
            msg = (mach_msg_header_t *)msg_buffer;
            // dispatchPort收到消息，立刻去处理 
            // dispatchPort 主线程接收消息的端口
            if (__CFRunLoopServiceMachPort(dispatchPort, &msg, sizeof(msg_buffer), &livePort, 0, &voucherState, NULL)) {
                // 收到消息，立马去处理
                goto handle_msg;
            }

            if (__CFRunLoopWaitForMultipleObjects(NULL, &dispatchPort, 0, 0, &livePort, NULL)) {
                goto handle_msg;
            }

        }
        // didDispatchPortLastTime 设置为false，以便进行消息处理
        didDispatchPortLastTime = false;

        // 6. kCFRunLoopBeforeWaiting，通知 observers runLoop即将休眠
		if (!poll && (rlm->_observerMask & kCFRunLoopBeforeWaiting)) __CFRunLoopDoObservers(rl, rlm, kCFRunLoopBeforeWaiting);
        
        // runLoop 休眠
        __CFRunLoopSetSleeping(rl);
        
        // 7.线程进入休眠, 直到被下面某一个事件唤醒。(文档给出的结果：)
	    // 7.1. 基于 port 的Source1 的事件
	    // 7.2. Timer 到时间了
	    // 7.3. RunLoop 启动时设置的最大超时时间到了
	    // 7.4. 被手动唤醒
        do {
            // 从消息缓冲区获取消息
            msg = (mach_msg_header_t *)msg_buffer;
				// 内部调用 mach_msg() 等待接受 waitSet 的消息
            __CFRunLoopServiceMachPort(waitSet, &msg, sizeof(msg_buffer), &livePort, poll ? 0 : TIMEOUT_INFINITY, &voucherState, &voucherCopy);
        } while (1);

        // 设置rl不再等待唤醒
        __CFRunLoopSetIgnoreWakeUps(rl);
        // runloop 醒来
        __CFRunLoopUnsetSleeping(rl);
        
        // 8. kCFRunLoopAfterWaiting 已被唤醒，通知observers
	   if (!poll && (rlm->_observerMask & kCFRunLoopAfterWaiting)) __CFRunLoopDoObservers(rl, rlm, kCFRunLoopAfterWaiting);

        // 9. 处理消息
        handle_msg:;
        // 设置rl不再等待唤醒
        __CFRunLoopSetIgnoreWakeUps(rl);

        // 判断 livePort
        // 9.1 如果不存在
        if (MACH_PORT_NULL == livePort) {
            CFRUNLOOP_WAKEUP_FOR_NOTHING();
            // 9.2 如果是唤醒rl的端口，回到第2步
        } else if (livePort == rl->_wakeUpPort) {
            CFRUNLOOP_WAKEUP_FOR_WAKEUP();
            ResetEvent(rl->_wakeUpPort);
        }
        // 定时器事件__CFRunLoopDoTimers
        // 9.3 如果是定时器的端口
        else if (modeQueuePort != MACH_PORT_NULL && livePort == modeQueuePort) {
            // 处理定时器事件
            CFRUNLOOP_WAKEUP_FOR_TIMER();
            if (!__CFRunLoopDoTimers(rl, rlm, mach_absolute_time())) {
                __CFArmNextTimerInMode(rlm, rl);
            }
        }
        // 9.4. 如果端口是主线程的端口，直接处理
        else if (livePort == dispatchPort) {
            CFRUNLOOP_WAKEUP_FOR_DISPATCH();
            __CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__(msg);     
        } else {
            // 9.5. 除上述4点之外的端口
            CFRUNLOOP_WAKEUP_FOR_SOURCE();
            
            // 从端口收到的消息事件，为source1事件
            CFRunLoopSourceRef rls = __CFRunLoopModeFindSourceForMachPort(rl, rlm, livePort);
            
            if (rls) {

                mach_msg_header_t *reply = NULL;
                        // 处理source1 事件
                sourceHandledThisLoop = __CFRunLoopDoSource1(rl, rlm, rls, msg, msg->msgh_size, &reply) || sourceHandledThisLoop;
                if (NULL != reply) {
                		// 消息处理，
                		// message.h中，以后有时间会再研究一下
                    (void)mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
                }

            }
            
        } 
        // 10. 返回结果的处理  
        if (sourceHandledThisLoop && stopAfterHandle) {
            // 10.1 如果事件处理完就返回，并且source处理完成
            retVal = kCFRunLoopRunHandledSource;
        } else if (timeout_context->termTSR < mach_absolute_time()) {
            // 10.2 超时
            retVal = kCFRunLoopRunTimedOut;
        } else if (__CFRunLoopIsStopped(rl)) {
            // 10.3 被外部调用者强制停止了
            __CFRunLoopUnsetStopped(rl);
            retVal = kCFRunLoopRunStopped;
        } else if (rlm->_stopped) {
            // 10.4 runLoopMode 状态停止
            rlm->_stopped = false;
            retVal = kCFRunLoopRunStopped;
        } else if (__CFRunLoopModeIsEmpty(rl, rlm, previousMode)) {
            // 10.5 source/timer/observer一个都没有了
            retVal = kCFRunLoopRunFinished;
        }
        // 上述几种情况，会跳出do..while循环，
        // 除此之外，继续循环
    } while (0 == retVal);
    return retVal;
}
```
上述2-10就是runLoop运行过程中的循环逻辑，而最终返回的状态有：`kCFRunLoopRunFinished`、`kCFRunLoopRunStopped`、`kCFRunLoopRunTimedOut`以及`kCFRunLoopRunHandledSource`四种枚举类型

## 总结：
***1. runLoop跟线程一一对应，非主线程的rl只能在其内部获取，runLoop管理rlm和回调block，而rlm存储了所有的事件。***

***2. runLoop运行核心就是一个do..while循环，遍历所有事件，有事件处理，无事件休眠，直至达到退出条件。***

***3. 以上就是runLoop内部的源码分析，当然会有理解不到位的情况，也留有待解决的问题，万望不吝赐教。***


参考资料：

[深入理解RunLoop
](http://blog.ibireme.com/2015/05/18/runloop/)

[RunLoop系列之源码分析
](http://aaaboom.com/?p=34#wow23)

[Run Loops
](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html#//apple_ref/doc/uid/10000057i-CH16-SW20)

[CFRunLoop
](https://developer.apple.com/reference/corefoundation/1666621-cfrunloop)



