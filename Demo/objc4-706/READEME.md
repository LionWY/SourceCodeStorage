# Autorelease

> 一个对象调用 `autorelease` 方法，就会被自动添加到最近的自动释放池 `autoreleasepool`，只有当自动释放池被销毁的时候，才会执行 `release` 方法，进行释放。

* 下文源码是当前最新版本[objc4-706.tar.gz](https://opensource.apple.com/tarballs/objc4/)，跟之前版本会有点差别，没有了特别牛的`POOL_SENTINEL`（哨兵对象），改为了`POOL_BOUNDARY`（边界对象），可能更好理解一点。


## 1. 入口 

> runtime 的 `NSObject.mm` 文件中 `autorelease` 方法

```
- (id)autorelease {
    return ((id)self)->rootAutorelease();
}

objc_object::rootAutorelease()
{
    return rootAutorelease2();
}

objc_object::rootAutorelease2()
{
    return AutoreleasePoolPage::autorelease((id)this);
}
static inline id autorelease(id obj)
{
   id *dest __unused = autoreleaseFast(obj);
   return obj;
}
```


## 2. AutoreleasePoolPage

1. 关键词：
    * `EMPTY_POOL_PLACEHOLDER`
        * 当一个释放池没有包含任何对象，又刚好被推入栈中，就存储在TLS（Thread_local_storage）中，叫做空池占位符
    * `POOL_BOUNDARY`
        * 边界对象，代表 `AutoreleasePoolPage` 中的第一个对象

2. 结构：

    ```
    class AutoreleasePoolPage 
    {
        
        magic_t const magic; // 当前类完整性的校验
        id *next;
        pthread_t const thread; // 当前类所处的线程
        
        // 双向链表 父子指针
        AutoreleasePoolPage * const parent;
        AutoreleasePoolPage *child;
        
        uint32_t const depth;
        uint32_t hiwat;
        
        static size_t const SIZE = PAGE_MAX_SIZE; // 4096
    }
    ```
    * 一个 poolPage 对应于一个线程
    * `AutoreleasePoolPage` 是以 **双向链表** 的形式连接起来的，其中 `parent` `child` 分别是父结点（指向父类poolPage的指针）和子结点（指向子类poolPage的指针）
    * 一个 poolPage 的大小 是 4096 字节。其中56 bit 用来存储其成员变量，剩下的存储加入到自动释放池中的对象。
        * 调用 `autorelease` 的对象
        * 声明在`@autoreleasepool{}` 中的对象
    * next 指向最新添加进来的对象所处的位置。


### autoreleaseFast()


```
AutoreleasePoolPage *page = hotPage();
 
if (page && !page->full()) {
return page->add(obj);
} else if (page) {
return autoreleaseFullPage(obj, page);
} else {
return autoreleaseNoPage(obj);
}
```

1. `hotPage()` 获取当前正在使用的 poolPage

2. 如果 page 存在，并且未满，直接进行添加
3. 如果 page 存在，但是当前 poolPage 已满：
    * 根据 page 遍历其所有的子 page，直到找到一个未满的子 page
    * 否则就根据最后一个子 page，创建一个新的 page

        ```
        do {
          if (page->child) page = page->child;
          else page = new AutoreleasePoolPage(page);
        } while (page->full());
        ```
        
    * 设置为 hotPage `setHotPage(page);`
    * 添加 `return page->add(obj);`

4. 如果 page 不存在
    
    ```
    AutoreleasePoolPage *page = new AutoreleasePoolPage(nil);
    setHotPage(page);   
    page->add(POOL_BOUNDARY);
    return page->add(obj);
    ```
    * 创建第一个 page，第一个 page 是没有父结点的
    * 设置为当前正在使用的 page 
    * 添加边界对象，初始化的第一个poolPage 存储的第一个对象，必定是边界对象 `POOL_BOUNDARY`
    * 添加对象

### page -> add() 
> 添加对象

```
id *ret = next;  
*next++ = obj;
```

1. 获取 next 指针所处位置，并向上移动
2. 把对象添加到 next 所处位置

### 小结：
一个对象，调用 `autorelease` 方法，实际是把该对象加入到当前线程正在使用的 `autoreleasePoolPage` 的栈中，但它怎么释放呢？ARC 下唯一能看到 `autorelease` 的就是入口函数 `main.m` 中的 `@autoreleasepool{}`。

## 3. @autoreleasepool{}


```
int main(int argc, char * argv[]) {  
   @autoreleasepool {
       
       return 0;
   }
}
```

* 使用 `clang -rewrite-objc main.m`重新编译，结果如下：

    ```
    struct __AtAutoreleasePool {
      __AtAutoreleasePool() {atautoreleasepoolobj = objc_autoreleasePoolPush();}
      ~__AtAutoreleasePool() {objc_autoreleasePoolPop(atautoreleasepoolobj);}
      void * atautoreleasepoolobj;
    };
    
    int main(int argc, char * argv[]) {
        /* @autoreleasepool */ { __AtAutoreleasePool __autoreleasepool; 
    
            return 0;
        }
    }
    ```
    
    * `@autoreleasepool{}` 实际调用的是两个方法`objc_autoreleasePoolPush` 和 `objc_autoreleasePoolPop`
    * objc_autoreleasePoolPush 方法内部实际就是 2.1， `autoreleaseFast()` 函数，不过，参数是 `POOL_BOUNDARY` 边界对象
    * 每创建一个自动释放池，就会在当前线程的 poolPage 的栈中先添加一个边界对象，然后把池中的对象添加进去，直至栈满，创建子 page，继续添加。



### objc_autoreleasePoolPop()

* 该方法内部实际调用的是 `AutoreleasePoolPage` 的 `pop()` 方法，主要代码如下

    ```
    static inline void pop(void *token) 
    {
       AutoreleasePoolPage *page;
       id *stop;
    
         // 根据指针 token 获取token所在的 page
       page = pageForPointer(token);
       stop = (id *)token;
       
        /*
         *   如果stop 不是边界对象，进行其他处理
         *   如果stop 是边界对象，进行下面的步骤，来释放
         */
       
        // 释放栈中的所有对象，直至stop，
        // 此时，stop是边界对象，即 创建 autoreleasepool 时      
        // 添加的第一个对象
       page->releaseUntil(stop);
    
        if (page->child) {
           // hysteresis: keep one empty child if page is more than half full
           // 不足一半满， 删除所有的子 page，
           // 否则 删除所有的孙 page
           if (page->lessThanHalfFull()) {
               page->child->kill();
           }
           else if (page->child->child) {
               page->child->child->kill();
           }
        }
    }
    ```

    * stop 是边界对象，即创建 `autoreleasepool` 时 page 栈中添加的第一个对象

    * `page->releaseUntil(stop)` 方法，会对 page 栈中 stop 上面的所有对象调用 `objc_release(obj)` 方法，除了边界对象 `POOL_BOUNDARY`   

    * `kill()` 方法，会删除当前 page 及其所有的子 page， 即 `page->child = nil;`

### 小结:

1. 对象释放的过程：首先找到当前对象所处的 AutoreleasePoolPage，然后获取到创建 page 时添加的第一个边界对象 `POOL_BOUNDARY`，边界对象之后的添加进来的所有对象调用 `release`，进行释放。
2. 创建自动释放池 `@autoreleasepool{}`的过程：获取到正在使用的 AutoreleasePoolPage，首先添加边界对象 `POOL_BOUNDARY`，如果有其他对象，则进行添加。最后进行释放，同上1。


## 总结：

1. `autoreleasepool` 机制跟栈一样，每创建一个，就将其推入栈中，而清空 `autoreleasepool`，相当于将其从栈中弹出

2. 一个对象调用 `autorelease` 方法，就会进入最近的 `autoreleasepool`，也就是栈顶的那个。

3. 自动释放池的创建：

    * 长时间在后台运行的任务，入口函数(`main.m`)，系统自动创建 `autoreleasepool`
    
    * 主线程或GCD中的线程，会自动创建 `autoreleasepool`, 关系是一对多的关系，即一个线程可以有多个 `autoreleasepool`
    
    * 人为手动创建 `@autoreleasepool{}`
        * 程序不是基于 UI framework，如命令行
        * 创建大量的临时对象，降低内存峰值
        * 创建了新线程

4. 事件开始前，系统会自动创建 `autoreleasepool`， 然后在结束时，进行 `drain`，对释放池内的所有对象执行 `release` 方法


## 参考：
1. 理论：

    * [NSAutoreleasePool](https://developer.apple.com/reference/foundation/nsautoreleasepool#//apple_ref/occ/cl/NSAutoreleasePool)
    
    * [以自动释放池降低内存峰值](https://github.com/LionWY/Read_Notes/blob/master/Effective%20Objective-C%202.0%20%E7%BC%96%E5%86%99%E9%AB%98%E8%B4%A8%E9%87%8FiOS%E4%B8%8EOS%20X%E4%BB%A3%E7%A0%81%E7%9A%8452%E4%B8%AA%E6%9C%89%E6%95%88%E6%96%B9%E6%B3%95/34.%E4%BB%A5%E8%87%AA%E5%8A%A8%E9%87%8A%E6%94%BE%E6%B1%A0%E5%9D%97%E9%99%8D%E4%BD%8E%E5%86%85%E5%AD%98%E5%B3%B0%E5%80%BC.md#以自动释放池降低内存峰值)
    
    * [iOS中autorelease的那些事儿](http://www.jianshu.com/p/5559bc15490d)

2. 实践：
    * [Objective-C Autorelease Pool 的实现原理](http://blog.leichunfeng.com/blog/2015/05/31/objective-c-autorelease-pool-implementation-principle/#jtss-tsina)
    
    * [自动释放池的前世今生](http://draveness.me/autoreleasepool/)
    
    * [黑幕背后的Autorelease](http://blog.sunnyxx.com/2014/10/15/behind-autorelease/)

