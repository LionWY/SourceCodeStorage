# 0210 Load 方法 暨 程序的加载顺序

[toc]

## 前言

众所周知，App 的入口是 `main` 函数，而在此之前，我们了解到的是系统会自动调用 `load` 方法。而且是先调用父类的，再是自己的，最后才是分类的。而为什么是这样呢，不清楚。
下面所有的 `load` 方法， 都指 `+ (void)load {}` 方法。

## 入口

借助于[可调试的 objc 源码](https://github.com/isaacselement/objc4-706) 了解了 `load` 方法的具体流程。

1. 创建一个类 `XXObject`，新建一个 `load` 方法，打断点，调用栈显示，引出了 `dyld` 和 `ImageLoader`。

![](http://oeb4c30x3.bkt.clouddn.com//20170213093622_g7tU44_QQ20170210-1.jpeg)

    

2. dyld: The Dynamic Link Editor
    * Apple 的动态链接库，系统内核做好启动程序的初始准备后，将其他事物交给 dyld 处理
    
    * 详细可以看 [sunnyxx](http://blog.sunnyxx.com/2014/08/30/objc-pre-main/)

3. ImageLoader：
    * images 表示二进制文件（可执行文件或者动态链接库 .so 文件）编译后的符号、代码等
    
    * ImageLoader 作用是将这些文件加载进内存，且每一个文件对应一个 ImageLoader 实例来负责加载
        * 在程序运行时它先将动态链接的 image 递归加载
        * 再从可执行文件 image 递归加载所有符号
    
## load 流程

* 在分析 load 之前，还需要了解下 runtime 的初始化入口。

### __objc_init
> Bootstrap initialization. Registers our image notifier with dyld.
> Called by libSystem BEFORE library initialization time

```
void _objc_init(void)
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    
    // 环境初始化
    environ_init();
    tls_init();
    static_init();
    lock_init();
    // 初始化 libobjc 的异常处理系统
    exception_init();
    // 添加通知、回调
    _dyld_objc_notify_register(&map_2_images, load_images, unmap_image);
}
```

1. 引导初始化
2. 注册通知，当二进制文件 images 加载到内存时，通知 runtime 进行处理。
    * `map_2_images`：处理已经被 mapped 的images
    
    * `load_images`：处理 已被 mapped 的 images 中的 +load 方法
    
    * `unmap_image`：处理将要 unmap 的 images
    


### 1. load_images
> Process +load in the given images which are being mapped in by dyld

* 处理 dyld 提供的已被 map_images 处理后的 images 中的 +load 方法

```
void
load_images(const char *path __unused, const struct mach_header *mh)
{
    // Return without taking locks if there are no +load methods here.
    // 如果没有 load 方法，直接返回，
    if (!hasLoadMethods((const headerType *)mh)) return;

    recursive_mutex_locker_t lock(loadMethodLock);

    // Discover load methods
    // 收集 load 方法，为下面调用做准备
    {
        rwlock_writer_t lock2(runtimeLock);
        // 准备所有的 load 方法
        prepare_load_methods((const headerType *)mh);
    }

    // Call +load methods (without runtimeLock - re-entrant)
    // 调用 load 方法
    call_load_methods();
}
```

1. 快速查询，类和分类的方法列表中是否含有 `load` 方法，如果没有，直接返回

2. 递归查询所有的 `load` 方法，并存储起来

3. 依次调用所有的 `load` 方法

#### 1.2 prepare_load_methods


```
void prepare_load_methods(const headerType *mhdr)
{
    size_t count, i;

    runtimeLock.assertWriting();

    // 收集所有类的列表
    classref_t *classlist = 
        _getObjc2NonlazyClassList(mhdr, &count);
    
    for (i = 0; i < count; i++) {
        // 收集当前类和父类的 load 方法，父类优先
        
        schedule_class_load(remapClass(classlist[i]));
    }

    // 获取所有的分类
    category_t **categorylist = _getObjc2NonlazyCategoryList(mhdr, &count);
    for (i = 0; i < count; i++) {
        category_t *cat = categorylist[i];
        // 获取到分类对应的类的指针
        Class cls = remapClass(cat->cls);
        // 若链接返回 nil， 如果是 弱链接，就跳过
        if (!cls) continue;  // category for ignored weak-linked class
        
        // 对类进行第一次初始化，包括 读写空间，返回真正的类结构
        realizeClass(cls);
        assert(cls->ISA()->isRealized());
        // 把 分类加入到一个全局列表中
        add_category_to_loadable_list(cat);
    }
}
```

1. 存储当前类和父类的所有 `load` 方法，其中父类优先

2. 存储分类的 `load` 方法

##### 1.2.1 schedule_class_load
> Schedule +load for classes in this image, any un-+load-ed superclasses in other images, and any categories in this image.

* 该方法是递归函数，找到未被加载的最顶级的父类，然后依次存储

```
static void schedule_class_load(Class cls)
{
    if (!cls) return;
    assert(cls->isRealized());  // _read_images should realize

    // A. 判断 类 的 load 方法 是否被调用
    if (cls->data()->flags & RW_LOADED) return;

    // Ensure superclass-first ordering
    schedule_class_load(cls->superclass);

    // 把 含有 load 方法的类 添加到 全局的 loadable_classes
    add_class_to_loadable_list(cls);
    // 添加标记，对应 A
    cls->setInfo(RW_LOADED); 
}
```

##### 1.2.2 add_class_to_loadable_list
> Class cls has just become connected. Schedule it for +load if it implements a +load method

* 存储实现了 `load` 方法的类

```
void add_class_to_loadable_list(Class cls)
{
    // 方法指针
    IMP method;

    
    loadMethodLock.assertLocked();
    
    // 方法内部会根据 方法名字 判断是否 load 方法，并返回
    method = cls->getLoadMethod();
    
    if (!method) return;  // Don't bother if cls has no +load method
    
    if (PrintLoading) {
        _objc_inform("LOAD: class '%s' scheduled for +load", 
                     cls->nameForLogging());
    }
    
    if (loadable_classes_used == loadable_classes_allocated) {
        loadable_classes_allocated = loadable_classes_allocated*2 + 16;
        loadable_classes = (struct loadable_class *)
            realloc(loadable_classes,
                              loadable_classes_allocated *
                              sizeof(struct loadable_class));
    }
    
    loadable_classes[loadable_classes_used].cls = cls;
    loadable_classes[loadable_classes_used].method = method;
    loadable_classes_used++;
}
```

1. 根据 "load" 获取对应方法的指针。其实方法实质上也是个对象，有它自己的成员变量，如下：

    ```
    struct method_t {
        SEL name; 
        const char *types; 
        IMP imp;
    }
    ```

2. 静态全局数组存储，如果数组已满，动态扩容
    * `loadable_classes`：数组，里面元素是结构体 `loadable_class`，存储类名和方法指针。
    * `loadable_classes_used`：数组内对象的个数，即已经存储的对象数量
    * `loadable_classes_allocated`：数组大小

3. `add_category_to_loadable_list` 分类存储方法，几乎一致

### 2. call_load_methods
> Call all pending class and category +load methods.
> Class +load methods are called superclass-first. 
> Category +load methods are not called until after the parent class's +load

* 依次执行已经被存储的 `load` 方法

```
void call_load_methods(void)
{
    // loading 设置为全局静态变量，保证只初始化一次，
    // 一旦执行一次， loading 即为 YES，
    static bool loading = NO;
    bool more_categories;

    loadMethodLock.assertLocked();

    // Re-entrant calls do nothing; the outermost call will finish the job.
    if (loading) return;
    loading = YES;

    // 创建自动释放池，在自动释放池中进行方法调用
    void *pool = objc_autoreleasePoolPush();

    do {
        // 1. Repeatedly call class +loads until there aren't any more
        while (loadable_classes_used > 0) {
            call_class_loads();
        }

        // 2. Call category +loads ONCE
        more_categories = call_category_loads();

        // 3. Run more +loads if there are classes OR more untried categories
    } while (loadable_classes_used > 0  ||  more_categories);

    objc_autoreleasePoolPop(pool);

    loading = NO;
}
```

1. 首先，保证是首次执行， `load` 方法只会执行一次。

2. 创建自动释放池，在池内执行方法，优化性能

3. `do {} while` 循环执行，直到数组为空，且分类方法也执行完毕，不再有新的分类方法

### 3. call_class_loads

* `call_class_loads` 方法比较简单，主要看分类方法的调用，这里涉及到在运行期间，后续又添加的 `load` 分类方法

```
static bool call_category_loads(void)
{
    int i, shift;
    bool new_categories_added = NO;
    
    // Detach current loadable list.
    // 1. 分离并获取当前的分类 列表 cats
    struct loadable_category *cats = loadable_categories;
    int used = loadable_categories_used;
    int allocated = loadable_categories_allocated;
    loadable_categories = nil;
    loadable_categories_allocated = 0;
    loadable_categories_used = 0;

    // Call all +loads for the detached list.
    // 2. for 循环 进行调用 load 方法，执行完毕后，把分类置空
    for (i = 0; i < used; i++) {
        Category cat = cats[i].cat;
        load_method_t load_method = (load_method_t)cats[i].method;
        Class cls;
        if (!cat) continue;

        cls = _category_getClass(cat);
        if (cls  &&  cls->isLoadable()) {
            if (PrintLoading) {
                _objc_inform("LOAD: +[%s(%s) load]\n", 
                             cls->nameForLogging(), 
                             _category_getName(cat));
            }
            (*load_method)(cls, SEL_load);
            cats[i].cat = nil;
        }
    }

    // Compact detached list (order-preserving)
    // 3. 将加载过的分类方法移除 分离列表，保留未被加载过的 分类方法
    shift = 0;
    for (i = 0; i < used; i++) {
        if (cats[i].cat) {
            cats[i-shift] = cats[i];
        } else {
            shift++;
        }
    }
    used -= shift;

    // Copy any new +load candidates from the new list to the detached list.
    // 运行过程中是否有新添加的分类方法
    new_categories_added = (loadable_categories_used > 0);
    
    // 4. 如果有，先存储在 分离列表 cats 
    for (i = 0; i < loadable_categories_used; i++) {
        if (used == allocated) {
            allocated = allocated*2 + 16;
            cats = (struct loadable_category *)
                realloc(cats, allocated *
                                  sizeof(struct loadable_category));
        }
        cats[used++] = loadable_categories[i];
    }

    // Destroy the new list.
    // 释放清空全局列表，以便后面重新赋值
    if (loadable_categories) free(loadable_categories);

    // Reattach the (now augmented) detached list. 
    // But if there's nothing left to load, destroy the list.
    // 5. 是否存在有新列表，并赋值给全局静态存储变量
    if (used) {
        loadable_categories = cats;
        loadable_categories_used = used;
        loadable_categories_allocated = allocated;
    } else {
        if (cats) free(cats);
        loadable_categories = nil;
        loadable_categories_used = 0;
        loadable_categories_allocated = 0;
    }

    if (PrintLoading) {
        if (loadable_categories_used != 0) {
            _objc_inform("LOAD: %d categories still waiting for +load\n",
                         loadable_categories_used);
        }
    }

    return new_categories_added;
}
```

* 上述代码中详细给出了5步，判断是否还有更多的分类方法，来决定是否继续在 `while`循环中执行。

* 这里着重提下，由于 OC 运行时的机制，系统之前已经收集完所有的 `load` 方法，并且正在执行 `load` 方法的时候，又有含有 `load` 方法的分类被添加进来，所以在执行分类的时候，又多出来 3、4、5 步，来保证所有的分类实现完毕。

### 4. load

```
+ (void)load
{
    NSLog(@"Load Hello World");
}
```

## 总结：

从方法调用栈中，找到了系统在执行 `load` 前调用的方法：
    
1. 启动 dyld，将二进制文件初始化

2. `ImageLoader` 把二进制文件加载进内存
3. runtime 执行`load_images`，执行所有的 `load`方法
   * 使用一个全局数组从含有 `load` 方法的根父类到自身，依次添加
   * 使用另一个全局数组添加含有 `load` 方法的所有分类
   * 依次执行存储的 `load` 方法，父类 -> 自身 -> 分类
4. 执行自定义的 `load` 方法
    
    

    

## 参考


[load 的简单了解](http://www.jianshu.com/p/1b8fb16d8a56)

[iOS 程序 main 函数之前发生了什么](http://blog.sunnyxx.com/2014/08/30/objc-pre-main/)


[你真的了解 load 方法么？](https://github.com/Draveness/iOS-Source-Code-Analyze/blob/master/contents/objc/%E4%BD%A0%E7%9C%9F%E7%9A%84%E4%BA%86%E8%A7%A3%20load%20%E6%96%B9%E6%B3%95%E4%B9%88%EF%BC%9F.md)


[load 方法全程跟踪](http://www.desgard.com/Load/)

