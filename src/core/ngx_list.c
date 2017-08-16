
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

/*
 * 返回链表元素的一个新的数组元素的位置指针
 * @desc    在使用它时通常先调用ngx_list_push得到返回的数组元素地址，再对返回的地址进行赋值。
 *          如果当前链表的最后一个元素的数组已经全部使用，则创建一个新的链表元素，并把第一个数组元素的地址返回，否则直接返回当前链表元素的下一个未使用的数组元素的地址。
 * @param *l 要添加链表元素的链表指针
 * @return  返回的是一个可用的数组元素的地址，用来存放该数组类型的数据
 */
void *
ngx_list_push(ngx_list_t *l)
{
    void             *elt;
    ngx_list_part_t  *last;

    last = l->last; // 临时的链表元素指针last，指向l链表的最后一个元素

    if (last->nelts == l->nalloc) { //如果最后一个链表元素的数组中，使用的数组元素数量等于链表限制的最大数组元素数量，即当前链表元素已经无数组元素可以使用，则申请创建一个新的链表元素。否则返回当前链表元素上的下一个未使用的数组元素地址

        /* the last part is full, allocate a new list part */

        //申请链表元素的空间。在链表l使用的内存池上，申请一块链表元素结构所需大小的内存，并用last指向它
        last = ngx_palloc(l->pool, sizeof(ngx_list_part_t));
        if (last == NULL) { //如果申请内存失败，则返回NULL
            return NULL;
        }

        //申请链表元素指向的数组的空间。在链表l使用的内存池上，申请一块该链表限制链表元素的最大数组的容量（链表元素的最大数组的容量=最大数组元素数量nalloc*单个数组元素的大小size）的大小的内存
        //并将该新建链表元素的首地址（last->elts）指向新申请的数组内存
        last->elts = ngx_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {   //如果申请内存失败，则返回NULL
            return NULL;
        }

        last->nelts = 0;    //数组中已经使用的元素数量，初始化为0
        last->next = NULL;  //下一个链表元素是NULL

        l->last->next = last;   //原来链表的最后一个元素的next指针，指向新建的链表元素
        l->last = last;         //链表的last指针，指向新建的链表元素
    }

    elt = (char *) last->elts + l->size * last->nelts;  //返回下一个未使用的数组元素的起始地址
    last->nelts++;  //last目前指向链表的最后一个链表元素。这个链表元素可以是之前就有的，也可能是刚刚新创建的。这个链表元素的数组元素使用数量加1

    return elt; //返回可用的数组元素指针（指针类型是任意类型的，具体类型根据接受这个返回指针的指针变量的类型来确定）
}
