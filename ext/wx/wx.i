%module wx

%native(set_client_data)    YogVal set_client_data(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block);

%{
#include <alloca.h>
#include "wx/wx.h"
#include "yog/eval.h"
#include "yog/function.h"
#include "yog/get_args.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

class Callback: public wxObject
{
private:
    YogVM* vm;
    YogIndirectPointer* pkg;
    YogIndirectPointer* func;
public:
    Callback(YogEnv* env, YogVal pkg, YogVal func)
    {
        YogVM* vm = env->vm;
        this->vm = vm;
        this->pkg = YogVM_alloc_indirect_ptr(env, vm, pkg);
        this->func = YogVM_alloc_indirect_ptr(env, vm, func);
    }

    ~Callback()
    {
        YogEnv* env = YogVM_get_env(this->vm);
        YogVM_free_indirect_ptr(env, this->vm, this->func);
        YogVM_free_indirect_ptr(env, this->vm, this->pkg);
    }

    void OnEvent(wxEvent& event)
    {
        Callback* self = (Callback*)event.m_callbackUserData;
        YogEnv* env = YogVM_get_env(self->vm);
        YOG_ASSERT(env, env != NULL, "env not found");
        SAVE_LOCALS(env);
        YogVal proxy = YUNDEF;
        YogVal shadow = YUNDEF;
        PUSH_LOCALS2(env, proxy, shadow);
        YogVal args[] = { YUNDEF };
        PUSH_LOCALSX(env, 1, args);

        wxString class_name = event.GetClassInfo()->GetClassName();
        wxString swig_name(class_name);
        swig_name.Append(wxT(" *"));
        swig_type_info* swig_type = SWIG_TypeQuery(swig_name.mb_str());
        YOG_ASSERT(env, swig_type != NULL, "swig_type_info (%s) not found", swig_name.mb_str());
        YogIndirectPointer* klass = (YogIndirectPointer*)swig_type->clientdata;
        proxy = YogEval_call_method(env, klass->val, "new", 0, NULL);
        shadow = Shadow_new(env, this->pkg->val, &event, swig_type, 0);
        YogObj_set_attr(env, proxy, "this", shadow);
        args[0] = proxy;
        YogCallable_call(env, self->func->val, array_sizeof(args), args);

        RETURN_VOID(env);
    }
};

class ClientData: public wxClientData
{
private:
    YogVM* vm;
    YogIndirectPointer* ptr;
public:
    ClientData(YogVM* vm, YogIndirectPointer* ptr): vm(vm), ptr(ptr)
    {
    }

    ~ClientData()
    {
        PTR_AS(Shadow, this->ptr->val)->ptr = NULL;
        YogEnv* env = YogVM_get_env(this->vm);
        YogVM_free_indirect_ptr(env, this->vm, this->ptr);
    }
};

YogVal
set_client_data(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal shadow = YUNDEF;
    PUSH_LOCAL(env, shadow);
    YogCArg params[] = { { "shadow", &shadow }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "set_client_data", params, args, kw);

    wxEvtHandler* handler = (wxEvtHandler*)PTR_AS(Shadow, shadow)->ptr;

    YogVM* vm = env->vm;
    YogIndirectPointer* ptr = YogVM_alloc_indirect_ptr(env, vm, shadow);
    handler->SetClientData(new ClientData(vm, ptr));

    RETURN(env, YNIL);
}
%}

%typemap(in) wxString& (wxString s) {
    s.assign(STRING_CSTR($input));
    $1 = &s;
}
%typemap(out) wxString& {
    $result = YogString_new_str(env, $1->wx_str());
}

%typemap(in) Callback* {
    $1 = new Callback(env, pkg, $input);
}

%typemap(in) (int& argc, wxChar** argv) (int temp) {
    if (IS_PTR($input)) {
        size_t size = YogArray_size(env, $input);
        if (size == 0) {
            temp = 0;
            $2 = NULL;
        }
        else {
            temp = size;
            $2 = (wxChar**)alloca(sizeof(wxChar*) * size);
            for (size_t i = 0; i < size; i++) {
                YogVal s = YogArray_at(env, $input, i);
                $2[i] = (wxChar*)alloca(sizeof(wxChar) * STRING_SIZE(s));
            }
        }
    }
    else {
        temp = 0;
        $2 = NULL;
    }
    $1 = &temp;
}

#define wxWindowID int

class wxObject
{
};

class wxEvtHandler: public wxObject
{
public:
    %extend {
        void Connect(int winid, int lastId, int eventType, Callback* callback)
        {
            $self->Connect(winid, lastId, eventType, (wxObjectEventFunction)&Callback::OnEvent, callback);
        }
    }
};

class wxWindowBase: public wxEvtHandler
{
public:
    virtual void SetLabel(const wxString& label) = 0;
    virtual bool Show(bool show);
};

class wxWindow: public wxWindowBase
{
public:
    virtual void SetLabel(const wxString& label);
};

class wxTopLevelWindow: public wxWindow
{
};

class wxFrameBase: public wxTopLevelWindow
{
};

class wxFrame: public wxFrameBase
{
public:
    %feature("yogappend") wxFrame "set_client_data(self.this)"
    wxFrame(wxWindow *parent, wxWindowID id, const wxString& title);
};

class wxAppConsole: public wxEvtHandler
{
public:
    virtual bool Initialize(int& argc, wxChar** argv);
    virtual bool OnInitGui();
    virtual int OnRun() = 0;
};

class wxAppBase: public wxAppConsole
{
public:
    void SetTopWindow(wxWindow *win);
    virtual bool CallOnInit();
    virtual int MainLoop();
    virtual int OnRun();
    virtual bool Yield(bool onlyIfNeeded) = 0;
};

class wxApp: public wxAppBase
{
    %feature("yogappend") wxApp "\n\
    _wx.wxEntryStart(ARGV)\n \
    try\n\
      handler = self.get_attr(\"OnPreInit\")\n\
    except AttributeError\n\
    else\n\
      handler()\n\
    end\n\
    try\n\
      handler = self.get_attr(\"OnInit\")\n\
    except AttributeError\n\
    else\n\
      handler()\n\
    end\n"
public:
    virtual bool Yield(bool onlyIfNeeded);
};

class wxEvent: public wxObject
{
public:
    virtual wxEvent *Clone() const = 0;
};

class wxMouseEvent: public wxEvent
{
};

bool wxEntryStart(int& argc, wxChar** argv);

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=cpp
 */
