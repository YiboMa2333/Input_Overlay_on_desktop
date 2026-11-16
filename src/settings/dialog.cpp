// SPDX-License-Identifier: GPL-2.0-only
#include "dialog.hpp"
#include <commctrl.h>
#include <commdlg.h>
#include <cwchar>
#include <cerrno>
#include <cmath>
namespace overlay {
namespace {
using namespace settings_control;
constexpr const wchar_t* key_names[]{L"Q",L"W",L"E",L"Shift",L"A",L"S",L"D",L"Ctrl",L"Space"};
void check(HWND w,int id,bool value) { CheckDlgButton(w,id,value?BST_CHECKED:BST_UNCHECKED); }
bool checked(HWND w,int id) { return IsDlgButtonChecked(w,id)==BST_CHECKED; }
void integer(HWND w,int id,int value) { SetDlgItemInt(w,id,value,FALSE); }
bool number(HWND w,int id,int low,int high,int& result) {
    wchar_t text[64]{};GetDlgItemTextW(w,id,text,64);wchar_t* end{};errno=0;
    const auto value=std::wcstoll(text,&end,10);
    if(errno||end==text||*end||value<low||value>high) {
        auto message=L"Enter a whole number from "+std::to_wstring(low)+L" to "+std::to_wstring(high)+L".";
        MessageBoxW(w,message.c_str(),L"Check settings",MB_ICONINFORMATION);SetFocus(GetDlgItem(w,id));return false;
    }
    result=static_cast<int>(value);return true;
}
}
SettingsDialog::~SettingsDialog() { if(window_) DestroyWindow(window_);if(font_) DeleteObject(font_); }
bool SettingsDialog::dispatch(MSG& msg) const { return window_&&IsDialogMessageW(window_,&msg); }
HWND SettingsDialog::add(const wchar_t* klass,const wchar_t* text,int id,int page,int x,int y,int w,int h,DWORD extra) {
    auto child=CreateWindowExW(std::wcscmp(klass,L"EDIT")==0?WS_EX_CLIENTEDGE:0,klass,text,
        WS_CHILD|WS_VISIBLE|extra,0,0,0,0,window_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    controls_.push_back({child,page,x,y,w,h});return child;
}
void SettingsDialog::build() {
    auto tab=add(WC_TABCONTROLW,L"",Tab,-1,12,12,576,444,WS_TABSTOP);
    for(const auto text : {L"Appearance",L"Layout",L"Behavior"}) { TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(text);TabCtrl_InsertItem(tab,TabCtrl_GetItemCount(tab),&item); }
    auto label=[&](const wchar_t* text,int page,int x,int y,int w=250) { add(L"STATIC",text,0,page,x,y,w,23); };
    auto edit=[&](int id,int page,int x,int y,int w=90) { add(L"EDIT",L"",id,page,x,y,w,25,WS_TABSTOP|ES_AUTOHSCROLL); };
    auto box=[&](const wchar_t* text,int id,int page,int y) { add(L"BUTTON",text,id,page,30,y,520,25,BS_AUTOCHECKBOX|WS_TABSTOP); };
    auto combo=[&](int id,int page,int x,int y,int w) { return add(WC_COMBOBOXW,L"",id,page,x,y,w,190,CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP); };
    add(L"BUTTON",L"Pressed color...",Pressed,0,30,54,245,28,BS_PUSHBUTTON|WS_TABSTOP);
    add(L"BUTTON",L"Border color...",Border,0,300,54,245,28,BS_PUSHBUTTON|WS_TABSTOP);
    const wchar_t* labels[]{L"Border thickness (0.5-5.0)",L"Key background opacity (0-100%)",L"Overall opacity (5-100%)",L"Font family",L"Font size (6-36)",L"Overlay scale (50-400%)",L"Keyboard/mouse spacing (8-160)"};
    const int ids[]{Thickness,Background,Opacity,Family,FontSize,Scale,Spacing};
    for(int i=0;i<7;++i) { label(labels[i],0,30,98+i*39,345);edit(ids[i],0,385,94+i*39,i==3?155:100); }
    add(L"BUTTON",L"Choose font...",FontPicker,0,30,381,180,28,BS_PUSHBUTTON|WS_TABSTOP);
    label(L"Changes preview on Apply. Edit mode remains at least 40% visible.",0,30,420,530);
    label(L"Built-in preset",1,30,60);auto presets=combo(Preset,1,280,56,260);
    for(auto text:{L"Compact gaming",L"WASD + modifiers",L"Single row"}) SendMessageW(presets,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
    label(L"Select a physical key to arrange",1,30,108);auto keys=combo(KeySelect,1,280,104,260);
    for(auto text:key_names) SendMessageW(keys,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
    box(L"Show selected key",KeyEnabled,1,150);
    label(L"X position (8-1000)",1,30,199);edit(KeyX,1,220,195);
    label(L"Y position (8-600)",1,30,238);edit(KeyY,1,220,234);
    add(L"STATIC",L"Positions use HUD units. Keys may overlap intentionally.\nSelect another key to edit it; Apply saves all positions.",0,1,30,278,510,48);
    box(L"Show mouse diagram",Mouse,1,332);
    add(L"BUTTON",L"Reset layout to default",ResetLayout,1,30,380,260,30,BS_PUSHBUTTON|WS_TABSTOP);
    box(L"Always on top of ordinary windows",Topmost,2,54);
    box(L"Edit mode (unchecked = Locked / click-through)",Edit,2,88);
    box(L"Lock aspect ratio",Aspect,2,122);
    box(L"Start with Windows (current user only)",Startup,2,156);
    box(L"Launch hidden to the system tray on next start",Tray,2,190);
    box(L"Enable global Edit/Locked shortcut",Hotkey,2,224);
    label(L"Shortcut",2,30,270);auto mods=combo(Modifiers,2,150,264,225);
    for(auto text:{L"Ctrl + Alt",L"Ctrl + Shift",L"Alt + Shift",L"Ctrl + Alt + Shift"}) SendMessageW(mods,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
    auto functions=combo(FunctionKey,2,400,264,140);
    for(int i=1;i<=11;++i) {auto s=L"F"+std::to_wstring(i);SendMessageW(functions,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s.c_str()));}
    label(L"If another app owns the shortcut, the tray controls still work.",2,30,310,520);
    add(L"BUTTON",L"Reset position and size on Apply",ResetGeometry,2,30,352,350,30,BS_PUSHBUTTON|WS_TABSTOP);
    label(L"Exit the application from the tray menu. Startup uses this EXE's path.",2,30,407,530);
    add(L"STATIC",L"",Status,-1,20,469,345,40);
    add(L"BUTTON",L"Apply",Apply,-1,375,474,95,30,BS_DEFPUSHBUTTON|WS_TABSTOP);
    add(L"BUTTON",L"Close",Close,-1,485,474,95,30,BS_PUSHBUTTON|WS_TABSTOP);
    populate();reflow();
}
void SettingsDialog::reflow() {
    const auto dpi=GetDpiForWindow(window_);auto next=CreateFontW(-MulDiv(12,dpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    for(auto c:controls_) {
        SetWindowPos(c.window,nullptr,MulDiv(c.x,dpi,96),MulDiv(c.y,dpi,96),MulDiv(c.w,dpi,96),MulDiv(c.h,dpi,96),SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(c.window,WM_SETFONT,reinterpret_cast<WPARAM>(next),TRUE);
        ShowWindow(c.window,c.page<0||c.page==page_?SW_SHOW:SW_HIDE);
    }
    if(font_) DeleteObject(font_);font_=next;
}
void SettingsDialog::populate() {
    wchar_t thickness[16]{};std::swprintf(thickness,16,L"%.1f",draft_.border_tenths/10.0);SetDlgItemTextW(window_,Thickness,thickness);
    integer(window_,Background,draft_.background_opacity);
    integer(window_,Opacity,draft_.overlay_opacity);integer(window_,FontSize,draft_.font_size);
    integer(window_,Scale,draft_.scale);integer(window_,Spacing,draft_.spacing);
    SetDlgItemTextW(window_,Family,draft_.font.c_str());
    check(window_,Mouse,draft_.show_mouse);check(window_,Topmost,draft_.always_on_top);check(window_,Edit,draft_.edit);
    check(window_,Aspect,draft_.aspect_lock);check(window_,Startup,draft_.start_windows);check(window_,Tray,draft_.launch_tray);
    check(window_,Hotkey,draft_.hotkey_enabled);
    const int mods=draft_.hotkey_modifiers==6?1:draft_.hotkey_modifiers==5?2:draft_.hotkey_modifiers==7?3:0;
    SendDlgItemMessageW(window_,Modifiers,CB_SETCURSEL,mods,0);
    SendDlgItemMessageW(window_,FunctionKey,CB_SETCURSEL,draft_.hotkey_vk>=VK_F1&&draft_.hotkey_vk<=VK_F11?draft_.hotkey_vk-VK_F1:7,0);
    SendDlgItemMessageW(window_,Preset,CB_SETCURSEL,draft_.preset,0);
    SendDlgItemMessageW(window_,KeySelect,CB_SETCURSEL,selected_key_,0);editor();
    for(auto pair:{std::pair{Pressed,draft_.pressed_color},std::pair{Border,draft_.border_color}}) {
        wchar_t text[64]{};std::swprintf(text,64,pair.first==Pressed?L"Pressed: #%06X...":L"Border: #%06X...",pair.second);
        SetDlgItemTextW(window_,pair.first,text);
    }
}
void SettingsDialog::editor() {
    const auto& key=draft_.keys[selected_key_];check(window_,KeyEnabled,key.enabled);
    integer(window_,KeyX,key.x);integer(window_,KeyY,key.y);
}
bool SettingsDialog::store_editor() {
    auto& key=draft_.keys[selected_key_];
    if(!number(window_,KeyX,8,1000,key.x)||!number(window_,KeyY,8,600,key.y)) return false;
    key.enabled=checked(window_,KeyEnabled);return true;
}
bool SettingsDialog::collect() {
    wchar_t text[64]{};GetDlgItemTextW(window_,Thickness,text,64);wchar_t* end{};errno=0;
    const double thickness=std::wcstod(text,&end);
    if(errno||end==text||*end||!std::isfinite(thickness)||thickness<0.5||thickness>5) {
        MessageBoxW(window_,L"Border thickness must be between 0.5 and 5.0.",L"Check settings",MB_ICONINFORMATION);return false;
    }
    draft_.border_tenths=static_cast<int>(std::lround(thickness*10));
    if(!store_editor()||!number(window_,Background,0,100,draft_.background_opacity)
        ||!number(window_,Opacity,5,100,draft_.overlay_opacity)||!number(window_,FontSize,6,36,draft_.font_size)
        ||!number(window_,Scale,50,400,draft_.scale)||!number(window_,Spacing,8,160,draft_.spacing)) return false;
    wchar_t family[256]{};GetDlgItemTextW(window_,Family,family,256);draft_.font=family;
    draft_.show_mouse=checked(window_,Mouse);draft_.always_on_top=checked(window_,Topmost);draft_.edit=checked(window_,Edit);
    draft_.aspect_lock=checked(window_,Aspect);draft_.start_windows=checked(window_,Startup);draft_.launch_tray=checked(window_,Tray);
    draft_.hotkey_enabled=checked(window_,Hotkey);
    constexpr unsigned mods[]{3,6,5,7};const auto mod=SendDlgItemMessageW(window_,Modifiers,CB_GETCURSEL,0,0);
    draft_.hotkey_modifiers=mods[mod>=0&&mod<4?mod:0];
    draft_.hotkey_vk=VK_F1+static_cast<unsigned>(SendDlgItemMessageW(window_,FunctionKey,CB_GETCURSEL,0,0));
    draft_.validate();return true;
}
void SettingsDialog::color(bool pressed) {
    static COLORREF custom[16]{};const auto c=pressed?draft_.pressed_color:draft_.border_color;
    CHOOSECOLORW choose{};choose.lStructSize=sizeof(choose);choose.hwndOwner=window_;choose.lpCustColors=custom;
    choose.rgbResult=RGB((c>>16)&255,(c>>8)&255,c&255);choose.Flags=CC_RGBINIT|CC_FULLOPEN;
    if(ChooseColorW(&choose)) {
        auto value=(GetRValue(choose.rgbResult)<<16)|(GetGValue(choose.rgbResult)<<8)|GetBValue(choose.rgbResult);
        (pressed?draft_.pressed_color:draft_.border_color)=value;
        wchar_t text[64]{};std::swprintf(text,64,pressed?L"Pressed: #%06X...":L"Border: #%06X...",value);
        SetDlgItemTextW(window_,pressed?Pressed:Border,text);
    }
}
void SettingsDialog::choose_font() {
    LOGFONTW font{};wchar_t family[LF_FACESIZE]{};GetDlgItemTextW(window_,Family,family,LF_FACESIZE);
    std::wcsncpy(font.lfFaceName,family,LF_FACESIZE-1);font.lfHeight=-MulDiv(draft_.font_size,GetDpiForWindow(window_),96);
    CHOOSEFONTW choose{};choose.lStructSize=sizeof(choose);choose.hwndOwner=window_;choose.lpLogFont=&font;
    choose.Flags=CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_NOSTYLESEL|CF_LIMITSIZE;choose.nSizeMin=5;choose.nSizeMax=27;
    if(ChooseFontW(&choose)) {SetDlgItemTextW(window_,Family,font.lfFaceName);integer(window_,FontSize,choose.iPointSize*96/720);}
}
LRESULT CALLBACK SettingsDialog::procedure(HWND w,UINT msg,WPARAM wp,LPARAM lp) {
    auto self=reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    if(msg==WM_NCCREATE) {self=static_cast<SettingsDialog*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window_=w;SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    return self?self->message(msg,wp,lp):DefWindowProcW(w,msg,wp,lp);
}
LRESULT SettingsDialog::message(UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_NOTIFY:
        if(reinterpret_cast<NMHDR*>(lp)->idFrom==Tab&&reinterpret_cast<NMHDR*>(lp)->code==TCN_SELCHANGE) {page_=TabCtrl_GetCurSel(GetDlgItem(window_,Tab));reflow();}return 0;
    case WM_COMMAND:
        switch(LOWORD(wp)) {
        case Pressed:color(true);break;case Border:color(false);break;case FontPicker:choose_font();break;
        case KeySelect:
            if(HIWORD(wp)==CBN_SELCHANGE) {if(store_editor()) {selected_key_=static_cast<int>(SendDlgItemMessageW(window_,KeySelect,CB_GETCURSEL,0,0));editor();}
                else SendDlgItemMessageW(window_,KeySelect,CB_SETCURSEL,selected_key_,0);}break;
        case Preset:
            if(HIWORD(wp)==CBN_SELCHANGE) {draft_.layout_preset(static_cast<int>(SendDlgItemMessageW(window_,Preset,CB_GETCURSEL,0,0)));editor();check(window_,Mouse,true);integer(window_,Spacing,draft_.spacing);}break;
        case ResetLayout:draft_.layout_preset(0);editor();check(window_,Mouse,true);integer(window_,Spacing,32);SendDlgItemMessageW(window_,Preset,CB_SETCURSEL,0,0);break;
        case ResetGeometry:reset_=true;SetDlgItemTextW(window_,Status,L"Position and size will reset on Apply.");break;
        case Apply:
            if(collect()) {
                if(apply_(draft_,reset_)) {if(reset_) {draft_.scale=100;integer(window_,Scale,100);}reset_=false;SetDlgItemTextW(window_,Status,L"Applied and saved. Launch-to-tray takes effect next start.");}
                else MessageBoxW(window_,L"Could not apply/save all settings. Check startup registration, rendering or the configuration directory.",L"Settings",MB_ICONERROR);
            }break;
        case IDCANCEL:case Close:DestroyWindow(window_);break;
        }return 0;
    case WM_DPICHANGED: {auto r=reinterpret_cast<RECT*>(lp);SetWindowPos(window_,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER);reflow();return 0;}
    case WM_CLOSE:DestroyWindow(window_);return 0;
    case DM_GETDEFID:return MAKELRESULT(Apply,DC_HASDEFID);
    case WM_DESTROY:controls_.clear();return 0;
    case WM_NCDESTROY: {auto old=window_;SetWindowLongPtrW(old,GWLP_USERDATA,0);window_=nullptr;return DefWindowProcW(old,msg,wp,lp);}
    }
    return DefWindowProcW(window_,msg,wp,lp);
}
void SettingsDialog::show(HWND owner,const Settings& settings,std::function<bool(Settings,bool)> callback) {
    if(window_) {ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);return;}
    draft_=settings;apply_=std::move(callback);selected_key_=0;page_=0;reset_=false;
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_TAB_CLASSES|ICC_STANDARD_CLASSES};InitCommonControlsEx(&controls);
    WNDCLASSW c{};c.lpfnWndProc=procedure;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"InputOverlaySettings";
    c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hIcon=LoadIconW(c.hInstance,MAKEINTRESOURCEW(101));
    c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);RegisterClassW(&c);
    const auto dpi=GetDpiForWindow(owner);RECT rect{0,0,MulDiv(600,dpi,96),MulDiv(520,dpi,96)};
    const DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;AdjustWindowRectExForDpi(&rect,style,FALSE,WS_EX_CONTROLPARENT,dpi);
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);GetMonitorInfoW(MonitorFromWindow(owner,MONITOR_DEFAULTTONEAREST),&monitor);
    const auto width=rect.right-rect.left,height=rect.bottom-rect.top;
    window_=CreateWindowExW(WS_EX_CONTROLPARENT,c.lpszClassName,L"Input Overlay Settings",style,
        monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-width)/2,monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-height)/2,
        width,height,owner,nullptr,c.hInstance,this);
    if(!window_) return;build();ShowWindow(window_,SW_SHOWNORMAL);SetForegroundWindow(window_);
}
}
