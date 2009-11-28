#include <assert.h>
#include "swigmod.h"

#define BUF_SIZE    1024

class YOG: public Language {
private:
    String* class_name;
    String* module;
    String* methods;

    File* f_begin;
    File* f_runtime;
    File* f_init;
    File* f_header;
    File* f_wrappers;
    String* f_shadow;

    bool have_constructor;
    bool shadow;
    bool in_class;

    void add_function(String* name) {
        Printf(this->methods, "    { \"%s\", %s },\n", name, Swig_name_wrapper(name));
    }

    String* funcCall(String* name) {
        return this->funcCall(name, "*args, **kw");
    }

    String* funcCall(String* name, const char* params) {
        String *str = NewString("");
        Printv(str, module, ".", name, "(", params, ")", NIL);
        return str;
    }

    void emitFunctionShadowHelper(File* f_dest, String* name) {
        Printv(f_dest,
            "\n"
            "def ", name, "(*args, **kw)\n"
            "  return ", this->funcCall(name), "\n"
            "end\n",
            NIL);
    }

    int emit_class_head(Node* n) {
        if (!this->shadow) {
            return SWIG_OK;
        }
        this->have_constructor = false;
        this->class_name = Getattr(n, "sym:name");
        if (!this->addSymbol(class_name, n)) {
            return SWIG_ERROR;
        }

        String *base_class = NewString("");
        List *baselist = Getattr(n, "baselist");
        if (baselist && (0 < Len(baselist))) {
            Iterator b = First(baselist);
            while (b.item) {
                if (Strcmp(base_class, "") != 0) {
                    return SWIG_ERROR;
                }
                base_class = Getattr(b.item, "name");
                break;
            }
        }

        Printv(this->f_shadow, "class ", class_name, NIL);
        if (Len(base_class)) {
            Printf(this->f_shadow, " > %s", base_class);
        }
        Printf(this->f_shadow, "\n");

        Printv(this->f_shadow,
                "  def own_this()\n"
                "    return self.this.own(true)\n"
                "  end\n"
                "\n"
                "  def disown_this()\n"
                "    return self.this.own(false)\n"
                "  end\n",
                NIL);

        return SWIG_OK;
    }

    void add_method(String* name, String* function) {
        Printf(this->methods, "    { \"%s\", %s },\n", name, function);
        Append(this->methods, "},\n");
    }

    int emit_class_tail(Node* n) {
        if (!this->shadow) {
            return SWIG_OK;
        }

        String* real_classname = Getattr(n, "name");
        SwigType* ct = Copy(real_classname);
        SwigType_add_pointer(ct);
        SwigType* realct = Copy(real_classname);
        SwigType_add_pointer(realct);
        SwigType_remember(realct);
        Printv(this->f_wrappers,
            "\n"
            "SWIGINTERN YogVal ", this->class_name, "_swigregister(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)\n"
            "{\n"
            "    SAVE_ARGS4(env, self, args, kw, block);\n"
            "    YogVal obj = YUNDEF;\n"
            "    PUSH_LOCAL(env, obj);\n"
            "    obj = YogArray(env, args, 0);\n"
            "    SWIG_TypeNewClientData(SWIGTYPE", SwigType_manglestr(ct), ", SWIG_NewClientData(obj));\n"
            "    RETURN(env, YNIL);\n"
            "}\n",
            NIL);

        String *cname = NewStringf("%s_swigregister", this->class_name);
        this->add_method(cname, cname);
        Delete(cname);
        Delete(ct);
        Delete(realct);

        if (!this->have_constructor) {
            Printv(this->f_shadow,
                "\n"
                "  def init(*args, **kw)\n"
                "    raise AttributeError.new(\"No constructor defined\")\n"
                "  end\n",
                NIL);
        }
        Printv(this->f_shadow,"end\n", NIL);
        Printf(this->f_shadow, "%s.%s_swigregister(%s)\n", module, class_name, class_name);

        return SWIG_OK;
    }

    virtual int classHandler(Node* n) {
        if (this->emit_class_head(n) != SWIG_OK) {
            return SWIG_ERROR;
        }

        this->in_class = true;
        Language::classHandler(n);
        this->in_class = false;

        if (this->emit_class_tail(n) != SWIG_OK) {
            return SWIG_ERROR;
        }

        return SWIG_OK;
    }

    virtual int memberfunctionHandler(Node *n) {
        bool oldshadow = this->shadow;
        Language::memberfunctionHandler(n);
        this->shadow = oldshadow;

        if (Getattr(n, "sym:nextSibling")) {
            return SWIG_OK;
        }
        if (!this->shadow) {
            return SWIG_OK;
        }
        String *symname = Getattr(n, "sym:name");
        if (Getattr(n, "feature:shadow")) {
            String* code = Getattr(n, "feature:shadow");
            String* action = NewStringf("%s.%s", module, Swig_name_member(class_name, symname));
            Replaceall(code, "$action", action);
            Delete(action);
            Printv(this->f_shadow, code, "\n", NIL);
            Delete(code);
            return SWIG_OK;
        }

        Printv(this->f_shadow,
            "\n"
            "  def ", symname, "(*args, **kw)\n"
            "    return ", this->funcCall(Swig_name_member(class_name, symname), "self.this, *args, **kw"), "\n"
            "  end\n",
            NIL);

        return SWIG_OK;

#if 0
        Printv(f_shadow, tab4, "def ", symname, "(",parms , ")", returnTypeAnnotation(n), ":", NIL);
        Printv(f_shadow, "\n", NIL);
        if (have_docstring(n)) {
            Printv(f_shadow, tab8, docstring(n, AUTODOC_METHOD, tab8), "\n", NIL);
        }
        if (have_pythonprepend(n)) {
            Printv(f_shadow, pythonprepend(n), "\n", NIL);
        }
        if (have_pythonappend(n)) {
            Printv(f_shadow, tab8, "val = ", funcCall(Swig_name_member(class_name, symname), callParms), "\n", NIL);
            Printv(f_shadow, pythonappend(n), "\n", NIL);
            Printv(f_shadow, tab8, "return val\n\n", NIL);
            return SWIG_OK;
        }

        Printv(f_shadow, tab8, "return ", funcCall(Swig_name_member(class_name, symname), callParms), "\n\n", NIL);

        return SWIG_OK;
#endif
    }

public:
    YOG() {
        this->class_name = NULL;
        this->module = NULL;
        this->methods = NULL;
        this->f_begin = NULL;
        this->f_runtime = NULL;
        this->f_init = NULL;
        this->f_header = NULL;
        this->f_wrappers = NULL;
        this->f_shadow = NULL;
        this->shadow = true;
        this->have_constructor = false;
        this->in_class = false;
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

    void emit_constructor(Node* n) {
        String* symname = Getattr(n, "sym:name");
        if (Getattr(n, "feature:shadow")) {
            String* code = Getattr(n, "feature:shadow");
            String* action = NewStringf("%s.%s", module, Swig_name_construct(symname));
            Replaceall(code, "$action", action);
            Delete(action);
            Printv(this->f_shadow, code, "\n", NIL);
            Delete(code);
            return;
        }

        Printv(this->f_shadow,
            "\n"
            "  def init(*args, **kw)\n"
            "    self.this = ", funcCall(Swig_name_construct(symname)), "\n"
            "  end\n",
            NIL);
    }

    virtual int constructorHandler(Node* n) {
        Language::constructorHandler(n);

        if (Getattr(n, "sym:nextSibling")) {
            return SWIG_OK;
        }
        if (!this->shadow) {
            return SWIG_OK;
        }
        bool handled_as_init = false;
        if (!this->have_constructor) {
            String *nname = Getattr(n, "sym:name");
            String *sname = Getattr(getCurrentClass(), "sym:name");
            String *cname = Swig_name_construct(sname);
            handled_as_init = (Strcmp(nname, sname) == 0) || (Strcmp(nname, cname) == 0);
            Delete(cname);
        }

        if (!this->have_constructor && handled_as_init) {
            this->emit_constructor(n);
            this->have_constructor = true;
        }

        return SWIG_OK;
    }

    virtual int functionWrapper(Node* n) {
        String *name = Getattr(n, "name");
        this->add_function(name);

        String* nodeType = Getattr(n, "nodeType");
        int constructor = !Cmp(nodeType, "constructor");
        bool handled_as_init = false;
        if (!this->have_constructor && (constructor || Getattr(n, "handled_as_constructor"))) {
            String *nname = Getattr(n, "sym:name");
            String *sname = Getattr(getCurrentClass(), "sym:name");
            String *cname = Swig_name_construct(sname);
            handled_as_init = (Strcmp(nname, sname) == 0) || (Strcmp(nname, cname) == 0);
            Delete(cname);
        }

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
            if (handled_as_init) {
                Replaceall(tm, "$owner", "SWIG_POINTER_NEW");
            }
            else if (GetFlag(n, "feature:new")) {
                Replaceall(tm, "$owner", "SWIG_POINTER_OWN");
            }
            else {
                Replaceall(tm, "$owner", "0");
            }
            Printf(f->code, "%s\n", tm);
            Delete(tm);
        }
        else {
            Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(d, 0), name);
        }
        emit_return_variable(n, d, f);

        Append(f->code, "    RETURN(env, resultobj);\n}\n");
        Wrapper_print(f, this->f_wrappers);

        if (!this->in_class) {
            this->emitFunctionShadowHelper(this->f_shadow, iname);
        }

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
        this->f_runtime = NewString("");
        this->f_init = NewString("");
        this->f_header = NewString("");
        this->f_wrappers = NewString("");

        Swig_register_filebyname("header", this->f_header);
        Swig_register_filebyname("wrapper", this->f_wrappers);
        Swig_register_filebyname("begin", this->f_begin);
        Swig_register_filebyname("runtime", this->f_runtime);
        Swig_register_filebyname("init", this->f_init);

        this->methods = NewString("");

        Swig_banner(f_begin);

        this->module = Copy(Getattr(n, "name"));
        String *filen = NewStringf("%s%s.yg", SWIG_output_directory(), Char(this->module));
        String* f_shadow_yg;
        if ((f_shadow_yg = NewFile(filen, "w", SWIG_output_files())) == 0) {
            FileErrorDisplay(filen);
            SWIG_exit(EXIT_FAILURE);
        }

        Insert(this->module, 0, "_");
        Printf(f_header, "#define SWIG_init YogInit_%s\n", this->module);

        Printf(f_wrappers, "#ifdef __cplusplus\n");
        Printf(f_wrappers, "extern \"C\" {\n");
        Printf(f_wrappers, "#endif\n");
        Append(this->methods, "static WrapperDef functions[] = { \n");

        this->f_shadow = NewString("");
        Swig_banner_target_lang(this->f_shadow, "#");
        Printf(this->f_shadow, "import %s\n", this->module);

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

        Printv(f_shadow_yg, this->f_shadow, "\n", NIL);
        Close(f_shadow_yg);
        Delete(f_shadow_yg);

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
