#include "swigmod.h"

class YOG: public Language {
protected:
    File* f_begin;
    File* f_runtime;

public:
    YOG() {
    }

    virtual void main(int argc, char *argv[]) {
        SWIG_library_directory("yog");
        SWIG_config_file("yog.swg");
    }

    virtual int top(Node *n) {
        String *outfile = Getattr(n, "outfile");
        f_begin = NewFile(outfile, "w", SWIG_output_files());
        if (!f_begin) {
            FileErrorDisplay(outfile);
            SWIG_exit(EXIT_FAILURE);
        }
        f_runtime = NewString("");

        Swig_register_filebyname("begin", f_begin);
        Swig_register_filebyname("runtime", f_runtime);

        Swig_banner(f_begin);

        Language::top(n);

        Dump(f_runtime, f_begin);

        Delete(f_runtime);
        Close(f_begin);
        Delete(f_begin);

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
