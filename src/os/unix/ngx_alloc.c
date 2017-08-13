
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


ngx_uint_t  ngx_pagesize;
ngx_uint_t  ngx_pagesize_shift;
ngx_uint_t  ngx_cacheline_size;

/*
 * nginx对malloc的简单封装
 * @param size 要分配的内存大小
 * @param *log nginx日志
 */
void *
ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "malloc(%uz) failed", size);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

    return p;
}

/*
 * nginx自己封装的calloc
 * @desc 这个有点奇怪，原来的calloc 是有两个参数指明数组元素个数，和每个元素的大小。这个封装没有数组的概念了。
 * @param size 要分配的内存大小
 * @param *log nginx日志
 */
void *
ngx_calloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = ngx_alloc(size, log);

    if (p) {
        ngx_memzero(p, size);   // #define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
    }

    return p;
}



/*
 * nginx对 memalign或者posix_memalign 的封装。
 * @desc 申请数据对齐的内存地址。根据系统不同，判断是有 posix_memalign 还是 memalign
 * @param alignment 对齐字节长度（单位：字节）。分配的内存地址需要是 alignment 的倍数
 * @param size 要分配的内存大小
 * @param *log nginx日志
 */
#if (NGX_HAVE_POSIX_MEMALIGN)

void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);  //posix_memalign c的标准库函数，成功时会返回size字节的动态内存,并且这块内存的地址是alignment的倍数。数据对齐的概念（http://blog.csdn.net/wallwind/article/details/7461701）

    if (err) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#elif (NGX_HAVE_MEMALIGN)

void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;

    p = memalign(alignment, size);  //和posix_memalign类似，不同是其将分配好的内存块首地址做为返回值。 在GNU系统中，malloc或realloc返回的内存块地址都是8的倍数（如果是64位系统，则为16的倍数）。如果你需要更大的粒度，请使用memalign或valloc。这些函数在头文件“stdlib.h”中声明。
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "memalign(%uz, %uz) failed", alignment, size);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif
