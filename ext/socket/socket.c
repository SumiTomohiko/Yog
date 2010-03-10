#include "yog/config.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Socket {
    struct YogBasicObj base;
    int sock;
};

typedef struct Socket Socket;

#define TYPE_SOCKET     ((type_t)TcpSocket_alloc)
#define CHECK_SELF(env, self)   do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_SOCKET)) { \
        YogError_raise_TypeError(env, "self must be socket"); \
    } \
} while (0)

static YogVal
TcpSocket_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Socket);
    YogBasicObj_init(env, obj, TYPE_SOCKET, 0, klass);
    PTR_AS(Socket, obj)->sock = 0;

    RETURN(env, obj);
}

static YogVal
recv_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal data = YUNDEF;
    YogVal size = YUNDEF;
    PUSH_LOCALS2(env, data, size);
    CHECK_SELF(env, self);
    YogCArg params[] = { { "size", &size }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "recv", params, args, kw);
    if (!IS_FIXNUM(size)) {
        YogError_raise_TypeError(env, "size must be Fixnum");
    }

    char* buf = (char*)YogSysdeps_alloca(VAL2INT(size));
    if (recv(PTR_AS(Socket, self)->sock, buf, VAL2INT(size), 0) < 0) {
        YogError_raise_sys_err(env, errno, YNIL);
    }
    data = YogBinary_new(env);
    YogBinary_add(env, data, buf, VAL2INT(size));

    RETURN(env, data);
}

static YogVal
close_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "close", params, args, kw);
    CHECK_SELF(env, self);
    if (close(PTR_AS(Socket, self)->sock) != 0) {
        YogError_raise_sys_err(env, errno, YNIL);
    }
    RETURN(env, self);
}

static YogVal
init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal host = YUNDEF;
    YogVal port = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS3(env, host, port, s);
    YogCArg params[] = { { "host", &host }, { "port", &port }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "init", params, args, kw);
    CHECK_SELF(env, self);
    if (!IS_PTR(host) || (BASIC_OBJ_TYPE(host) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "host must be String, not %C", host);
    }
    if (!IS_FIXNUM(port)) {
        YogError_raise_TypeError(env, "port must be String, not %C", port);
    }

    char port_s[6];
    YogSysdeps_snprintf(port_s, array_sizeof(port_s), "%u", VAL2INT(port));
    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = NULL;
    int err = getaddrinfo(STRING_CSTR(host), port_s, &hints, &res);
    if (err != 0) {
        YogError_raise_IOError(env, gai_strerror(err));
    }

    int sock;
    struct addrinfo* pa;
    for (pa = res; pa != NULL; pa = pa->ai_next) {
        sock = socket(pa->ai_family, pa->ai_socktype, pa->ai_protocol);
        if (sock == -1) {
            continue;
        }
        if (connect(sock, pa->ai_addr, pa->ai_addrlen) != 0) {
            continue;
        }
        break;
    }
    freeaddrinfo(res);
    if (pa == NULL) {
        s = YogSprintf_sprintf(env, "%S:%u", host, VAL2INT(port));
        YogError_raise_sys_err(env, errno, s);
    }
    PTR_AS(Socket, self)->sock = sock;

    RETURN(env, self);
}

static YogVal
send_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal data = YUNDEF;
    PUSH_LOCAL(env, data);
    YogCArg params[] = { { "data", &data }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "send", params, args, kw);
    CHECK_SELF(env, self);
    if (!IS_PTR(data) || (BASIC_OBJ_TYPE(data) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "data must be String");
    }

    int sock = PTR_AS(Socket, self)->sock;
    uint_t size = YogString_size(env, data);
    if (send(sock, STRING_CSTR(data), size, 0) < 0) {
        YogError_raise_sys_err(env, errno, YNIL);
    }

    RETURN(env, self);
}

YogVal
YogInit_socket(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    YogVal cTcpSocket = YUNDEF;
    PUSH_LOCALS2(env, pkg, cTcpSocket);

    pkg = YogPackage_new(env);

    cTcpSocket = YogClass_new(env, "TcpSocket", env->vm->cObject);
    YogClass_define_allocator(env, cTcpSocket, TcpSocket_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cTcpSocket, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("close", close_);
    DEFINE_METHOD("init", init);
    DEFINE_METHOD("recv", recv_);
    DEFINE_METHOD("send", send_);
#undef DEFINE_METHOD
    YogObj_set_attr(env, pkg, "TcpSocket", cTcpSocket);

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
