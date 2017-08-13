
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

/*
 * 创建nginx内存池
 * @param size  内存池头部（包括收个内存数据块可分配内存）的大小
 * @param *log  
 */
ngx_pool_t *
ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;     //内存池指针，指向一个内存池。

    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);    // 分配数据对齐的内存地址，地址必须是16的倍数，也就是二进制地址的最后四位是0。NGX_POOL_ALIGNMENT 是内存池校准对齐常量，值为 16
    if (p == NULL) {
        return NULL;
    }
    //sizeof(ngx_pool_t) = 40B	sizeof(ngx_pool_data_t) = 16B
    //   结构体指针用 -> 引用结构体内的变量。结构体变量则用 . 引用结构体内的变量
    //(u_char *) p  是强制将p转换为无符号字符的指针，这样 p->d.last 也就是无符号字符指针了。
    p->d.last = (u_char *) p + sizeof(ngx_pool_t);  //第一个内存数据块的下一内存分配起始地址，指向内存池头的末尾（可以参看内存池的结构图理解http://abumaster.com/2017/06/08/Nginx%E6%BA%90%E7%A0%81%E5%AD%A6%E4%B9%A0-%E5%86%85%E5%AD%98%E6%B1%A0/）
    p->d.end = (u_char *) p + size; //指向内存池数据块的结尾。此处size应该大于内存头的大小40B
    p->d.next = NULL;   //下一个内存数据块暂时没分配
    p->d.failed = 0;    //分配内存失败次数初始化为0

    size = size - sizeof(ngx_pool_t);   //整个创建内存池的分配的内存大小 - 内存池头部结构体占用的空间 = 第一个数据块可以分配最大内存字节数

    // #define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)
    // ngx_pagesize = getpagesize()   当前系统一页内存是多少字节
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL; //如果第一个数据块的内存字节数 小于系统一页内存的容量，则就是当前数据块的字节数，否则max是当前系统一页内存的容量。  即max 最大只能是当前系统一页内存的字节数减1

    p->current = p; //指向当前内存池
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}


void
ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

    for (l = pool->large; l; l = l->next) {

        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);

        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

#if (NGX_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}


void
ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    pool->large = NULL;

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    }
}

/*
 * 从内存池中分配内存
 * @param *pool 内存池指针
 * @param size  分配的大小
 */
void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    ngx_pool_t  *p;

    if (size <= pool->max) { //如果分配的内存小于内存池数据块的大小（最大是系统内存一页的大小）

        p = pool->current;

        do {
            //ngx_align_ptr 是内存对齐宏。该处返回一个内存对齐的开始地址
            m = ngx_align_ptr(p->d.last, NGX_ALIGNMENT);    //#define NGX_ALIGNMENT sizeof(unsigned long)

            if ((size_t) (p->d.end - m) >= size) {  //如果内存数据库的尾部地址 减去 分配的内存对齐的开始地址 大于 要分配的内存大小。则说明剩余的内存足够分配。
                p->d.last = m + size;   //可分配的内存的开始指针(last)指到 分配的对齐地址+分配的大小 的位置。下次分配从这里开始继续分配

                return m;   //返回此次分配的内存的首地址。
            }
            //如果本内存数据块不够分配的。则指向当前待分配的内存数据块的指针p 指到下一个内存数据块。如果下一个内存数据块存在，则继续下个循环，进行内存分配，否则调用ngx_palloc_block
            p = p->d.next;

        } while (p);

        //如果遍历完内存池还是没有合适的大小的可以分配。则新建一个内存数据块挂接到内存池上，并分配相应的内存。
        return ngx_palloc_block(pool, size);
    }

    //如果要分配的内存大小大于内存池限制分配的最大值。则分配大内存块
    return ngx_palloc_large(pool, size);
}

/*
 * 从内存池申请内存
 * @desc 用户可以调用ngx_palloc()向内存池申请内存，这个函数会判断用户申请的是小块内存还是大块内存。如果用户申请的是小块内存，那么就会从小块内存链表中查找是否有满足用户需求的小块内存，如果没找到，则创建一个新的小块内存。
 * @param *pool     要申请内存的内存池指针
 * @param size      要申请的内存大小
 */
void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    ngx_pool_t  *p;

    if (size <= pool->max) {    //如果申请内存大小 小于 每个数据块最大的申请大小（申请小内存），则继续想像内存池申请小块内存，否则去申请一块大块内存。

        p = pool->current;

        do {
            m = p->d.last;

            if ((size_t) (p->d.end - m) >= size) {  //如果内存块中剩余可分配的内存 大于 要申请的内存，则分配成功，返回分配的内存首地址
                p->d.last = m + size;

                return m;
            }
            //如果当前内存数据块没有足够的可分配内存，则指向下一个内存数据块，继续查找下一个内存数据块是否有合适大小的内存可以分配
            p = p->d.next;

        } while (p);

        //如果整个当前内存池中的所有数据块都没有满足的内存尺寸可以分配，则重新创建一个新的内存数据块,并返回在新建数据块上申请的内存首地址。
        return ngx_palloc_block(pool, size);    
    }

    //如果申请内存大小  大于  每个数据块的最大申请容量，则去申请大块内存
    return ngx_palloc_large(pool, size);
}

/*
 * 创建新的小块内存
 *
 * @desc 创建新的小块内存，并插入到小块内存链表的末尾：
 * @param *pool 要插入的内存池
 * @param size 创建新的小块内存数据块后，返回在新数据块上要申请的内存大小。（这里的size一定得搞明白，并不要新创建内存数据块的大小）
 */
static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new, *current;

    psize = (size_t) (pool->d.end - (u_char *) pool);   //获取当前内存池的头结构（包括第一个内存数据块的结构以及总共可分配空间，在加上内存池头结构的几个变量） 所占内存大小

    //这里要注意的是，新分配的小内存数据块，ngx_pool_data_t 结构变量和数据区整体占用的容量，是和内存池第一块大小一样，但是内存池的第一块还包括内存池整体的几个变量 max,current,large,cheanup,log等。因此内存池链表的后边的数据块都要比内存池头上的数据块要大一点（这里大家不搞明白可能不好理解代码）。
    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log); // 分配数据对齐的内存地址，地址必须是16的倍数，也就是二进制地址的最后四位是0。NGX_POOL_ALIGNMENT 是内存池校准对齐常量，值为 16
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m; //将新申请到的内存指针，转换为内存池类型的指针，这样就可以操作内存池内的结构了。

    //给新小内存数据块的各个结构变量赋值。
    new->d.end = m + psize; //end 指向新内存数据块的总的可分配内存的末尾地址
    new->d.next = NULL; //本内存数据块是链表的最后一个，因此赋值为NULL
    new->d.failed = 0;  //当前是新的数据块，分配失败次数初始化为0

    m += sizeof(ngx_pool_data_t);   //m指针指向 ngx_pool_data_t结构体变量的末尾，即数据区的开始位置
    m = ngx_align_ptr(m, NGX_ALIGNMENT);    //找到根据 ngx_align_ptr宏进行内存对齐后的地址，作为数据块数据区的开始位置。
    new->d.last = m + size; //新内存数据块的下次内存分配的起始地址重新赋值（此处在新创建的内存数据块上直接分配了size的内存并最后返回给当前的函数调用者）

    current = pool->current;

    //循环遍历整个内存池链表。每个数据块的分配次数加一。如果当前数据块的分配失败次数大于4次，则将current指针指向它的下一个数据块
    for (p = current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            current = p->d.next;
        }
    }

    //经过上面的循环p指针已经指向链表最后一个数据块。下面就是把新创建的new数据块，挂到链表最后一个数据块上。
    p->d.next = new;

    //内存池的current指针，指向前面的一个fail次数小于等于4的数据块节点。如果整个链表都fail都超过4次了，则指向新创建的new节点。（也就是内存池的current总是指向失败次数小于等于4次的数据节点）
    pool->current = current ? current : new;

    return m;   //返回在新建数据块上申请到的容量为size的内存首地址，这个m指针是 u_char类型的。


/*
 * 分配大块内存
 *
 * @desc 
 * @param *pool 
 * @param size 
 */
static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;   //指向大块内存链表的某个节点的指针

    p = ngx_alloc(size, pool->log); //申请一块size字节的普通内存。调用malloc函数
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    // 遍历大块内存链表，找到可以挂载 p大块内存 的位置。（因为创建新的大块内存都是挂接到链表的开头，所以这里限制最多往后找三次。这里的问题是，什么情况下回出现存在某个大块结构体变量，但是这个结构体变量的alloc变量是 NULL?）
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = ngx_palloc(pool, sizeof(ngx_pool_large_t)); //在小内存池上分配一块内存，存放大内存结构变量（这个乍看比较容易看错）。
    //如果分配小内存失败，则把申请到的p指针指向的内存free掉。 p是局部变量，存储在栈上，因此不需要将其赋值为 NULL 。但是需要将p指向的内存空间 free掉
    if (large == NULL) {
        ngx_free(p);    // #define ngx_free          free
        return NULL;
    }

    large->alloc = p;   //将大块内存结构体变量（large）的 alloc变量，指向开头申请到的大块内存p。

    //将内存池的large指针，指向刚申请到的large结构体变量（即在大块内存链表的开头插入新分配的大块内存结构体变量）
    large->next = pool->large;  
    pool->large = large;    

    return p;
}


void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;

    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    large = ngx_palloc(pool, sizeof(ngx_pool_large_t));
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


ngx_int_t
ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            ngx_free(l->alloc);
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}


void
ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}


void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (ngx_delete_file(c->name) == NGX_FILE_ERROR) {
        err = ngx_errno;

        if (err != NGX_ENOENT) {
            ngx_log_error(NGX_LOG_CRIT, c->log, err,
                          ngx_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif
