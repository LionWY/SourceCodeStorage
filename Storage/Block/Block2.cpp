
#ifndef BLOCK_IMPL
#define BLOCK_IMPL
struct __block_impl {
  void *isa;
  int Flags;
  int Reserved;
  void *FuncPtr;
};
// Runtime copy/destroy helper functions (from Block_private.h)
#ifdef __OBJC_EXPORT_BLOCKS
extern "C" __declspec(dllexport) void _Block_object_assign(void *, const void *, const int);
extern "C" __declspec(dllexport) void _Block_object_dispose(const void *, const int);
extern "C" __declspec(dllexport) void *_NSConcreteGlobalBlock[32];
extern "C" __declspec(dllexport) void *_NSConcreteStackBlock[32];
#else
__OBJC_RW_DLLIMPORT void _Block_object_assign(void *, const void *, const int);
__OBJC_RW_DLLIMPORT void _Block_object_dispose(const void *, const int);
__OBJC_RW_DLLIMPORT void *_NSConcreteGlobalBlock[32]; // 设置在程序的数据区域（.data 区）
__OBJC_RW_DLLIMPORT void *_NSConcreteStackBlock[32];
#endif
#endif
#define __block
#define __weak

#include <stdarg.h>
struct __NSContainer_literal {
  void * *arr;
  __NSContainer_literal (unsigned int count, ...) {
	va_list marker;
	va_start(marker, count);
	arr = new void *[count];
	for (unsigned i = 0; i < count; i++)
	  arr[i] = va_arg(marker, void *);
	va_end( marker );
  };
  ~__NSContainer_literal() {
	delete[] arr;
  }
};
extern "C" __declspec(dllimport) void * objc_autoreleasePoolPush(void);
extern "C" __declspec(dllimport) void objc_autoreleasePoolPop(void *);

struct __AtAutoreleasePool {
  __AtAutoreleasePool() {atautoreleasepoolobj = objc_autoreleasePoolPush();}
  ~__AtAutoreleasePool() {objc_autoreleasePoolPop(atautoreleasepoolobj);}
  void * atautoreleasepoolobj;
};

#define __OFFSETOFIVAR__(TYPE, MEMBER) ((long long) &((TYPE *)0)->MEMBER)
int global_val = 1;
static int static_global_val = 2;

struct __Block_byref_val_0 {
  void *__isa;
__Block_byref_val_0 *__forwarding;
 int __flags;
 int __size;
 int val;
};

struct __main_block_impl_0 {
  struct __block_impl impl;
  struct __main_block_desc_0* Desc;
  int *static_val;
  __Block_byref_val_0 *val; // by ref
    
  __main_block_impl_0(void *fp, struct __main_block_desc_0 *desc, int *_static_val, __Block_byref_val_0 *_val, int flags=0) : static_val(_static_val), val(_val->__forwarding) {
    impl.isa = &_NSConcreteStackBlock;
    impl.Flags = flags;
    impl.FuncPtr = fp;
    Desc = desc;
  }
};
static void __main_block_func_0(struct __main_block_impl_0 *__cself) {
  __Block_byref_val_0 *val = __cself->val; // bound by ref
  int *static_val = __cself->static_val; // bound by copy


        global_val *= 1;
        static_global_val *= 2;
        (*static_val) *= 3;

        (val->__forwarding->val) = 1;
    }
static void __main_block_copy_0(struct __main_block_impl_0*dst, struct __main_block_impl_0*src) {
    // 将对象赋值在对象类型的结构体成员变量中
    _Block_object_assign((void*)&dst->val, (void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);}

static void __main_block_dispose_0(struct __main_block_impl_0*src) {
    // 释放赋值在 Block 用结构体成员变量 中的对象
    _Block_object_dispose((void*)src->val, 8/*BLOCK_FIELD_IS_BYREF*/);}

static struct __main_block_desc_0 {
  size_t reserved;
  size_t Block_size;
  void (*copy)(struct __main_block_impl_0*, struct __main_block_impl_0*);
  void (*dispose)(struct __main_block_impl_0*);
} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0), __main_block_copy_0, __main_block_dispose_0};
int main()
{

    static int static_val = 3;

    // 初始化 
    /*
        __isa = 0
        __forwarding = &val
        __flags = 0
        __size = sizeof(__Block_byref_val_0)
        val = 10
     
     */
    
    __attribute__((__blocks__(byref))) __Block_byref_val_0 val = {(void*)0,(__Block_byref_val_0 *)&val, 0, sizeof(__Block_byref_val_0), 10};

    void (* testBlock)() = ((void (*)())&__main_block_impl_0((void *)__main_block_func_0, &__main_block_desc_0_DATA, &static_val, (__Block_byref_val_0 *)&val, 570425344));

    ((void (*)(__block_impl *))((__block_impl *)testBlock)->FuncPtr)((__block_impl *)testBlock);



    return 0;
}
static struct IMAGE_INFO { unsigned version; unsigned flag; } _OBJC_IMAGE_INFO = { 0, 2 };
