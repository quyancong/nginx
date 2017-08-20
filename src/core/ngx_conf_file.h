
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_CONF_FILE_H_INCLUDED_
#define _NGX_HTTP_CONF_FILE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 *        AAAA  number of agruments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */

#define NGX_CONF_NOARGS      0x00000001
#define NGX_CONF_TAKE1       0x00000002
#define NGX_CONF_TAKE2       0x00000004
#define NGX_CONF_TAKE3       0x00000008
#define NGX_CONF_TAKE4       0x00000010
#define NGX_CONF_TAKE5       0x00000020
#define NGX_CONF_TAKE6       0x00000040
#define NGX_CONF_TAKE7       0x00000080

#define NGX_CONF_MAX_ARGS    8

#define NGX_CONF_TAKE12      (NGX_CONF_TAKE1|NGX_CONF_TAKE2)
#define NGX_CONF_TAKE13      (NGX_CONF_TAKE1|NGX_CONF_TAKE3)

#define NGX_CONF_TAKE23      (NGX_CONF_TAKE2|NGX_CONF_TAKE3)

#define NGX_CONF_TAKE123     (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3)
#define NGX_CONF_TAKE1234    (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3   \
                              |NGX_CONF_TAKE4)

#define NGX_CONF_ARGS_NUMBER 0x000000ff
#define NGX_CONF_BLOCK       0x00000100
#define NGX_CONF_FLAG        0x00000200
#define NGX_CONF_ANY         0x00000400
#define NGX_CONF_1MORE       0x00000800
#define NGX_CONF_2MORE       0x00001000
#define NGX_CONF_MULTI       0x00002000

#define NGX_DIRECT_CONF      0x00010000

#define NGX_MAIN_CONF        0x01000000
#define NGX_ANY_CONF         0x0F000000



#define NGX_CONF_UNSET       -1
#define NGX_CONF_UNSET_UINT  (ngx_uint_t) -1
#define NGX_CONF_UNSET_PTR   (void *) -1
#define NGX_CONF_UNSET_SIZE  (size_t) -1
#define NGX_CONF_UNSET_MSEC  (ngx_msec_t) -1


#define NGX_CONF_OK          NULL
#define NGX_CONF_ERROR       (void *) -1

#define NGX_CONF_BLOCK_START 1
#define NGX_CONF_BLOCK_DONE  2
#define NGX_CONF_FILE_DONE   3

#define NGX_CORE_MODULE      0x45524F43  /* "CORE" */
#define NGX_CONF_MODULE      0x464E4F43  /* "CONF" */


#define NGX_MAX_CONF_ERRSTR  1024

/*
 * 定义模块的配置参数的结构体
 */
struct ngx_command_s {
    ngx_str_t             name; //配置项的名称 如 gzip
    ngx_uint_t            type; //配置项类型，type将指定配置项可以出现的位置。例如，出现在server{} 或者 localtion{}中，以及它可以携带的参数个数
    char               *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf); //出现了name中指定的配置项后，将会调用set方法处理配置项的参数


    /*
     * 它的值可以是以下的一种，指明当前配置要放在main，srv，loc三种配置结构体哪一个结构体里。它和下面的 offset配合使用。
        NGX_HTTP_MAIN_CONF_OFFSET
        NGX_HTTP_SRV_CONF_OFFSET
        NGX_HTTP_LOC_CONF_OFFSET
    */
    ngx_uint_t            conf; 


    ngx_uint_t            offset;   //当前配置项在整个存储配置项的结构体中的偏移位置。对于使用Nginx预设的解析方法：Nginx首先通过conf成员找到应该用哪个结构体来存放，然后通过offset成员找到这个结构体中的相应成员，以便存放该配置。
    void                 *post;     //配置项读取后的回调处理方法，必须是ngx_conf_post_t指针
};

#define ngx_null_command  { ngx_null_string, 0, NULL, 0, 0, NULL }


struct ngx_open_file_s {
    ngx_fd_t              fd;
    ngx_str_t             name;

    u_char               *buffer;
    u_char               *pos;
    u_char               *last;

#if 0
    /* e.g. append mode, error_log */
    ngx_uint_t            flags;
    /* e.g. reopen db file */
    ngx_uint_t          (*handler)(void *data, ngx_open_file_t *file);
    void                 *data;
#endif
};


#define NGX_MODULE_V1          0, 0, 0, 0, 0, 0, 1
#define NGX_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0

/*
 * nginx模块化的基础数据结构。
 * 基本接口ngx_module_t只涉及模块的初始化、退出以及对配置项的处理。
 * ngx_module_t结构体作为所有模块的通用接口，只定义了init_master 、init_module、init_process、init_thread、exit_thread、exit_process、exit_master这七个回调方法，
 * 而init_master 、init_thread、exit_thread这三个方法目前没有使用，后两个是因为nginx目前没有使用线程（因为多线程代码实现更难控制）。
 * 他们负责模块的初始化和退出，同时他们可以处理核心结构体ngx_cycle_t（每个进程都有一个该结构体变量）。
 * 而ngx_command_t类型的commands数组指定了模块中的配置名和模块处理配置变量的方法，ctx是一个void*指针，他表示一类模块所特有的通用函数接口。
 */
struct  ngx_module_s {
    /* 对于一类模块（由下面的type成员决定类别）而言，ctx_index标示当前模块在这类模块中的序号。
    这个成员常常是由管理这类模块的一个nginx核心模块设置的，对于所有的HTTP模块而言，ctx_index
    是由核心模块ngx_http_module设置的。 */
    ngx_uint_t            ctx_index;

    /* index表示当前模块在ngx_modules数组中的序号(ctx_index标示当前模块在这类模块中的序号)。Nginx启动的时候会根据ngx_modules数组设置各个模块的index值 */
    ngx_uint_t            index;

    /* spare系列的保留变量，暂未使用 */
    ngx_uint_t            spare1;
    ngx_uint_t            spare2;
    ngx_uint_t            spare3;

    /* nginx模块的版本，便于将来的扩展。目前只有一种，默认为1 */
    ngx_uint_t            version;

    /* ctx用于指向一类模块的上下文结构体。模块上下文，每个模块有不同模块上下文,每个模块都有自己的特性，而ctx会指向特定类型模块的公共接口。
    比如，在HTTP模块中，ctx需要指向ngx_http_module_t结构体。 */
    void                 *ctx;

    /* 模块命令集，将处理nginx.conf中的配置项 */
    ngx_command_t        *commands;

    /* 标示该模块的类型，和ctx是紧密相关的。在官方的Nginx中，它的取值范围是以下几种:
    NGX_HTTP_MODULE,NGX_CORE_MODULE,NGX_CONF_MODULE,
    NGX_EVENT_MODULE,NGX_MAIL_MODULE 。实际上，还可以自定义新的模块类型 */
    ngx_uint_t            type;

    ngx_uint_t            spare0;

    /* 下面7个函数是nginx在启动，停止过程中的7个执行点 */
    ngx_int_t           (*init_master)(ngx_log_t *log);         //在master进程启动时候，回调init_master ，但到目前为止框架代码从来不会调用他，因此可以将其赋值为NULL

    ngx_int_t           (*init_module)(ngx_cycle_t *cycle);     //init_module在初始化所有模块时被调用。在master/slave模式下，这个阶段将在启动worker紫禁城前完成。

    ngx_int_t           (*init_process)(ngx_cycle_t *cycle);    //init_process回调方法在正常服务前被调用。在master/slave模式下，多个worker子进程已经产生，在每个worker进程的初始化过程会调用所有模块的init_process函数
    ngx_int_t           (*init_thread)(ngx_cycle_t *cycle);     //由于nginx不支持多线程模式，所以init_thread在框架代码中没有被调用过，设为NULL
    void                (*exit_thread)(ngx_cycle_t *cycle);     //同上，exit_thread也不支持，设为NULL
    void                (*exit_process)(ngx_cycle_t *cycle);    //exit_process回调方法在服务停止前调用。在master/worker模式下，worker进程会在退出前调用它

    void                (*exit_master)(ngx_cycle_t *cycle);     //exit_master回调方法将在master进程退出前被调用

    //保留字段，目前没有使用，可以使用 #define NGX_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0  宏来替换
    uintptr_t             spare_hook0;
    uintptr_t             spare_hook1;
    uintptr_t             spare_hook2;
    uintptr_t             spare_hook3;
    uintptr_t             spare_hook4;
    uintptr_t             spare_hook5;
    uintptr_t             spare_hook6;
    uintptr_t             spare_hook7;
};


typedef struct {
    ngx_str_t             name;
    void               *(*create_conf)(ngx_cycle_t *cycle);
    char               *(*init_conf)(ngx_cycle_t *cycle, void *conf);
} ngx_core_module_t;


typedef struct {
    ngx_file_t            file;
    ngx_buf_t            *buffer;
    ngx_uint_t            line;
} ngx_conf_file_t;


typedef char *(*ngx_conf_handler_pt)(ngx_conf_t *cf,
    ngx_command_t *dummy, void *conf);


struct ngx_conf_s {
    char                 *name;
    ngx_array_t          *args;

    ngx_cycle_t          *cycle;
    ngx_pool_t           *pool;
    ngx_pool_t           *temp_pool;
    ngx_conf_file_t      *conf_file;
    ngx_log_t            *log;

    void                 *ctx;
    ngx_uint_t            module_type;
    ngx_uint_t            cmd_type;

    ngx_conf_handler_pt   handler;
    char                 *handler_conf;
};


typedef char *(*ngx_conf_post_handler_pt) (ngx_conf_t *cf,
    void *data, void *conf);

typedef struct {
    ngx_conf_post_handler_pt  post_handler;
} ngx_conf_post_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    char                     *old_name;
    char                     *new_name;
} ngx_conf_deprecated_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    ngx_int_t                 low;
    ngx_int_t                 high;
} ngx_conf_num_bounds_t;


typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                value;
} ngx_conf_enum_t;


#define NGX_CONF_BITMASK_SET  1

typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                mask;
} ngx_conf_bitmask_t;



char * ngx_conf_deprecated(ngx_conf_t *cf, void *post, void *data);
char *ngx_conf_check_num_bounds(ngx_conf_t *cf, void *post, void *data);


#define ngx_get_conf(conf_ctx, module)  conf_ctx[module.index]



#define ngx_conf_init_value(conf, default)                                   \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = default;                                                      \
    }

#define ngx_conf_init_ptr_value(conf, default)                               \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = default;                                                      \
    }

#define ngx_conf_init_uint_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_size_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_msec_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_merge_value(conf, prev, default)                            \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_ptr_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = (prev == NGX_CONF_UNSET_PTR) ? default : prev;                \
    }

#define ngx_conf_merge_uint_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = (prev == NGX_CONF_UNSET_UINT) ? default : prev;               \
    }

#define ngx_conf_merge_msec_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = (prev == NGX_CONF_UNSET_MSEC) ? default : prev;               \
    }

#define ngx_conf_merge_sec_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_size_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = (prev == NGX_CONF_UNSET_SIZE) ? default : prev;               \
    }

#define ngx_conf_merge_off_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_str_value(conf, prev, default)                        \
    if (conf.data == NULL) {                                                 \
        if (prev.data) {                                                     \
            conf.len = prev.len;                                             \
            conf.data = prev.data;                                           \
        } else {                                                             \
            conf.len = sizeof(default) - 1;                                  \
            conf.data = (u_char *) default;                                  \
        }                                                                    \
    }

#define ngx_conf_merge_bufs_value(conf, prev, default_num, default_size)     \
    if (conf.num == 0) {                                                     \
        if (prev.num) {                                                      \
            conf.num = prev.num;                                             \
            conf.size = prev.size;                                           \
        } else {                                                             \
            conf.num = default_num;                                          \
            conf.size = default_size;                                        \
        }                                                                    \
    }

#define ngx_conf_merge_bitmask_value(conf, prev, default)                    \
    if (conf == 0) {                                                         \
        conf = (prev == 0) ? default : prev;                                 \
    }


char *ngx_conf_param(ngx_conf_t *cf);
char *ngx_conf_parse(ngx_conf_t *cf, ngx_str_t *filename);


ngx_int_t ngx_conf_full_name(ngx_cycle_t *cycle, ngx_str_t *name,
    ngx_uint_t conf_prefix);
ngx_open_file_t *ngx_conf_open_file(ngx_cycle_t *cycle, ngx_str_t *name);
void ngx_cdecl ngx_conf_log_error(ngx_uint_t level, ngx_conf_t *cf,
    ngx_err_t err, const char *fmt, ...);


char *ngx_conf_set_flag_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_str_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_str_array_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
char *ngx_conf_set_keyval_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_num_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_size_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_off_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_msec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_sec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_bufs_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_enum_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_conf_set_bitmask_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


extern ngx_uint_t     ngx_max_module;
extern ngx_module_t  *ngx_modules[];


#endif /* _NGX_HTTP_CONF_FILE_H_INCLUDED_ */
