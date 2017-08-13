
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

/*
 * 对下面的 ngx_list_part_s 结构体做了一个类型定义，新类型名定义为 ngx_list_part_t 
 */
typedef struct ngx_list_part_s  ngx_list_part_t;
/*
 * 链表中的一个元素 
 */
struct ngx_list_part_s {
    void             *elts;     //指向数组的起始地址
    ngx_uint_t        nelts;    //表示数组中已经使用了多少个元素，nelts必须小于 ngx_list_t 结构体中的nalloc
    ngx_list_part_t  *next;     //下一个链表元素 ngx_list_part_t 的地址
};

/*
 * 链表容器，描述整个链表 
 */
typedef struct {
    ngx_list_part_t  *last;     //指向链表的最后一个元素
    ngx_list_part_t   part;     //链表的首个元素
    size_t            size;     //限制数组里面的每个项占用的空间大小。用户存储一个数据所占用的字节数必须小于等于size
    ngx_uint_t        nalloc;   //每个ngx_list_part_t 数组的容量，即最多能存多少个数据
    ngx_pool_t       *pool;     //链表中管理内存分配的内存池对象。用户要存放的数据占用的内存都是由pool分配的
} ngx_list_t;


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
