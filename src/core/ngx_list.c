
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

/*
 * 创建新的链表
 * @desc
 * @param *pool 内存池指针
 * @param n  每个链表元素的数组元素的个数
 * @param size  每个链表元素的数组中的每个元素的大小
 * @return list 返回新创建的 ngx_list_t 链表的指针
 */
ngx_list_t *
ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    ngx_list_t  *list;  //声明一个指向新建链表的指针

    list = ngx_palloc(pool, sizeof(ngx_list_t));    //从内存池中分配一块内存，内存的大小是 ngx_list_t结构占用的大小
    // 如果分配成功，则 list即指向了新创建的内存，指向了一个新创建的ngx_list_t结构体变量，可以对ngx_list_t 结构体的成员变量进行操作赋值
    if (list == NULL) {
        return NULL;
    }

    list->part.elts = ngx_palloc(pool, n * size);   //从内存池中分配一个 n*size 大小的内存作为链表的首个元素的数组数据区的起始地址
    if (list->part.elts == NULL) {
        return NULL;
    }

    list->part.nelts = 0;   //链表首个元素的数组中已经使用了多少个元素，初始化为0
    list->part.next = NULL; //下一个链表元素为NULL，即没有下一个链表元素
    list->last = &list->part;   //list->part是一个变量，不是指针，链表的首个元素直接作为链表结构的一个成员变量。这里就是取list->part的地址，赋给list->last。即list->last 指针，指向链表的首个元素
    list->size = size;  //限制链表元素中的数组中每个元素的大小最大为 size
    list->nalloc = n;   //限制链表元素中的数组的元素个数，最大为 n
    list->pool = pool;  //链表中管理内存分配的内存池，指向创建列表传入的参数的内存池

    return list;    //返回新创建的ngx_list_t 链表指针
}


void *
ngx_list_push(ngx_list_t *l)
{
    void             *elt;
    ngx_list_part_t  *last;

    last = l->last;

    if (last->nelts == l->nalloc) {

        /* the last part is full, allocate a new list part */

        last = ngx_palloc(l->pool, sizeof(ngx_list_part_t));
        if (last == NULL) {
            return NULL;
        }

        last->elts = ngx_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {
            return NULL;
        }

        last->nelts = 0;
        last->next = NULL;

        l->last->next = last;
        l->last = last;
    }

    elt = (char *) last->elts + l->size * last->nelts;
    last->nelts++;

    return elt;
}
