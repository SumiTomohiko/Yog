#include "swigmod.h"

static String *module = 0;

class YOG: public Language {
protected:
    File* f_begin;
    File* f_runtime;
    File* f_init;
    File* f_header;

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
        f_init = NewString("");
        f_header = NewString("");

        Swig_register_filebyname("begin", f_begin);
        Swig_register_filebyname("runtime", f_runtime);
        Swig_register_filebyname("init", f_init);
        Swig_register_filebyname("header", f_header);

        Swig_banner(f_begin);

        module = Copy(Getattr(n, "name"));
        Printf(f_header, "#define SWIG_init YogInit_%s\n", module);

        Language::top(n);

        Dump(f_runtime, f_begin);
        Dump(f_header, f_begin);
        Wrapper_pretty_print(f_init, f_begin);

        Delete(f_header);
        Delete(f_runtime);
        Delete(f_init);
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
