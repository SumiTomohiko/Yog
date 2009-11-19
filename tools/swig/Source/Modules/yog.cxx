#include <assert.h>
#include "swigmod.h"

#define BUF_SIZE    1024

class YOG: public Language {
private:
    String* module;
    String* methods;

    File* f_begin;
    File* f_runtime;
    File* f_init;
    File* f_header;
    File* f_wrappers;

    void add_function(String* name) {
        Printf(this->methods, "    { \"%s\", %s },\n", name, Swig_name_wrapper(name));
    }

public:
    YOG() {
    }

    virtual void main(int argc, char *argv[]) {
        SWIG_library_directory("yog");
        SWIG_config_file("yog.swg");
    }

    void print_locals_guard(String* dest, int rest_arg_num) {
        char name[BUF_SIZE];
        if (rest_arg_num == 1) {
            strcpy(name, "PUSH_LOCAL");
        }
        else if (rest_arg_num <= 4) {
            snprintf(name, BUF_SIZE, "PUSH_LOCALS%d", rest_arg_num);
        }
        else {
            strcpy(name, "PUSH_LOCALS4");
        }
        Printf(dest, name);
        Printf(dest, "(env");
    }

    virtual int functionWrapper(Node* n) {
        String *name = Getattr(n, "name");
        this->add_function(name);

        Wrapper* f = NewWrapper();
        ParmList* l = Getattr(n, "parms");
        emit_parameter_variables(l, f);
        emit_attach_parmmaps(l, f);

        String *iname = Getattr(n, "sym:name");
        String *wname = Swig_name_wrapper(iname);
        Printv(f->def,
            "SWIGINTERN YogVal ", wname, "(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block) {\n",
            "    SAVE_ARGS4(env, self, args, kw, block);\n",
            NIL);

        int i;
        Parm* p;
        int num_arguments = ::emit_num_arguments(l);
        for (i = 0, p = l; i < num_arguments; i++) {
            while (checkAttribute(p, "tmap:in:numinputs", "0")) {
                p = Getattr(p, "tmap:in:next");
            }

            char s[BUF_SIZE];
            snprintf(s, BUF_SIZE, "obj%d", i);
            Printf(f->code, "    YogVal %s = YUNDEF;\n", s);
        }
        if (0 < num_arguments) {
            for (i = 0, p = l; i < num_arguments; i++) {
                while (checkAttribute(p, "tmap:in:numinputs", "0")) {
                    p = Getattr(p, "tmap:in:next");
                }

                if (i % 8 == 0) {
                    this->print_locals_guard(f->code, num_arguments - i);
                }

                char s[BUF_SIZE];
                snprintf(s, BUF_SIZE, ", obj%d", i);
                Printf(f->code, s);

                if (i % 8 == 7) {
                    Printf(f->code, ");\n");
                }
            }
            if (i % 8 != 7) {
                Printf(f->code, ");\n");
            }
        }
        Append(f->code, "    YogVal resultobj = YUNDEF;\n");
        Append(f->code, "    PUSH_LOCAL(env, resultobj);\n");

        for (i = 0, p = l; i < num_arguments; i++) {
            while (checkAttribute(p, "tmap:in:numinputs", "0")) {
                p = Getattr(p, "tmap:in:next");
            }

            char s[BUF_SIZE];
            snprintf(s, BUF_SIZE, "    obj%d = YogArray_at(env, args, %d);\n", i, i);
            Append(f->code, s);
        }
        for (i = 0, p = l; i < num_arguments; i++) {
            while (checkAttribute(p, "tmap:in:numinputs", "0")) {
                p = Getattr(p, "tmap:in:next");
            }

            String *tm = Getattr(p, "tmap:in");;
            if (tm == NULL) {
                break;
            }
            char s[BUF_SIZE];
            snprintf(s, BUF_SIZE, "obj%d", i);
            Replaceall(tm, "$input", s);
            Append(f->code, tm);
        }

        Setattr(n, "wrap:name", wname);
        String* actioncode = emit_action(n);
        String* tm;
        SwigType *d = Getattr(n, "type");
        if ((tm = Swig_typemap_lookup_out("out", n, "result", f, actioncode))) {
            Replaceall(tm, "$result", "resultobj");
            Printf(f->code, "%s\n", tm);
            Delete(tm);
        }
        else {
            Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(d, 0), name);
        }
        emit_return_variable(n, d, f);

        Append(f->code, "    RETURN(env, resultobj);\n}\n");
        Wrapper_print(f, this->f_wrappers);

        DelWrapper(f);

        return SWIG_OK;
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
        f_wrappers = NewString("");

        Swig_register_filebyname("header", f_header);
        Swig_register_filebyname("wrapper", f_wrappers);
        Swig_register_filebyname("begin", f_begin);
        Swig_register_filebyname("runtime", f_runtime);
        Swig_register_filebyname("init", f_init);

        this->methods = NewString("");

        Swig_banner(f_begin);

        module = Copy(Getattr(n, "name"));
        Insert(module, 0, "_");
        Printf(f_header, "#define SWIG_init YogInit_%s\n", module);

        Printf(f_wrappers, "#ifdef __cplusplus\n");
        Printf(f_wrappers, "extern \"C\" {\n");
        Printf(f_wrappers, "#endif\n");
        Append(this->methods, "static WrapperDef functions[] = { \n");

        Language::top(n);

        Append(this->methods, "    { NULL, NULL }\n");
        Append(this->methods, "};\n");
        Printf(f_wrappers, "%s\n", this->methods);

        Printf(f_wrappers, "#ifdef __cplusplus\n");
        Printf(f_wrappers, "}\n");
        Printf(f_wrappers, "#endif\n");

        Dump(f_runtime, f_begin);
        Dump(f_header, f_begin);
        Dump(f_wrappers, f_begin);
        Wrapper_pretty_print(f_init, f_begin);

        Delete(f_header);
        Delete(f_wrappers);
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
