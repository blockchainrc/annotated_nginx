// annotated by chrono since 2016
//
// * ngx_http_upstream_conf_t
// * ngx_http_upstream_s

/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_UPSTREAM_H_INCLUDED_
#define _NGX_HTTP_UPSTREAM_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_event_connect.h>
#include <ngx_event_pipe.h>
#include <ngx_http.h>


#define NGX_HTTP_UPSTREAM_FT_ERROR           0x00000002
#define NGX_HTTP_UPSTREAM_FT_TIMEOUT         0x00000004
#define NGX_HTTP_UPSTREAM_FT_INVALID_HEADER  0x00000008
#define NGX_HTTP_UPSTREAM_FT_HTTP_500        0x00000010
#define NGX_HTTP_UPSTREAM_FT_HTTP_502        0x00000020
#define NGX_HTTP_UPSTREAM_FT_HTTP_503        0x00000040
#define NGX_HTTP_UPSTREAM_FT_HTTP_504        0x00000080
#define NGX_HTTP_UPSTREAM_FT_HTTP_403        0x00000100
#define NGX_HTTP_UPSTREAM_FT_HTTP_404        0x00000200
#define NGX_HTTP_UPSTREAM_FT_UPDATING        0x00000400
#define NGX_HTTP_UPSTREAM_FT_BUSY_LOCK       0x00000800
#define NGX_HTTP_UPSTREAM_FT_MAX_WAITING     0x00001000
#define NGX_HTTP_UPSTREAM_FT_NON_IDEMPOTENT  0x00002000
#define NGX_HTTP_UPSTREAM_FT_NOLIVE          0x40000000
#define NGX_HTTP_UPSTREAM_FT_OFF             0x80000000

#define NGX_HTTP_UPSTREAM_FT_STATUS          (NGX_HTTP_UPSTREAM_FT_HTTP_500  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_502  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_503  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_504  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_403  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_404)

#define NGX_HTTP_UPSTREAM_INVALID_HEADER     40


#define NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT    0x00000002
#define NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES     0x00000004
#define NGX_HTTP_UPSTREAM_IGN_EXPIRES        0x00000008
#define NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL  0x00000010
#define NGX_HTTP_UPSTREAM_IGN_SET_COOKIE     0x00000020
#define NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE  0x00000040
#define NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING   0x00000080
#define NGX_HTTP_UPSTREAM_IGN_XA_CHARSET     0x00000100
#define NGX_HTTP_UPSTREAM_IGN_VARY           0x00000200


typedef struct {
    ngx_msec_t                       bl_time;
    ngx_uint_t                       bl_state;

    ngx_uint_t                       status;
    ngx_msec_t                       response_time;
    ngx_msec_t                       connect_time;
    ngx_msec_t                       header_time;
    off_t                            response_length;

    ngx_str_t                       *peer;
} ngx_http_upstream_state_t;


// upstream模块的主配置，存储所有的upstream{}配置
typedef struct {
    ngx_hash_t                       headers_in_hash;
    ngx_array_t                      upstreams;
                                             /* ngx_http_upstream_srv_conf_t */
} ngx_http_upstream_main_conf_t;

typedef struct ngx_http_upstream_srv_conf_s  ngx_http_upstream_srv_conf_t;

// 给load balance模块用初始化函数指针
typedef ngx_int_t (*ngx_http_upstream_init_pt)(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us);
typedef ngx_int_t (*ngx_http_upstream_init_peer_pt)(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);


// 给load balance模块用初始化结构体
// 定义了load-balance模块的入口，有两个回调函数，用来初始化load-balance模块
typedef struct {
    ngx_http_upstream_init_pt        init_upstream;
    ngx_http_upstream_init_peer_pt   init;
    void                            *data;
} ngx_http_upstream_peer_t;


// 在upstream{}里的每个server指令相关的信息
// 包括地址/权重/超时等等
typedef struct {
    ngx_str_t                        name;
    ngx_addr_t                      *addrs;
    ngx_uint_t                       naddrs;
    ngx_uint_t                       weight;
    ngx_uint_t                       max_fails;
    time_t                           fail_timeout;

    // 1.11.5新增slow_start、max_conns

    unsigned                         down:1;
    unsigned                         backup:1;
} ngx_http_upstream_server_t;


#define NGX_HTTP_UPSTREAM_CREATE        0x0001
#define NGX_HTTP_UPSTREAM_WEIGHT        0x0002
#define NGX_HTTP_UPSTREAM_MAX_FAILS     0x0004
#define NGX_HTTP_UPSTREAM_FAIL_TIMEOUT  0x0008
#define NGX_HTTP_UPSTREAM_DOWN          0x0010
#define NGX_HTTP_UPSTREAM_BACKUP        0x0020


// 每个upstream{}块的配置结构体，存储server信息
struct ngx_http_upstream_srv_conf_s {
    ngx_http_upstream_peer_t         peer;
    void                           **srv_conf;

    // 存储ngx_http_upstream_server_t的数组
    ngx_array_t                     *servers;  /* ngx_http_upstream_server_t */

    ngx_uint_t                       flags;
    ngx_str_t                        host;

    // 在配置文件里的位置
    u_char                          *file_name;
    ngx_uint_t                       line;

    in_port_t                        port;
    in_port_t                        default_port;
    ngx_uint_t                       no_port;  /* unsigned no_port:1 */

#if (NGX_HTTP_UPSTREAM_ZONE)
    ngx_shm_zone_t                  *shm_zone;
#endif
};


typedef struct {
    ngx_addr_t                      *addr;
    ngx_http_complex_value_t        *value;
} ngx_http_upstream_local_t;


// 需要设置超时时间、缓冲、限速等必要的参数，才能正确地连接上游服务器
typedef struct {
    ngx_http_upstream_srv_conf_t    *upstream;

    ngx_msec_t                       connect_timeout;
    ngx_msec_t                       send_timeout;
    ngx_msec_t                       read_timeout;
    ngx_msec_t                       timeout;
    ngx_msec_t                       next_upstream_timeout;

    size_t                           send_lowat;
    size_t                           buffer_size;
    size_t                           limit_rate;

    size_t                           busy_buffers_size;
    size_t                           max_temp_file_size;
    size_t                           temp_file_write_size;

    size_t                           busy_buffers_size_conf;
    size_t                           max_temp_file_size_conf;
    size_t                           temp_file_write_size_conf;

    ngx_bufs_t                       bufs;

    ngx_uint_t                       ignore_headers;
    ngx_uint_t                       next_upstream;
    ngx_uint_t                       store_access;
    ngx_uint_t                       next_upstream_tries;
    ngx_flag_t                       buffering;
    ngx_flag_t                       request_buffering;
    ngx_flag_t                       pass_request_headers;
    ngx_flag_t                       pass_request_body;

    ngx_flag_t                       ignore_client_abort;
    ngx_flag_t                       intercept_errors;
    ngx_flag_t                       cyclic_temp_file;
    ngx_flag_t                       force_ranges;

    ngx_path_t                      *temp_path;

    ngx_hash_t                       hide_headers_hash;
    ngx_array_t                     *hide_headers;
    ngx_array_t                     *pass_headers;

    ngx_http_upstream_local_t       *local;

#if (NGX_HTTP_CACHE)
    ngx_shm_zone_t                  *cache_zone;
    ngx_http_complex_value_t        *cache_value;

    ngx_uint_t                       cache_min_uses;
    ngx_uint_t                       cache_use_stale;
    ngx_uint_t                       cache_methods;

    ngx_flag_t                       cache_lock;
    ngx_msec_t                       cache_lock_timeout;
    ngx_msec_t                       cache_lock_age;

    ngx_flag_t                       cache_revalidate;
    ngx_flag_t                       cache_convert_head;

    ngx_array_t                     *cache_valid;
    ngx_array_t                     *cache_bypass;
    ngx_array_t                     *no_cache;
#endif

    ngx_array_t                     *store_lengths;
    ngx_array_t                     *store_values;

#if (NGX_HTTP_CACHE)
    signed                           cache:2;
#endif
    signed                           store:2;
    unsigned                         intercept_404:1;
    unsigned                         change_buffering:1;

#if (NGX_HTTP_SSL)
    ngx_ssl_t                       *ssl;
    ngx_flag_t                       ssl_session_reuse;

    ngx_http_complex_value_t        *ssl_name;
    ngx_flag_t                       ssl_server_name;
    ngx_flag_t                       ssl_verify;
#endif

    ngx_str_t                        module;
} ngx_http_upstream_conf_t;


typedef struct {
    ngx_str_t                        name;
    ngx_http_header_handler_pt       handler;
    ngx_uint_t                       offset;
    ngx_http_header_handler_pt       copy_handler;
    ngx_uint_t                       conf;
    ngx_uint_t                       redirect;  /* unsigned   redirect:1; */
} ngx_http_upstream_header_t;


typedef struct {
    ngx_list_t                       headers;

    ngx_uint_t                       status_n;
    ngx_str_t                        status_line;

    ngx_table_elt_t                 *status;
    ngx_table_elt_t                 *date;
    ngx_table_elt_t                 *server;
    ngx_table_elt_t                 *connection;

    ngx_table_elt_t                 *expires;
    ngx_table_elt_t                 *etag;
    ngx_table_elt_t                 *x_accel_expires;
    ngx_table_elt_t                 *x_accel_redirect;
    ngx_table_elt_t                 *x_accel_limit_rate;

    ngx_table_elt_t                 *content_type;
    ngx_table_elt_t                 *content_length;

    ngx_table_elt_t                 *last_modified;
    ngx_table_elt_t                 *location;
    ngx_table_elt_t                 *accept_ranges;
    ngx_table_elt_t                 *www_authenticate;
    ngx_table_elt_t                 *transfer_encoding;
    ngx_table_elt_t                 *vary;

#if (NGX_HTTP_GZIP)
    ngx_table_elt_t                 *content_encoding;
#endif

    ngx_array_t                      cache_control;
    ngx_array_t                      cookies;

    off_t                            content_length_n;
    time_t                           last_modified_time;

    unsigned                         connection_close:1;
    unsigned                         chunked:1;
} ngx_http_upstream_headers_in_t;


typedef struct {
    ngx_str_t                        host;
    in_port_t                        port;
    ngx_uint_t                       no_port; /* unsigned no_port:1 */

    ngx_uint_t                       naddrs;
    ngx_resolver_addr_t             *addrs;

    struct sockaddr                 *sockaddr;
    socklen_t                        socklen;

    ngx_resolver_ctx_t              *ctx;
} ngx_http_upstream_resolved_t;


typedef void (*ngx_http_upstream_handler_pt)(ngx_http_request_t *r,
    ngx_http_upstream_t *u);


// 定义了upstream机制需要的所有信息
struct ngx_http_upstream_s {
    ngx_http_upstream_handler_pt     read_event_handler;
    ngx_http_upstream_handler_pt     write_event_handler;

    // 连接结构体
    ngx_peer_connection_t            peer;

    ngx_event_pipe_t                *pipe;

    // 发送的请求数据
    // u->request_bufs = r->request_body->bufs;
    ngx_chain_t                     *request_bufs;

    ngx_output_chain_ctx_t           output;
    ngx_chain_writer_ctx_t           writer;

    // 上游的连接参数设置
    ngx_http_upstream_conf_t        *conf;
#if (NGX_HTTP_CACHE)
    ngx_array_t                     *caches;
#endif

    // 上游的响应头
    ngx_http_upstream_headers_in_t   headers_in;

    // 上游服务器的地址
    ngx_http_upstream_resolved_t    *resolved;

    ngx_buf_t                        from_client;

    // 数据缓冲区
    ngx_buf_t                        buffer;

    // 缓冲数据的长度
    off_t                            length;

    // 从上游接收到的数据
    ngx_chain_t                     *out_bufs;

    ngx_chain_t                     *busy_bufs;
    ngx_chain_t                     *free_bufs;

    // 处理上游服务器响应数据的回调函数
    ngx_int_t                      (*input_filter_init)(void *data);
    ngx_int_t                      (*input_filter)(void *data, ssize_t bytes);
    void                            *input_filter_ctx;

#if (NGX_HTTP_CACHE)
    ngx_int_t                      (*create_key)(ngx_http_request_t *r);
#endif

    // 发送接收请求的回调函数，9个
    ngx_int_t                      (*create_request)(ngx_http_request_t *r);
    ngx_int_t                      (*reinit_request)(ngx_http_request_t *r);
    ngx_int_t                      (*process_header)(ngx_http_request_t *r);
    void                           (*abort_request)(ngx_http_request_t *r);
    void                           (*finalize_request)(ngx_http_request_t *r,
                                         ngx_int_t rc);
    ngx_int_t                      (*rewrite_redirect)(ngx_http_request_t *r,
                                         ngx_table_elt_t *h, size_t prefix);
    ngx_int_t                      (*rewrite_cookie)(ngx_http_request_t *r,
                                         ngx_table_elt_t *h);

    ngx_msec_t                       timeout;

    // 处理的状态信息
    ngx_http_upstream_state_t       *state;

    ngx_str_t                        method;
    ngx_str_t                        schema;
    ngx_str_t                        uri;

#if (NGX_HTTP_SSL)
    ngx_str_t                        ssl_name;
#endif

    ngx_http_cleanup_pt             *cleanup;

    unsigned                         store:1;
    unsigned                         cacheable:1;
    unsigned                         accel:1;
    unsigned                         ssl:1;
#if (NGX_HTTP_CACHE)
    unsigned                         cache_status:3;
#endif

    // 是否使用更多的缓冲区来接收上游的数据
    unsigned                         buffering:1;

    unsigned                         keepalive:1;
    unsigned                         upgrade:1;

    // 是否已经发送请求
    unsigned                         request_sent:1;

    // 是否已经发送请求体数据
    unsigned                         request_body_sent:1;

    // 是否已经发送响应头
    unsigned                         header_sent:1;
};


typedef struct {
    ngx_uint_t                      status;
    ngx_uint_t                      mask;
} ngx_http_upstream_next_t;


typedef struct {
    ngx_str_t   key;
    ngx_str_t   value;
    ngx_uint_t  skip_empty;
} ngx_http_upstream_param_t;


ngx_int_t ngx_http_upstream_cookie_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
ngx_int_t ngx_http_upstream_header_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

ngx_int_t ngx_http_upstream_create(ngx_http_request_t *r);
void ngx_http_upstream_init(ngx_http_request_t *r);
ngx_http_upstream_srv_conf_t *ngx_http_upstream_add(ngx_conf_t *cf,
    ngx_url_t *u, ngx_uint_t flags);
char *ngx_http_upstream_bind_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
char *ngx_http_upstream_param_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
ngx_int_t ngx_http_upstream_hide_headers_hash(ngx_conf_t *cf,
    ngx_http_upstream_conf_t *conf, ngx_http_upstream_conf_t *prev,
    ngx_str_t *default_hide_headers, ngx_hash_init_t *hash);


#define ngx_http_conf_upstream_srv_conf(uscf, module)                         \
    uscf->srv_conf[module.ctx_index]


extern ngx_module_t        ngx_http_upstream_module;
extern ngx_conf_bitmask_t  ngx_http_upstream_cache_method_mask[];
extern ngx_conf_bitmask_t  ngx_http_upstream_ignore_headers_masks[];


#endif /* _NGX_HTTP_UPSTREAM_H_INCLUDED_ */