
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;
    void                 *data;
    ngx_pool_cleanup_t   *next;
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;

struct ngx_pool_large_s {
    ngx_pool_large_t     *next;
    void                 *alloc;
};

/*
 * 内存池数据块结构
 */
typedef struct {
    u_char               *last;     //当前内存池数据块分配到此处，下一次分配从这里继续分配
    u_char               *end;      //当前内存池数据块的结束为止。 last指针必须在 end指针的范围之内
    ngx_pool_t           *next;     //指向下一个内存池数据块。这里为什么用 ngx_pool_t 而不是用 ngx_pool_data_t 声明指针？因为这里 ngx_pool_t 结构体的开头部分是 ngx_pool_data_t ，因此 ngx_pool_t 的指针可以用于操作 ngx_pool_data_t结构体变量。ngx_pool_t可以理解为一个”父结构体“
    ngx_uint_t            failed;   //内存块分配的时候大小不够，导致分配失败的次数。
} ngx_pool_data_t;

/*
 * 内存池头部结构。它开头包含一个内存池数据块结构
 * typedef struct ngx_pool_s        ngx_pool_t;
 */
struct ngx_pool_s {
    ngx_pool_data_t       d;        //内存池数据块
    size_t                max;      //内存池数据块的最大值。  size_t 是 unsigned long
    ngx_pool_t           *current;  //指向当前的内存池
    ngx_chain_t          *chain;    //该指针挂接一个ngx_chain_t结构  
    ngx_pool_large_t     *large;    //指向大块内存块链表。大于内存池数据块的数据，单独申请大块内存块
    ngx_pool_cleanup_t   *cleanup;  //清理数据的回调函数
    ngx_log_t            *log;      //日志指针
};


typedef struct {
    ngx_fd_t              fd;
    u_char               *name;
    ngx_log_t            *log;
} ngx_pool_cleanup_file_t;


void *ngx_alloc(size_t size, ngx_log_t *log);
void *ngx_calloc(size_t size, ngx_log_t *log);

ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);


#endif /* _NGX_PALLOC_H_INCLUDED_ */
