#include "yog/config.h"
#include <errno.h>
#if defined(YOG_HAVE_SYS_TIME_H)
#   include <sys/time.h>
#endif
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Datetime {
    struct YogBasicObj base;
    struct timeval val;
};

typedef struct Datetime Datetime;

#define TYPE_DATETIME TO_TYPE(alloc)

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal datetime = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Datetime);
    PUSH_LOCAL(env, datetime);
    YogBasicObj_init(env, datetime, TYPE_DATETIME, 0, klass);
    if (gettimeofday(&PTR_AS(Datetime, datetime)->val, NULL) != 0) {
        YogError_raise_sys_err(env, errno, YNIL);
    }

    RETURN(env, datetime);
}

YogVal
YogDatetime_new(YogEnv* env, time_t timestamp)
{
    YogVal obj = alloc(env, env->vm->cDatetime);
    PTR_AS(Datetime, obj)->val.tv_sec = timestamp;
    PTR_AS(Datetime, obj)->val.tv_usec = 0;
    return obj;
}

static void
check_Datetime(YogEnv* env, YogHandle* obj, const char* name)
{
    YogVal val = HDL2VAL(obj);
    if (!IS_PTR(val) || (BASIC_OBJ_TYPE(val) != TYPE_DATETIME)) {
        YogError_raise_TypeError(env, "%s must be Datetime, not %C", name, val);
    }
}

#define CHECK_SELF_TYPE(env, self)  check_Datetime((env), (self), "self")

static void
init_body(YogEnv* env, YogHandle* self, int_t year, int_t month, int_t day, int_t hour, int_t minute, int_t second, int_t microsecond)
{
    struct tm tm;
    bzero(&tm, sizeof(tm));
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    struct timeval* ptv = &HDL_AS(Datetime, self)->val;
    ptv->tv_sec = mktime(&tm);
    ptv->tv_usec = microsecond;
}

static YogVal
init(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* year, YogHandle* month, YogHandle* day, YogHandle* hour, YogHandle* minute, YogHandle* second, YogHandle* microsecond)
{
    CHECK_SELF_TYPE(env, self);
#define CHECK_ARG(name) YogMisc_check_Fixnum_optional(env, name, #name)
    CHECK_ARG(year);
    CHECK_ARG(month);
    CHECK_ARG(day);
    CHECK_ARG(hour);
    CHECK_ARG(minute);
    CHECK_ARG(second);
    CHECK_ARG(microsecond);
#undef CHECK_ARG

    if ((year == NULL) && (month == NULL) && (day == NULL) && (hour == NULL) && (minute == NULL) && (second == NULL) && (microsecond == NULL)) {
        return HDL2VAL(self);
    }
#define INT_OR_DEFAULT(h, default_) \
    ((h) != NULL ? VAL2INT(HDL2VAL((h))) : (default_))
    init_body(
        env,
        self,
        INT_OR_DEFAULT(year, 1970),
        INT_OR_DEFAULT(month, 1),
        INT_OR_DEFAULT(day, 1),
        INT_OR_DEFAULT(hour, 0),
        INT_OR_DEFAULT(minute, 0),
        INT_OR_DEFAULT(second, 0),
        INT_OR_DEFAULT(microsecond, 0));
#undef INT_OR_DEFAULT
    return HDL2VAL(self);
}

static YogVal
ufo(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* datetime)
{
    CHECK_SELF_TYPE(env, self);
    YogVal o = HDL2VAL(datetime);
    if (!IS_PTR(o) || (BASIC_OBJ_TYPE(o) != TYPE_DATETIME)) {
        return YNIL;
    }
    struct timeval* tv1 = &HDL_AS(Datetime, self)->val;
    struct timeval* tv0 = &HDL_AS(Datetime, datetime)->val;
    double diff = difftime(tv1->tv_sec, tv0->tv_sec);
    if (diff != 0.0) {
        return INT2VAL(0.0 < diff ? 1 : -1);
    }
    int_t udiff = tv1->tv_usec - tv0->tv_usec;
    return INT2VAL(udiff != 0 ? udiff : 0);
}

static YogVal
strftime_(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* format)
{
    CHECK_SELF_TYPE(env, self);
    YogMisc_check_String(env, format, "format");
    YogVal b = YogString_to_bin_in_default_encoding(env, format);
    /**
     * TODO: buf is too small.
     * Issue: 0de8dfbc9c096241d78377936ab885051dab933c
     */
    char buf[128];
    struct tm tm;
    localtime_r(&HDL_AS(Datetime, self)->val.tv_sec, &tm);
    if (strftime(buf, array_sizeof(buf), BINARY_CSTR(b), &tm) == 0) {
        YogError_raise_ArgumentError(env, "format is too long");
    }
    return YogString_from_string(env, buf);
}

#define IMPLEMENT_PROPERTY(name, member, offset) \
    static YogVal \
    name(YogEnv* env, YogHandle* self, YogHandle* pkg) \
    { \
        CHECK_SELF_TYPE(env, self); \
        struct tm tm; \
        localtime_r(&HDL_AS(Datetime, self)->val.tv_sec, &tm); \
        return INT2VAL(tm.member + offset); \
    }
IMPLEMENT_PROPERTY(get_year, tm_year, 1900)
IMPLEMENT_PROPERTY(get_month, tm_mon, 1)
IMPLEMENT_PROPERTY(get_day, tm_mday, 0)
IMPLEMENT_PROPERTY(get_hour, tm_hour, 0)
IMPLEMENT_PROPERTY(get_minute, tm_min, 0)
IMPLEMENT_PROPERTY(get_second, tm_sec, 0)

static YogVal
get_microsecond(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE(env, self);
    return INT2VAL(HDL_AS(Datetime, self)->val.tv_usec);
}

void
YogDatetime_eval_builtin_script(YogEnv* env, YogVal klass)
{
    YogMisc_eval_source(env, VAL2HDL(env, klass),
#include "datetime.inc"
    );
}

void
YogDatetime_define_classes(YogEnv* env, YogHandle* pkg)
{
    YogVM* vm = env->vm;

    YogVal obj = YogClass_new(env, "Datetime", vm->cObject);
    YogHandle* cDatetime = VAL2HDL(env, obj);

    YogClass_define_allocator(env, HDL2VAL(cDatetime), alloc);
    YogClass_include_module(env, HDL2VAL(cDatetime), vm->mComparable);
#define DEFINE_METHOD(name, ...) do { \
    YogVal val = HDL2VAL(cDatetime); \
    YogClass_define_method2(env, val, HDL2VAL(pkg), (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD("init", init, "|", "year", "month", "day", "hour", "minute", "second", "microsecond", NULL);
    DEFINE_METHOD("strftime", strftime_, "format", NULL);
    DEFINE_METHOD("<=>", ufo, "datetime", NULL);
#undef DEFINE_METHOD
#define DEFINE_PROP(name, getter, setter)   do { \
    YogClass_define_property2(env, cDatetime, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("year", get_year, NULL);
    DEFINE_PROP("month", get_month, NULL);
    DEFINE_PROP("day", get_day, NULL);
    DEFINE_PROP("hour", get_hour, NULL);
    DEFINE_PROP("minute", get_minute, NULL);
    DEFINE_PROP("second", get_second, NULL);
    DEFINE_PROP("microsecond", get_microsecond, NULL);
#undef DEFINE_PROP
    vm->cDatetime = HDL2VAL(cDatetime);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
