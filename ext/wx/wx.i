%module wx

%native(set_client_data)    YogVal set_client_data(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block);

%{
#include <alloca.h>
#include "wx/wx.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

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
        PTR_AS(SwigYogObject, this->ptr->val)->ptr = NULL;
        YogEnv* env = YogVM_get_env(this->vm);
        YogVM_free_indirect_ptr(env, this->vm, this->ptr);
    }
};

YogVal
set_client_data(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal shadow = YUNDEF;
    PUSH_LOCAL(env, shadow);
    shadow = YogArray_at(env, args, 0);
    wxEvtHandler* handler = (wxEvtHandler*)PTR_AS(SwigYogObject, shadow)->ptr;

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
    _wx.wxEntryStart(ARGV.size, ARGV)\n \
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

bool wxEntryStart(int& argc, wxChar** argv);

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=cpp
 */
