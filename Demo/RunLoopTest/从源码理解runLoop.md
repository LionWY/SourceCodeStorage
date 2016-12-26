## 目的：
1. 管理线程，让线程在没有消息处理时休眠以避免资源占用、有消息时立刻被唤醒，进行消息处理

2. RunLoop是一个对象，该对象管理了其需要处理的事件和消息，并提供了入口函数来管理逻辑。

3. 线程执行了该函数后，就会处于接收消息->等待->处理的循环中，直至循环结束，函数返回

## NSRunLoop && CFRunLoopRef
> CFRunLoopRef 是在coreFoundation框架内的，提供了纯C函数的API，并且是线程安全的
> NSRunLoop 是基于CFRunLoopRef的封装，提供了面向对象的API，但不是线程安全的


