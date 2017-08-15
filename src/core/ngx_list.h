
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

/*
 * 链表初始化
 * @desc    该函数是用于ngx_list_t类型的对象已经存在，但是其第一个节点存放元素的内存空间还未分配的情况下，可以调用此函数来给这个list的首节点来分配存放元素的内存空间。
 *          那么什么时候会出现已经有了ngx_list_t类型的对象，而其首节点存放元素的内存尚未分配的情况呢？那就是这个ngx_list_t类型的变量并不是通过调用ngx_list_create函数创建的。
 *          例如：如果某个结构体的一个成员变量是ngx_list_t类型的，那么当这个结构体类型的对象被创建出来的时候，这个成员变量也被创建出来了，但是它的首节点的存放元素的内存并未被分配。
 *          如果这个ngx_list_t类型的变量，如果不是你通过调用函数ngx_list_create创建的，那么就必须调用此函数去初始话，否则，你往这个list里追加元素就可能引发不可预知的行为，亦或程序会崩溃!
 
 * @param   *list   链表指针
 * @param   *pool   用来分配内存的内存池指针
 * @param   n       n个数组元素
 * @param   size    每个数组元素大小是size字节
 */
static ngx_inline ngx_int_t //静态内联函数。用它声明的函数，只在本文件（包括被#include引入）中被内联到代码中，不会生成单独的汇编代码供其他文件调用
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);   //链表第一个元素，它的数组数据区指向新申请的内存（ngx_palloc从内存池pool中申请的大小为n*size内存）。
    if (list->part.elts == NULL) {
        return NGX_ERROR;   // -1
    }

    list->part.nelts = 0;   //链表首个元素的数组中已经使用了多少个元素，初始化为0
    list->part.next = NULL; //下一个链表元素为NULL，即没有下一个
    list->last = &list->part;   ///list->part是一个变量，不是指针，链表的首个元素直接作为链表结构的一个成员变量。这里就是取list->part的地址，赋给list->last。即list->last 指针，指向链表的首个元素
    list->size = size;  //限制链表元素中的数组中每个元素的大小最大为 size
    list->nalloc = n;   //限制链表元素中的数组的元素个数，最大为 n
    list->pool = pool;  //链表中管理内存分配的内存池，指向创建列表传入的参数的内存池

    return NGX_OK;  // 0。初始化正常
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
