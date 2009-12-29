#if !defined(__YOG_FRAME_H__)
#define __YOG_FRAME_H__

#include "yog/yog.h"

enum YogFrameType {
    FRAME_C,
    FRAME_METHOD,
    FRAME_PKG,
    FRAME_CLASS,
    FRAME_FINISH,
};

typedef enum YogFrameType YogFrameType;

struct YogFrame {
    YogVal prev;
    enum YogFrameType type;
};

typedef struct YogFrame YogFrame;

struct YogCFrame {
    struct YogFrame base;
    YogVal self;
    YogVal args;
    YogVal f;
    YogVal multi_val;
};

typedef struct YogCFrame YogCFrame;

struct YogOuterVars {
    uint_t size;
    YogVal items[0];
};

typedef struct YogOuterVars YogOuterVars;

struct YogScriptFrame {
    struct YogFrame base;
    pc_t pc;
    YogVal code;
    uint_t stack_size;
    YogVal stack;
    YogVal globals;
    YogVal outer_vars;
    YogVal frame_to_long_return;
    YogVal frame_to_long_break;

    uint_t lhs_left_num;
    uint_t lhs_middle_num;
    uint_t lhs_right_num;
};

typedef struct YogScriptFrame YogScriptFrame;

#define YogFinishFrame  YogScriptFrame

#define SCRIPT_FRAME(v)     PTR_AS(YogScriptFrame, (v))

struct YogNameFrame {
    struct YogScriptFrame base;
    YogVal self;
    YogVal vars;
};

typedef struct YogNameFrame YogNameFrame;

#define NAME_FRAME(v)   PTR_AS(YogNameFrame, (v))
#define NAME_VARS(v)    (NAME_FRAME(v)->vars)

#define YogClassFrame       YogNameFrame
#define YogPackageFrame     YogNameFrame

struct YogMethodFrame {
    struct YogScriptFrame base;
    YogVal vars;

    YogVal klass;   /* class defining this method */
    ID name;
};

typedef struct YogMethodFrame YogMethodFrame;

#define METHOD_FRAME(v)     PTR_AS(YogMethodFrame, (v))
#define LOCAL_VARS(f)       (METHOD_FRAME(f)->vars)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
#if defined(__cplusplus)
extern "C" {
#endif
/* src/frame.c */
YogVal YogCFrame_new(YogEnv*);
void YogCFrame_return_multi_value(YogEnv*, YogVal, YogVal);
YogVal YogClassFrame_new(YogEnv*);
YogVal YogFinishFrame_new(YogEnv*);
YogVal YogMethodFrame_new(YogEnv*);
YogVal YogOuterVars_new(YogEnv*, uint_t);
YogVal YogPackageFrame_new(YogEnv*);
YogVal YogScriptFrame_pop_stack(YogEnv*, YogScriptFrame*);
void YogScriptFrame_push_stack(YogEnv*, YogScriptFrame*, YogVal);


#if defined(__cplusplus)
}
#endif
/* PROTOTYPE_END */

#define FRAME_PUSH(env, val)    do { \
    YogVal cur_frame = env->frame; \
    YogScriptFrame_push_stack((env), PTR_AS(YogScriptFrame, cur_frame), (val)); \
} while (0)

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
