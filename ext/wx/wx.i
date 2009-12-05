%module wx

%{
#include "wx/wx.h"
#include "yog/string.h"

void RegisterModules()
{
    wxModule::RegisterModules();
}

void InitializeModules()
{
    wxModule::InitializeModules();
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
