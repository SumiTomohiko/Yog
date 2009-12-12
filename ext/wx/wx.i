%module wx

%native(set_client_data)    YogVal set_client_data(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block);

%{
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

void RegisterModules()
{
    wxModule::RegisterModules();
}

void InitializeModules()
{
    wxModule::InitializeModules();
}

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
public:
    virtual bool Yield(bool onlyIfNeeded);
};

void RegisterModules();
void InitializeModules();

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=cpp
 */
