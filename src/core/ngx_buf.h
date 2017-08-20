
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t;

typedef struct ngx_buf_s  ngx_buf_t;

/*
 * Nginx的buf缓冲区数据结构，主要用来存储非常大块的内存。ngx_buf_t数据结构也贯穿了整个Nginx。
 * 这个ngx_buf_t就是这个ngx_chain_t链表的每个节点的实际数据。该结构实际上是一种抽象的数据结构，它代表某种具体的数据。
 * 这个数据可能是指向内存中的某个缓冲区，也可能指向一个文件的某一部分，也可能是一些纯元数据（元数据的作用在于指示这个链表的读取者对读取的数据进行不同的处理）。
 */
struct ngx_buf_s {
    u_char          *pos;           //待处理数据的开始标记
    u_char          *last;          //待处理数据的结尾标记
    off_t            file_pos;      //处理文件时，待处理的文件开始标记
    off_t            file_last;     //处理文件时，待处理的文件结尾标记

    u_char          *start;         /* start of buffer */   //缓冲区开始的指针地址
    u_char          *end;           /* end of buffer */     //缓冲区结尾的指针地址
    ngx_buf_tag_t    tag;           //缓冲区标记地址，是一个void类型的指针。
    ngx_file_t      *file;          //引用的文件
    ngx_buf_t       *shadow;


    /* the buf's content could be changed */
    unsigned         temporary:1;   //标志位，为1时，内存可修改

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
    unsigned         memory:1;      //标志位，为1时，内存只读

    /* the buf's content is mmap()ed and must not be changed */
    unsigned         mmap:1;        //标志位，为1时，mmap映射过来的内存，不可修改

    unsigned         recycled:1;        //标志位，为1时，可回收
    unsigned         in_file:1;         //标志位，为1时，表示处理的是文件
    unsigned         flush:1;           //标志位，为1时，表示需要进行flush操作
    unsigned         sync:1;            //标志位，为1时，表示可以进行同步操作，容易引起堵塞
    unsigned         last_buf:1;        //标志位，为1时，表示为缓冲区链表ngx_chain_t上的最后一块待处理缓冲区
    unsigned         last_in_chain:1;   //标志位，为1时，表示为缓冲区链表ngx_chain_t上的最后一块缓冲区

    unsigned         last_shadow:1;     //标志位，为1时，表示是否是最后一个影子缓冲区
    unsigned         temp_file:1;       //标志位，为1时，表示当前缓冲区是否属于临时文件

    /* STUB */ int   num;
};

/** 
 * 缓冲区链表结构，放在pool内存池上面 
 */ 
struct ngx_chain_s {
    ngx_buf_t    *buf;
    ngx_chain_t  *next;
};


typedef struct {
    ngx_int_t    num;
    size_t       size;
} ngx_bufs_t;


typedef struct ngx_output_chain_ctx_s  ngx_output_chain_ctx_t;

typedef ngx_int_t (*ngx_output_chain_filter_pt)(void *ctx, ngx_chain_t *in);

#if (NGX_HAVE_FILE_AIO)
typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);
#endif

struct ngx_output_chain_ctx_s {
    ngx_buf_t                   *buf;
    ngx_chain_t                 *in;
    ngx_chain_t                 *free;
    ngx_chain_t                 *busy;

    unsigned                     sendfile:1;
    unsigned                     directio:1;
#if (NGX_HAVE_ALIGNED_DIRECTIO)
    unsigned                     unaligned:1;
#endif
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;
#if (NGX_HAVE_FILE_AIO)
    unsigned                     aio:1;

    ngx_output_chain_aio_pt      aio_handler;
#endif

    off_t                        alignment;

    ngx_pool_t                  *pool;
    ngx_int_t                    allocated;
    ngx_bufs_t                   bufs;
    ngx_buf_tag_t                tag;

    ngx_output_chain_filter_pt   output_filter;
    void                        *filter_ctx;
};


typedef struct {
    ngx_chain_t                 *out;
    ngx_chain_t                **last;
    ngx_connection_t            *connection;
    ngx_pool_t                  *pool;
    off_t                        limit;
} ngx_chain_writer_ctx_t;


#define NGX_CHAIN_ERROR     (ngx_chain_t *) NGX_ERROR


#define ngx_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)
#define ngx_buf_in_memory_only(b)   (ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_special(b)                                                   \
    ((b->flush || b->last_buf || b->sync)                                    \
     && !ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_sync_only(b)                                                 \
    (b->sync                                                                 \
     && !ngx_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)

#define ngx_buf_size(b)                                                      \
    (ngx_buf_in_memory(b) ? (off_t) (b->last - b->pos):                      \
                            (b->file_last - b->file_pos))

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);
ngx_chain_t *ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs);


#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t *ngx_alloc_chain_link(ngx_pool_t *pool);
#define ngx_free_chain(pool, cl)                                             \
    cl->next = pool->chain;                                                  \
    pool->chain = cl



ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in);
ngx_int_t ngx_chain_writer(void *ctx, ngx_chain_t *in);

ngx_int_t ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in);
ngx_chain_t *ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free);
void ngx_chain_update_chains(ngx_chain_t **free, ngx_chain_t **busy,
    ngx_chain_t **out, ngx_buf_tag_t tag);


#endif /* _NGX_BUF_H_INCLUDED_ */
