#include "swigmod.h"

class YOG: public Language {
public:
    YOG() {
    }

    virtual void main(int argc, char *argv[]) {
#if 0
        SWIG_config_file("yog.swg");
#endif
    }

    virtual int top(Node *n) {
        return SWIG_OK;
    }
};

static Language *new_swig_yog() {
    return new YOG();
}
extern "C" Language *swig_yog(void) {
    return new_swig_yog();
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
