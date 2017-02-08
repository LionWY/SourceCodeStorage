# Block 详解

## Block 的本质

1. `block` 本身也是对象，拥有指针 `isa`

2. 


## 截获自动变量值

1. block 语法表达式中使用的自动变量被作为成员变量追加到了 `__main_block_impl_0` 结构体中

2. Block 中所使用的被截获自动变量就如“带有自动变量值的匿名函数”所说，仅截获自动变量的值。
3. Block 中使用自动变量后，在 Block 的结构体实例中重写该自动变量也不会改变原先截获的自动变量

## __block 说明符

1. 可在 block 中进行修改的三种变量
    * 静态变量
    * 静态全局变量
    * 全局变量

2. 在由 Block 语法生成的值 Block 上，可以存有超过其变量作用域的被截获对象的自动变量。
3. 变量作用域结束的同时，原来的自动变量被废弃
4. 因此 Block 中超过变量作用域而存在的变量同静态变量一样，将不能通过指针访问原来的自动变量

5. `__block` 变成了结构体



## block 存储区域

1. block 存储在程序的数据区域 -`NSConcreteGlobalBlock`
    * 记述全局变量的地方有 block 语法
    * block 语法表达式中没有截获自动变量

2. 全局 Block：从变量作用域外也可以通过指针安全地使用
3. 设置在栈上的 Block：如果其所属的变量作用域结束，该 Block就被废弃



5. 编译器自动识别，把 block 从栈上复制到堆上
    * 向方法或函数的参数中传递 Block 时
    * Cocoa 框架的方法且方法名中含有 usingBlock 等时
    * GCD 的 API

6. 

## __block 变量存储域

1. 当把 Block 从栈复制到堆上时，__block 变量也会从栈复制到堆，并被 该 Block 持有，但是栈上的 __block 结构体实例的成员变量 `__forwarding` 指向的是复制到堆上的 __block 变量

4. __block 变量用结构成员变量 __forwarding 可以实现无论 __block 变量设置在栈上还是堆上，都能够正确的访问 __block 变量

## 截获对象

1. `copy`：
    * 栈上的 Block 复制到堆时
    * 将对象赋值在对象类型的结构体的成员变量中

2. `dispose`：
    * 堆上的 Block 被废弃时
    * 释放赋值在 Block 用结构体成员变量中的对象

3. 栈上的 Block 复制到堆的情况：
    * 调用 Block 的 copy 实例方法
    * Block 作为函数返回值
    * 将 Block 赋值给附有 `__strong` 修饰符 id 类型的类或 Block 类型成员变量
    * 方法命中含有 `usingBlock` 的Cocoa 框架方法或 GCD 的 API 中传递参数 Block
4. `BLOCK_FIELD_IS_BYREF` 对应于 __block 变量

5. `BLOCK_FIELD_IS_OBJECT` 对应于 对象

6. Block 中使用的赋值给附有 __strong 修饰符的自动变量的对象和复制到堆上的 __block 变量由于被堆上的 Block 所持有，因而可超过其变量作用域而存在



## __block 变量和对象

1. arc 下，默认添加 `__strong` 修饰符

## Block 循环引用

1. __block 优点：
    * 通过 __block 变量可控制对象的持有期间
    * 在执行 Block 时可动态地决定是否将 nil 或其他对象赋值在 __block 变量中

2. __block 缺点
    * 为避免循环引用必须执行 Block

## copy/release

1. ARC 无效时，__block 说明符被用来避免 Block 中的循环引用
    * 当 Block从栈复制到堆时，若 Block 使用的变量为附有 __block 说明符的 id 类型或对象类型的自动变量，不会被 retain
    * 反之会被 retain





[block 底层解析](http://www.jianshu.com/p/51d04b7639f1)


[libclosure-67](https://opensource.apple.com/source/libclosure/libclosure-67/)


