
# 从源码上认识RunLoop
> 源码比文字更令人深刻

版本： CF-1151.16

[官方介绍](https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html#//apple_ref/doc/uid/10000057i-CH16-SW12)

[相关API](https://developer.apple.com/reference/corefoundation)

[源码下载](http://opensource.apple.com/tarballs/CF/CF-1151.16.tar.gz)

为了文章简洁，只摘抄部分主要的代码
源码详细介绍在[这里](https://github.com/LionWY/SourceCodeStorage/blob/master/Demo/RunLoopTest/RunLoopTest/CF-1151.16/CFRunLoop.c)


## 基础概念

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


## 相关方法
> 5个类之间的主要方法，来详细了解类之间的相互关系

1. 获取runLoop正在运行的mode（即`_currentMode`）的name。
```
CFStringRef CFRunLoopCopyCurrentMode(CFRunLoopRef rl) {

	    CFStringRef result = NULL;

       result = (CFStringRef)CFRetain(rl->_currentMode->_name);
	    return result;
}
```


2. 返回一个数组，其中包含了runLoop所有定义过的mode（即`_modes`）的name
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

3. 向runLoop的commonModes添加一个mode
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
 
4. 添加一个source到指定的runLoopMode
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
            // 2.4 把rl 加入到rls的_runLoops中
            if (NULL == rls->_runLoops) {
                
                rls->_runLoops = CFBagCreateMutable(kCFAllocatorSystemDefault, 0, &kCFTypeBagCallBacks); // sources retain run loops!
            }
            CFBagAddValue(rls->_runLoops, rl);
       }
}
```

5. 添加rlo到指定的rlm
```
CF_EXPORT void CFRunLoopAddObserver(CFRunLoopRef rl, CFRunLoopObserverRef observer, CFRunLoopMode mode);
```
内部跟4差不多，都是根据mode是否commonMode分两种情况，差别在于，mode有一个数组`_observers`，添加是根据rlo的`_order`进行添加的，而关联rl根据`_rlCount`是否为0。只有当rlo的`_rlCount`为0时，其`_runLoop`才是rl。
observer可以添加进rl的多个mode，但是

6. 













```
ForFoundationOnly.h
// ---- Thread-specific data --------------------------------------------

// Get some thread specific data from a pre-assigned slot.
CF_EXPORT void *_CFGetTSD(uint32_t slot);

// Set some thread specific data in a pre-assigned slot. Don't pick a random value. Make sure you're using a slot that is unique. Pass in a destructor to free this data, or NULL if none is needed. Unlike pthread TSD, the destructor is per-thread.
CF_EXPORT void *_CFSetTSD(uint32_t slot, void *newVal, void (*destructor)(void *));
```





