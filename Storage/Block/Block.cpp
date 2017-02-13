
struct __block_impl {
  void *isa; // 代表block类型
  int Flags; // 标记变量
  int Reserved; // 保留变量
  void *FuncPtr; // block 执行时调用的函数指针
};

// 创建block 的方法
struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
    // 构造方法
  __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, int flags=0) {
    impl.isa = &_NSConcreteStackBlock; // stack 栈
    impl.Flags = flags; // 默认为0
      
      // fp 指 __main_block_func_0，即在 block 中运行的代码
      // 
    impl.FuncPtr = fp; 
    Desc = desc;  // block 大小信息 desc 指的是 &__main_block_desc_0_DATA
  }
};

// __cself 相当于 OC 中的 self
// block 中执行的方法
static void __main_block_func_0(struct __main_block_impl_0 *__cself) {

        int a;
        a = 1 + 1;
}

static struct __main_block_desc_0 {
  size_t reserved; // 保留字段
  size_t Block_size; // block 大小
} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0)};


int main(int argc, char const *argv[])
{
    
    /** 
    
    struct __main_block_impl_0 tmp = __main_block_impl_0(__main_block_func_0, &__main_block_desc_0_DATA);
    
    struct __main_block_impl_0 *testBlock = &tmp;
    
    
    (testBlock -> FuncPtr)(testBlock);
    
    **/
 void (* testBlock)() = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA));

 ((void (*)(__block_impl *))((__block_impl *)testBlock)->FuncPtr)((__block_impl *)testBlock);


 return 0;
}

