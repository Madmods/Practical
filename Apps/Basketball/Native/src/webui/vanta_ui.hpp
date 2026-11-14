// vanta_ui.hpp — hosts the Vanta UI (vanta-ui/index.html) in a frameless, transparent Win32 window
// using Microsoft Edge WebView2, and bridges it to your C++ code.
//
//   vanta::UiHost ui;
//   vanta::UiCallbacks cb;            // fill in what your app supports (all optional)
//   cb.signIn = [](auto& user, auto& pass) { return vanta::AuthResult{ true }; };
//   ui.Create(hInstance, info, cb);
//   ui.Run();                         // message loop; returns when the window closes
//
// Threading: signIn / createAccount / runCheck / installUpdate run on a worker thread, so they may
// block (network, process scans). Everything else is called on the UI thread. SetPing / SetRoblox /
// Post are safe to call from any thread.
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellscalingapi.h>
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shcore.lib")
#include <wrl.h>
#include <WebView2.h>

#include <DispatcherQueue.h>
#undef GetCurrentTime
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <windows.ui.composition.interop.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <future>
#include <mutex>

#include "json_lite.hpp"

#ifndef VANTA_UI_RESOURCE
#define VANTA_UI_RESOURCE 101  // RCDATA id of the embedded index.html (see vanta.rc)
#endif

namespace vanta {

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

struct AuthResult {
    bool ok = false;
    std::wstring error;   // shown under the form when ok == false
    std::wstring field;   // optional: which field to highlight ("user", "pass", "email", "key")
};

struct CheckResult {
    bool ok = false;
    std::wstring result;  // pill text, e.g. "Detected", "Compatible", "Not running"
    std::wstring detail;  // small grey text under the check name (optional)
};

// What Windows draws behind the page's glass shapes.
enum class Glass {
    Frosted,  // live blur of the desktop/game behind the window (needs Windows "Transparency effects" on)
    None,     // no blur; the page glass is made more opaque instead
};

struct AppInfo {
    Glass glass = Glass::Frosted;
    std::wstring version = L"1.4.2";        // running build
    std::wstring latestVersion = L"1.4.2";  // newest published build; higher than version = "update found"
    std::wstring robloxVersion;             // shown in Home › Versions (optional)
    std::wstring title = L"Vanta";
    std::wstring product;                   // "football" switches the page to the Vanta Sports companion modules
    std::wstring user;                      // shown as the signed-in name when skipAuth is set
    std::wstring gameVersion;               // shown in Home › Versions for products with a game
    std::wstring dataName = L"VantaUI";     // %LOCALAPPDATA%\<dataName> holds the page's storage
    bool skipAuth = false;                  // no account step: loader goes straight to the connection checks
    bool debug = false;                     // enables DevTools + right-click menu
    std::wstring initialState = L"{}";
    bool testMode = false;
    double scale = 0;                       // UI size; 0 = fit to the monitor (2560×1440 at 100% → 1.0, 1080p → 0.75)
};

struct UiCallbacks {
    // worker thread — may block
    std::function<AuthResult(const std::wstring& user, const std::wstring& pass, bool remember)> signIn;
    std::function<AuthResult(const std::wstring& email, const std::wstring& user, const std::wstring& pass, const std::wstring& key)> createAccount;
    std::function<CheckResult(const std::wstring& id)> runCheck;  // ids: "client", "version", "game", "session"
    std::function<bool()> installUpdate;
    // UI thread
    std::function<void(const std::wstring& featureId, bool on)> featureChanged;
    std::function<void(const std::wstring& key, const json::Value& value)> settingChanged;  // e.g. "autotime.release" → 37
    std::function<void(const json::Value& snapshot)> modulesChanged;  // full module state after a profile load / reset
    std::function<bool()> workspaceOpened;
    std::function<void()> signedOut;
    std::function<void(const std::wstring& type, const json::Value& msg)> custom;  // any other message type from the page
};

class UiHost {
public:
    ~UiHost() { Shutdown(); }
    void Shutdown() {
        { std::lock_guard<std::mutex> lock(postMutex_); closing_ = true; }
        for (auto& task : workers_) if (task.valid()) task.wait();
        workers_.clear();
        if (hwnd_) DestroyWindow(hwnd_);
        panes_.clear(); backdrop_ = nullptr; glassRoot_ = nullptr;
        target_ = nullptr; compositor_ = nullptr; dispatcher_ = nullptr;
    }
    static constexpr UINT WM_APP_POST = WM_APP + 41;
    static constexpr int kPanelW = 1060, kPanelH = 700, kMargin = 24;  // DIPs; margin leaves room for the shadow
    static constexpr int kIslandW = 440, kIslandH = 150;  // room for the expanded island + its shadow

    bool Create(HINSTANCE inst, const AppInfo& info, UiCallbacks callbacks) {
        info_ = info;
        cb_ = std::move(callbacks);
        if (!PrepareUiFolder()) return false;

        WNDCLASSEXW wc{ sizeof(wc) };
        wc.lpfnWndProc = &UiHost::StaticProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));  // black = transparent on DWM glass
        wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
        wc.lpszClassName = L"VantaUiHost";
        RegisterClassExW(&wc);

        // created tiny, then sized and centred for whichever monitor it lands on (see FitToMonitor)
        hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, info_.title.c_str(),
                                WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU, 0, 0, 1, 1, nullptr, nullptr, inst, this);
        if (!hwnd_) return false;
        alwaysOnTop_ = info_.product == L"basketball";
        SetWindowPos(hwnd_,alwaysOnTop_?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        FitToMonitor(true);

        // Whole client area becomes DWM "glass": anything the page leaves transparent shows the desktop/game.
        MARGINS glass{ -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(hwnd_, &glass);
        DWORD corners = 1; /* DWMWCP_DONOTROUND — the page draws its own rounded corners */
        DwmSetWindowAttribute(hwnd_, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corners, sizeof(corners));
        frosted_ = info_.glass == Glass::Frosted && CreateBackdrop();
        // Nothing is visible until the page reports its glass shapes (see "shape"); if it never does,
        // the fallback timer shows the whole window so the app can't end up invisible.
        SetWindowRgn(hwnd_, CreateRectRgn(0, 0, 0, 0), FALSE);
        SetTimer(hwnd_, kShapeFallbackTimer, 5000, nullptr);
        if (info_.testMode) SetTimer(hwnd_, 73, 55000, nullptr);
        else if(info_.product == L"basketball") SetTimer(hwnd_, 74, 50, nullptr);
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);

        const std::wstring dataDir = LocalAppData() + L"\\" + info_.dataName + L"\\WebView2";  // own folder; your main app uses %LOCALAPPDATA%\Vanta
        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, dataDir.c_str(), nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    if (FAILED(result) || !env) return Fail(L"WebView2 could not start. Install the Microsoft Edge WebView2 Runtime.");
                    return env->CreateCoreWebView2Controller(hwnd_, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT res, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(res) || !controller) return Fail(L"WebView2 controller failed to start.");
                            return OnControllerReady(controller);
                        }).Get());
                }).Get());
        if (FAILED(hr)) { Fail(L"Microsoft Edge WebView2 Runtime is not installed."); return false; }
        return true;
    }

    int Run() {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

    // ---- push data into the page (any thread) ----
    void SetPing(int ms) { Post(L"{\"type\":\"ping\",\"ms\":" + std::to_wstring(ms) + L"}"); }
    void SetRobloxConnected(bool on) { Post(std::wstring(L"{\"type\":\"status\",\"roblox\":") + (on ? L"true" : L"false") + L"}"); }
    void Post(std::wstring jsonText) {
        std::lock_guard<std::mutex> lock(postMutex_);
        if (closing_) return;
        auto* payload = new std::wstring(std::move(jsonText));
        if (!hwnd_ || !PostMessageW(hwnd_, WM_APP_POST, 0, reinterpret_cast<LPARAM>(payload))) delete payload;
    }
    HWND Window() const { return hwnd_; }
    void Close(){closeApproved_=true;if(hwnd_)PostMessageW(hwnd_,WM_CLOSE,0,0);}
    void Capture(const std::wstring& path, std::function<void(bool)> done) {
        if(!info_.testMode || !webview_) { done(false); return; }
        ComPtr<IStream> stream;
        if(FAILED(SHCreateStreamOnFileEx(path.c_str(),STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE,FILE_ATTRIBUTE_NORMAL,TRUE,nullptr,&stream))) {done(false);return;}
        auto finished=done;
        HRESULT hr=webview_->CapturePreview(COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG,stream.Get(),
            Callback<ICoreWebView2CapturePreviewCompletedHandler>([stream,done](HRESULT hr)->HRESULT{stream->Commit(STGC_DEFAULT);done(SUCCEEDED(hr));return S_OK;}).Get());
        if(FAILED(hr))finished(false);
    }
    bool MenuKeys(int primary, int secondary = 0) {
        UnregisterHotKey(hwnd_, 91); UnregisterHotKey(hwnd_, 92);
        if (info_.testMode) return true;
        const bool first = !primary || RegisterHotKey(hwnd_, 91, MOD_NOREPEAT, primary);
        const bool second = !secondary || RegisterHotKey(hwnd_, 92, MOD_NOREPEAT, secondary);
        return first && second;
    }

private:
    std::mutex postMutex_;
    bool closing_ = false;
    std::vector<std::future<void>> workers_;
    HWND hwnd_ = nullptr;
    AppInfo info_;
    UiCallbacks cb_;
    std::wstring uiDir_;
    ComPtr<ICoreWebView2Controller> controller_;
    ComPtr<ICoreWebView2> webview_;
    bool island_ = false;
    bool alwaysOnTop_ = false;
    bool notice_=false,noticeWasIsland_=false,quitLayout_=false,closeApproved_=false;
    bool shaped_ = false;
    RECT panelRect_{};
    static constexpr UINT_PTR kShapeFallbackTimer = 1;

    // ---------------------------------------------------------------- glass
    // A page can only blur its own content, not the desktop/game behind the window, and the old DWM
    // blur APIs always blur the whole window rectangle. So the blur is built with Windows' compositor:
    // under the page sits one frosted visual per glass shape (loader pill, auth card, panel, island…),
    // each using the host-backdrop brush (a live blur of what's behind the window) and clipped to that
    // shape's rounded rectangle with smooth corners. The window region follows the same shapes, so
    // everything outside them is gone and clicks pass through to whatever is behind.
    winrt::Windows::System::DispatcherQueueController dispatcher_{ nullptr };
    winrt::Windows::UI::Composition::Compositor compositor_{ nullptr };
    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget target_{ nullptr };
    winrt::Windows::UI::Composition::ContainerVisual glassRoot_{ nullptr };
    winrt::Windows::UI::Composition::CompositionBrush backdrop_{ nullptr };
    std::vector<winrt::Windows::UI::Composition::SpriteVisual> panes_;
    bool frosted_ = false;

    bool CreateBackdrop() {
        namespace wuc = winrt::Windows::UI::Composition;
        try {
            // "Transparency effects" off in Windows settings → there's nothing to blur with.
            if (!winrt::Windows::UI::ViewManagement::UISettings().AdvancedEffectsEnabled()) return false;
            if (!winrt::Windows::System::DispatcherQueue::GetForCurrentThread()) {
                DispatcherQueueOptions opts{ sizeof(opts), DQTYPE_THREAD_CURRENT, DQTAT_COM_STA };
                winrt::check_hresult(CreateDispatcherQueueController(opts,
                    reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(dispatcher_))));
            }
            compositor_ = wuc::Compositor();
            BOOL use = TRUE;
            DwmSetWindowAttribute(hwnd_, 17 /* DWMWA_USE_HOSTBACKDROPBRUSH */, &use, sizeof(use));
            auto interop = compositor_.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();
            winrt::check_hresult(interop->CreateDesktopWindowTarget(hwnd_, FALSE,  // FALSE = beneath the page
                reinterpret_cast<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget**>(winrt::put_abi(target_))));
            glassRoot_ = compositor_.CreateContainerVisual();
            target_.Root(glassRoot_);
            backdrop_ = compositor_.CreateHostBackdropBrush();
            return true;
        } catch (...) {
            target_ = nullptr; compositor_ = nullptr;
            return false;
        }
    }

    // rects: [[x, y, w, h, radius], ...] in CSS pixels; dpr converts to device pixels.
    void ApplyShape(const json::Value& msg) {
        namespace wuc = winrt::Windows::UI::Composition;
        const double dpr = msg[L"dpr"].num(1.0);
        const auto& rects = msg[L"rects"].a;
        if(!std::isfinite(dpr)||dpr<.1||dpr>8||rects.size()>32)return;
        // 0..1 from the page's Frost slider (Desktop backdrop); 0 when the page draws its own scene
        const float frost = float(std::clamp(msg[L"frost"].num(1.0), 0.0, 1.0));
        HRGN all = CreateRectRgn(0, 0, 0, 0);
        size_t used = 0;
        for (const json::Value& rect : rects) {
            if (rect.a.size() < 5) continue;
            bool valid=true;for(size_t i=0;i<5;++i)if(rect.a[i].type!=json::Value::Number||!std::isfinite(rect.a[i].n)||std::abs(rect.a[i].n)>32768)valid=false;
            if(!valid||rect.a[2].n<0||rect.a[3].n<0||rect.a[4].n<0)continue;
            const float x = float(rect.a[0].num() * dpr), y = float(rect.a[1].num() * dpr);
            const float w = float(rect.a[2].num() * dpr), h = float(rect.a[3].num() * dpr), r = float(rect.a[4].num() * dpr);

            // window region: 2px larger than the shape so its hard edge never cuts the smooth corners
            const int d = int(r * 2 + 4.5);
            HRGN one = CreateRoundRectRgn(int(x) - 2, int(y) - 2, int(x + w + .99f) + 3, int(y + h + .99f) + 3, d, d);
            CombineRgn(all, all, one, RGN_OR);
            DeleteObject(one);

            if (!frosted_ || frost <= 0) continue;
            if (used == panes_.size()) {
                wuc::SpriteVisual pane = compositor_.CreateSpriteVisual();
                pane.Brush(backdrop_);
                auto geometry = compositor_.CreateRoundedRectangleGeometry();
                pane.Clip(compositor_.CreateGeometricClip(geometry));
                glassRoot_.Children().InsertAtTop(pane);
                panes_.push_back(pane);
            }
            wuc::SpriteVisual& pane = panes_[used++];
            pane.IsVisible(true);
            pane.Opacity(frost);  // below 1 the sharp desktop shows through: clearer glass
            pane.Offset({ x, y, 0 });
            pane.Size({ w, h });
            auto geometry = pane.Clip().as<wuc::CompositionGeometricClip>().Geometry().as<wuc::CompositionRoundedRectangleGeometry>();
            geometry.Size({ w, h });
            geometry.CornerRadius({ r, r });
        }
        for (size_t i = used; i < panes_.size(); ++i) panes_[i].IsVisible(false);
        SetWindowRgn(hwnd_, all, TRUE);  // the window owns the region from here on
        shaped_ = true;
    }

    // ---------------------------------------------------------------- setup
    static std::wstring LocalAppData() {
        PWSTR path = nullptr; std::wstring out;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) out = path;
        CoTaskMemFree(path);
        return out;
    }
    // The page is served from https://vanta.app/ (a virtual host), so it gets a real origin for storage.
    // Dev override: a "ui\index.html" next to the exe is used directly, so you can edit without rebuilding.
    bool PrepareUiFolder() {
        wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
        std::wstring dir = exe; dir = dir.substr(0, dir.find_last_of(L"\\/"));
        if (info_.debug && GetFileAttributesW((dir + L"\\ui\\index.html").c_str()) != INVALID_FILE_ATTRIBUTES) { uiDir_ = dir + L"\\ui"; return true; }

        HMODULE mod = GetModuleHandleW(nullptr);
        HRSRC res = FindResourceW(mod, MAKEINTRESOURCEW(VANTA_UI_RESOURCE), RT_RCDATA);
        if (!res) return Fail(L"The embedded UI is missing from this build."), false;
        HGLOBAL data = LoadResource(mod, res);
        const void* bytes = LockResource(data);
        const DWORD size = SizeofResource(mod, res);

        uiDir_ = LocalAppData() + L"\\" + info_.dataName + L"\\ui";
        SHCreateDirectoryExW(nullptr, uiDir_.c_str(), nullptr);
        HANDLE f = CreateFileW((uiDir_ + L"\\index.html").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f == INVALID_HANDLE_VALUE) return Fail(L"Could not write the UI to %LOCALAPPDATA%\\VantaUI."), false;
        DWORD written = 0; WriteFile(f, bytes, size, &written, nullptr); CloseHandle(f);
        if(written != size)return false;
        SHCreateDirectoryExW(nullptr,(uiDir_+L"\\fonts").c_str(),nullptr);
        const wchar_t* fonts[]={L"Inter-400.ttf",L"Inter-500.ttf",L"Inter-600.ttf",L"Inter-700.ttf",L"OFL.txt"};
        for(int i=0;i<5;++i){
            auto r=FindResourceW(mod,MAKEINTRESOURCEW(102+i),RT_RCDATA);if(!r)return false;
            auto d=LoadResource(mod,r);const DWORD length=SizeofResource(mod,r);
            HANDLE font=CreateFileW((uiDir_+L"\\fonts\\"+fonts[i]).c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(font==INVALID_HANDLE_VALUE)return false;DWORD count=0;
            const bool ok=WriteFile(font,LockResource(d),length,&count,nullptr)&&count==length;CloseHandle(font);if(!ok)return false;
        }
        return true;
    }

    HRESULT OnControllerReady(ICoreWebView2Controller* controller) {
        controller_ = controller;
        controller_->get_CoreWebView2(&webview_);

        ComPtr<ICoreWebView2Controller2> c2;
        if (SUCCEEDED(controller_.As(&c2))) c2->put_DefaultBackgroundColor(COREWEBVIEW2_COLOR{ 0, 0, 0, 0 });  // see-through page

        ComPtr<ICoreWebView2Settings> s;
        webview_->get_Settings(&s);
        s->put_AreDefaultContextMenusEnabled(info_.debug);
        s->put_AreDevToolsEnabled(info_.debug);
        s->put_IsStatusBarEnabled(FALSE);
        s->put_IsZoomControlEnabled(FALSE);
        ComPtr<ICoreWebView2Settings3> s3;
        if (SUCCEEDED(s.As(&s3))) s3->put_AreBrowserAcceleratorKeysEnabled(info_.debug);  // no Ctrl+R / F5 / Ctrl+P

        // Hand the page its host info before any of its scripts run (also switches it into "app" layout).
        std::wstring boot =
            L"window.__VANTA_HOST__={version:" + json::quote(info_.version) + L",latestVersion:" + json::quote(info_.latestVersion) +
            L",robloxVersion:" + json::quote(info_.robloxVersion) + L",glass:" + (frosted_ ? L"true" : L"false") +
            L",product:" + json::quote(info_.product) + L",user:" + json::quote(info_.user) + L",gameVersion:" + json::quote(info_.gameVersion) +
            L",basketball:" + info_.initialState + L",testMode:" + (info_.testMode ? L"true" : L"false") +
            L",skipAuth:" + (info_.skipAuth ? L"true" : L"false") + L"};";
        webview_->AddScriptToExecuteOnDocumentCreated(boot.c_str(), nullptr);

        EventRegistrationToken token{};
        webview_->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                LPWSTR raw = nullptr;
                LPWSTR source = nullptr;
                args->get_Source(&source);
                const bool local = source && std::wstring(source) == L"https://vanta.app/index.html";
                CoTaskMemFree(source);
                if (!local) return S_OK;
                if (SUCCEEDED(args->get_WebMessageAsJson(&raw)) && raw) { OnMessage(json::parse(raw)); CoTaskMemFree(raw); }
                return S_OK;
            }).Get(), &token);
        webview_->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
            [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                LPWSTR uri = nullptr; args->get_Uri(&uri);
                if (!uri || std::wstring(uri) != L"https://vanta.app/index.html") args->put_Cancel(TRUE);
                CoTaskMemFree(uri); return S_OK;
            }).Get(), &token);
        webview_->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT { args->put_Handled(TRUE); return S_OK; }).Get(), &token);

        ComPtr<ICoreWebView2_3> w3;
        if (FAILED(webview_.As(&w3))) return Fail(L"This WebView2 Runtime is too old. Update Microsoft Edge WebView2.");
        w3->SetVirtualHostNameToFolderMapping(L"vanta.app", uiDir_.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);

        controller_->put_ZoomFactor(scale_);
        Resize();
        webview_->Navigate(L"https://vanta.app/index.html");
        controller_->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        return S_OK;
    }

    HRESULT Fail(const wchar_t* message) {
        MessageBoxW(hwnd_, message, info_.title.c_str(), MB_ICONERROR);
        if (hwnd_) DestroyWindow(hwnd_);
        return E_FAIL;
    }

    void Resize() {
        if (!controller_) return;
        RECT rc; GetClientRect(hwnd_, &rc);
        controller_->put_Bounds(rc);
    }

    // ---------------------------------------------------------------- page → C++
    void Reply(const json::Value& msg, const std::wstring& fields = L"") {
        const double rid = msg[L"rid"].num(-1);
        if (!std::isfinite(rid) || rid < 0 || rid > 9007199254740991.0) return;
        Post(L"{\"type\":\"reply\",\"rid\":" + std::to_wstring(static_cast<long long>(rid)) + (fields.empty() ? L"" : L"," + fields) + L"}");
    }
    template <class Fn>
    void Async(json::Value msg, Fn work) {  // run on a worker, reply when done
        workers_.erase(std::remove_if(workers_.begin(), workers_.end(), [](auto& f) {
            return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }), workers_.end());
        if (workers_.size() >= 8) { Reply(msg, L"\"ok\":false,\"error\":\"Please retry shortly.\""); return; }
        workers_.push_back(std::async(std::launch::async, [this, msg = std::move(msg), work]() mutable {
            try { Reply(msg, work(msg)); } catch (...) { Reply(msg, L"\"ok\":false,\"error\":\"Request failed.\""); }
        }));
    }
    static std::wstring AuthFields(const AuthResult& r) {
        std::wstring f = std::wstring(L"\"ok\":") + (r.ok ? L"true" : L"false");
        if (!r.error.empty()) f += L",\"error\":" + json::quote(r.error);
        if (!r.field.empty()) f += L",\"field\":" + json::quote(r.field);
        return f;
    }

    void OnMessage(const json::Value& msg) {
        const std::wstring type = msg[L"type"].str();
        if (type == L"shape") {
            ApplyShape(msg);
        } else if (type == L"drag") {
            ReleaseCapture();
            SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        } else if (type == L"close") {
            PostMessageW(hwnd_, WM_CLOSE, 0, 0);
        } else if(type==L"closeConfirmed"){
            Close();
        } else if(type==L"quitLayout"){
            quitLayout_=msg[L"open"].boolean();if(island_){MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(hwnd_,MONITOR_DEFAULTTONEAREST),&mi);PlaceIsland(mi);}Reply(msg,L"\"ok\":true");
        } else if(type==L"connectionNotice"){
            if(cb_.custom)cb_.custom(type,msg);
            EnterNotice();Reply(msg,L"\"ok\":true");
        } else if(type==L"connectionNoticeEnd"){
            LeaveNotice();Reply(msg,L"\"ok\":true");
        } else if(type==L"testWindow"&&info_.testMode){
            RECT r{};GetWindowRect(hwnd_,&r);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(hwnd_,MONITOR_DEFAULTTONEAREST),&mi);
            Reply(msg,L"\"ok\":true,\"width\":"+std::to_wstring(r.right-r.left)+L",\"height\":"+std::to_wstring(r.bottom-r.top)+L",\"top\":"+std::to_wstring(r.top)+L",\"workTop\":"+std::to_wstring(mi.rcWork.top)+L",\"notice\":"+(notice_?L"true":L"false")+L",\"island\":"+(island_?L"true":L"false")+L",\"topmost\":"+((GetWindowLongPtrW(hwnd_,GWL_EXSTYLE)&WS_EX_TOPMOST)?L"true":L"false"));
        } else if(type==L"testTopmost"&&info_.testMode){
            Reply(msg,std::wstring(L"\"desired\":")+(alwaysOnTop_?L"true":L"false")+L",\"style\":"+std::to_wstring(GetWindowLongPtrW(hwnd_,GWL_EXSTYLE)));
        } else if(type==L"testScale"&&info_.testMode){
            BeginScale(std::clamp(msg[L"scale"].num(1),.6,1.5)*dpi_/96.0);Reply(msg,L"\"ok\":true");
        } else if (type == L"minimize") {
            EnterIsland(); Reply(msg);
        } else if (type == L"restore") {
            LeaveIsland(); Reply(msg);
        } else if (type == L"signin") {
            // Basketball's existing local 1/1 check has no I/O. Keep its state changes
            // ordered with logout on the UI thread; provider auth can use Async.
            if (info_.product == L"basketball") {
                const auto r = cb_.signIn ? cb_.signIn(msg[L"user"].str(), msg[L"pass"].str(), false) : AuthResult{};
                Reply(msg, AuthFields(r)); return;
            }
            Async(msg, [this](const json::Value& m) {
                AuthResult r{ false };
                if (cb_.signIn) r = cb_.signIn(m[L"user"].str(), m[L"pass"].str(), m[L"remember"].boolean());
                return AuthFields(r);
            });
        } else if (type == L"signup") {
            Async(msg, [this](const json::Value& m) {
                AuthResult r{ false };
                if (cb_.createAccount) r = cb_.createAccount(m[L"email"].str(), m[L"user"].str(), m[L"pass"].str(), m[L"key"].str());
                return AuthFields(r);
            });
        } else if (type == L"check") {
            Async(msg, [this](const json::Value& m) {
                CheckResult r{ true };
                if (cb_.runCheck) r = cb_.runCheck(m[L"check"].str());
                std::wstring f = std::wstring(L"\"ok\":") + (r.ok ? L"true" : L"false");
                if (!r.result.empty()) f += L",\"result\":" + json::quote(r.result);
                if (!r.detail.empty()) f += L",\"detail\":" + json::quote(r.detail);
                return f;
            });
        } else if (type == L"update") {
            Async(msg, [this](const json::Value&) {
                const bool ok = cb_.installUpdate ? cb_.installUpdate() : false;
                return std::wstring(L"\"ok\":") + (ok ? L"true" : L"false");
            });
        } else if (type == L"feature") {
            if (cb_.featureChanged && msg[L"on"].type == json::Value::Bool) cb_.featureChanged(msg[L"feature"].str(), msg[L"on"].boolean());
        } else if (type == L"setting") {
            if(info_.product==L"basketball"&&msg[L"key"].str()==L"theme.topmost"){
                if(msg[L"value"].type!=json::Value::Bool)return;
                alwaysOnTop_=msg[L"value"].boolean();
                SetWindowPos(hwnd_,(alwaysOnTop_||island_||notice_)?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
                return;
            }
            if (cb_.settingChanged) cb_.settingChanged(msg[L"key"].str(), msg[L"value"]);
        } else if (type == L"snapshot") {
            if (cb_.modulesChanged) cb_.modulesChanged(msg[L"data"]);
        } else if (type == L"workspace") {
            const bool ok=cb_.workspaceOpened&&cb_.workspaceOpened();
            Reply(msg,ok?L"\"ok\":true":L"\"ok\":false,\"error\":\"The required game connection is not ready.\"");
        } else if (type == L"signout") {
            if (cb_.signedOut) cb_.signedOut();
        } else if (cb_.custom) {
            cb_.custom(type, msg);
        }
    }

    // ---------------------------------------------------------------- size per monitor
    // The UI is designed for a 2560×1440 desktop at 100% Windows scaling. On other monitors it is scaled
    // so it takes the same share of the screen: 1920×1080 → 75%, 3840×2160 at 150% → 100%, etc.
    // (Windows' own display scaling is applied first, then this.) WebView2's zoom factor scales the page
    // and the window is resized to match; it's re-checked whenever the window moves to another monitor.
    static constexpr double kRefW = 2560, kRefH = 1440;
    double scale_ = 1.0;
    UINT dpi_ = 96;
    bool dragging_=false,scaling_=false,applyingScale_=false;
    double scaleFrom_=1,scaleTarget_=1,scaleStarted_=0,anchorX_=.5,anchorY_=.04,scaleCenterX_=0,scaleCenterY_=0;
    static double Seconds(){return GetTickCount64()*.001;}
    static UINT MonitorDpi(HMONITOR mon,UINT fallback){UINT x=fallback,y=fallback;return SUCCEEDED(GetDpiForMonitor(mon,MDT_EFFECTIVE_DPI,&x,&y))?x:fallback;}
    int Px(int dip) const { return static_cast<int>(dip * dpi_ / 96.0 * scale_ + .5); }
    double MonitorScale(HMONITOR mon, UINT dpi) const {
        if (info_.scale > 0) return info_.scale;
        MONITORINFO mi{ sizeof(mi) };
        GetMonitorInfoW(mon, &mi);
        const double w = (mi.rcMonitor.right - mi.rcMonitor.left) * 96.0 / dpi, h = (mi.rcMonitor.bottom - mi.rcMonitor.top) * 96.0 / dpi;
        return std::clamp((std::min)(w / kRefW, h / kRefH), .6, 1.5);
    }
    void FitToMonitor(bool center) {
        HMONITOR mon = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
        const UINT dpi = GetDpiForWindow(hwnd_);
        const double s = MonitorScale(mon, dpi);
        if(!center){BeginScale(s*dpi/96.0);return;}
        const bool changed = std::abs(s - scale_) > .001 || dpi != dpi_;
        scale_ = s; dpi_ = dpi;
        if (controller_) controller_->put_ZoomFactor(scale_);
        if (!changed && !center) return;
        MONITORINFO mi{ sizeof(mi) };
        GetMonitorInfoW(mon, &mi);
        if (island_) { PlaceIsland(mi); return; }
        const int w = Px(kPanelW + 2 * kMargin), h = Px(kPanelH + 2 * kMargin);
        RECT r{}; GetWindowRect(hwnd_, &r);
        int cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2;
        if (center) { cx = (mi.rcWork.left + mi.rcWork.right) / 2; cy = (mi.rcWork.top + mi.rcWork.bottom) / 2; }
        const int x = std::clamp<int>(cx - w / 2, mi.rcWork.left, (std::max)(static_cast<int>(mi.rcWork.left), static_cast<int>(mi.rcWork.right) - w));
        const int y = std::clamp<int>(cy - h / 2, mi.rcWork.top, (std::max)(static_cast<int>(mi.rcWork.top), static_cast<int>(mi.rcWork.bottom) - h));
        SetWindowPos(hwnd_, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    // Physical size is interpolated, including during the system's move loop.
    // Keep the cursor's grabbed point fixed rather than jumping the whole window.
    void BeginScale(double target){
        if(std::abs(target-(scaling_?scaleTarget_:scale_*dpi_/96.0))<.0005)return;
        scaleFrom_=scale_*dpi_/96.0;scaleTarget_=target;scaleStarted_=Seconds();scaling_=true;
        RECT r{};GetWindowRect(hwnd_,&r);scaleCenterX_=(r.left+r.right)*.5;scaleCenterY_=(r.top+r.bottom)*.5;
        SetTimer(hwnd_,75,16,nullptr);
    }
    void TrackMonitor(POINT cursor){const auto mon=MonitorFromPoint(cursor,MONITOR_DEFAULTTONEAREST);const UINT dpi=MonitorDpi(mon,dpi_);BeginScale(MonitorScale(mon,dpi)*dpi/96.0);}
    void ScaleFrame(){
        if(!scaling_)return;
        const double t=std::clamp((Seconds()-scaleStarted_)/.28,0.0,1.0),ease=1-std::pow(1-t,3);
        const double physical=scaleFrom_+(scaleTarget_-scaleFrom_)*ease;scale_=physical*96.0/dpi_;
        if(controller_)controller_->put_ZoomFactor(scale_);
        applyingScale_=true;
        if(island_){MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(hwnd_,MONITOR_DEFAULTTONEAREST),&mi);PlaceIsland(mi);}
        else {
            RECT r{};GetWindowRect(hwnd_,&r);const int w=Px(kPanelW+2*kMargin),h=Px(kPanelH+2*kMargin);
            int x=int(scaleCenterX_-w*.5),y=int(scaleCenterY_-h*.5);
            if(dragging_){POINT p{};GetCursorPos(&p);x=p.x-int(anchorX_*w);y=p.y-int(anchorY_*h);}
            SetWindowPos(hwnd_,nullptr,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
        }
        applyingScale_=false;
        if(t>=1){scaling_=false;KillTimer(hwnd_,75);}
    }

    // ---------------------------------------------------------------- island (minimised) mode
    // The page plays its own shrink animation; the host then turns the window into a small,
    // always-on-top strip at the top-centre of the current monitor, and back again on restore.
    void PlaceIsland(const MONITORINFO& mi) {
        const int w = Px((notice_||quitLayout_)?520:kIslandW), h = Px((notice_||quitLayout_)?238:kIslandH);
        const int x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - w) / 2;
        SetWindowPos(hwnd_, HWND_TOPMOST, x, mi.rcWork.top, w, h, SWP_NOACTIVATE);
    }
    void EnterIsland() {
        if(notice_)return;
        if (island_) return;
        GetWindowRect(hwnd_, &panelRect_);
        MONITORINFO mi{ sizeof(mi) };
        GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &mi);
        island_ = true;
        PlaceIsland(mi);
    }
    void LeaveIsland() {
        if(notice_)return;
        if (!island_) return;
        SetWindowPos(hwnd_, alwaysOnTop_?HWND_TOPMOST:HWND_NOTOPMOST, panelRect_.left, panelRect_.top,
                     panelRect_.right - panelRect_.left, panelRect_.bottom - panelRect_.top, 0);
        island_ = false;
        SetForegroundWindow(hwnd_);
    }
    void EnterNotice(){
        if(notice_)return;noticeWasIsland_=island_;
        if(!island_)GetWindowRect(hwnd_,&panelRect_);
        notice_=true;island_=true;MONITORINFO mi{sizeof(mi)};
        GetMonitorInfoW(MonitorFromWindow(hwnd_,MONITOR_DEFAULTTONEAREST),&mi);PlaceIsland(mi);
    }
    void LeaveNotice(){
        if(!notice_)return;notice_=false;
        if(noticeWasIsland_){MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(hwnd_,MONITOR_DEFAULTTONEAREST),&mi);PlaceIsland(mi);}
        else{island_=false;SetWindowPos(hwnd_,alwaysOnTop_?HWND_TOPMOST:HWND_NOTOPMOST,panelRect_.left,panelRect_.top,panelRect_.right-panelRect_.left,panelRect_.bottom-panelRect_.top,SWP_NOACTIVATE);SetForegroundWindow(hwnd_);}
    }

    // ---------------------------------------------------------------- window procedure
    static LRESULT CALLBACK StaticProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_NCCREATE) {
            auto* self = static_cast<UiHost*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->hwnd_ = hwnd;
        }
        auto* self = reinterpret_cast<UiHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        return self ? self->Proc(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
    }

    LRESULT Proc(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_WINDOWPOSCHANGING: {
                // WebView/window initialization can reorder the host after the
                // preference is applied. Preserve the requested band during
                // native positioning without activating or moving the window.
                if(alwaysOnTop_){auto* pos=reinterpret_cast<WINDOWPOS*>(lp);pos->hwndInsertAfter=HWND_TOPMOST;pos->flags&=~SWP_NOZORDER;}
                break;
            }
            case WM_APP_POST: {
                std::unique_ptr<std::wstring> payload(reinterpret_cast<std::wstring*>(lp));
                if (webview_) webview_->PostWebMessageAsJson(payload->c_str());
                return 0;
            }
            case WM_SIZE:
                Resize();
                return 0;
            case WM_HOTKEY:
                if (wp == 91 || wp == 92) Post(L"{\"type\":\"menuToggle\"}");
                return 0;
            case WM_TIMER:
                if(wp==75){ScaleFrame();return 0;}
                if(wp==73){KillTimer(hwnd_,73);if(cb_.custom)cb_.custom(L"testTimeout",{});return 0;}
                if(wp==74){if(GetAsyncKeyState(VK_F8)&0x8000)DestroyWindow(hwnd_);return 0;}
                if (wp == kShapeFallbackTimer) {
                    KillTimer(hwnd_, kShapeFallbackTimer);
                    if (!shaped_) SetWindowRgn(hwnd_, nullptr, TRUE);
                    return 0;
                }
                break;
            case WM_DPICHANGED: {
                const double physical=scale_*dpi_/96.0;dpi_=HIWORD(wp);scale_=physical*96.0/dpi_;
                if(controller_)controller_->put_ZoomFactor(scale_);
                if(!applyingScale_){POINT cursor{};GetCursorPos(&cursor);if(dragging_)TrackMonitor(cursor);else FitToMonitor(false);}
                return 0;
            }
            case WM_ENTERSIZEMOVE:{
                dragging_=true;RECT r{};POINT p{};GetWindowRect(hwnd_,&r);GetCursorPos(&p);
                anchorX_=double(p.x-r.left)/(std::max)(1L,r.right-r.left);anchorY_=double(p.y-r.top)/(std::max)(1L,r.bottom-r.top);return 0;
            }
            case WM_MOVING:{
                POINT p{};GetCursorPos(&p);TrackMonitor(p);auto* r=reinterpret_cast<RECT*>(lp);
                const int w=Px(kPanelW+2*kMargin),h=Px(kPanelH+2*kMargin);
                r->left=p.x-int(anchorX_*w);r->top=p.y-int(anchorY_*h);r->right=r->left+w;r->bottom=r->top+h;return TRUE;
            }
            case WM_EXITSIZEMOVE:      // dropped on another monitor (same DPI, different size)
                {RECT r{};GetWindowRect(hwnd_,&r);scaleCenterX_=(r.left+r.right)*.5;scaleCenterY_=(r.top+r.bottom)*.5;dragging_=false;FitToMonitor(false);}break;
            case WM_DISPLAYCHANGE:     // resolution or monitor layout changed
                FitToMonitor(false);
                break;
            case WM_CLOSE:
                if(!closeApproved_&&webview_){Post(L"{\"type\":\"closeRequested\"}");return 0;}
                DestroyWindow(hwnd_);return 0;
            case WM_ACTIVATE:
                if (LOWORD(wp) != WA_INACTIVE && controller_) controller_->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
                return 0;
            case WM_DESTROY:
                UnregisterHotKey(hwnd_, 91); UnregisterHotKey(hwnd_, 92);
                { std::lock_guard<std::mutex> lock(postMutex_);
                  closing_ = true;
                  MSG queued{};
                  while (PeekMessageW(&queued, hwnd_, WM_APP_POST, WM_APP_POST, PM_REMOVE))
                      delete reinterpret_cast<std::wstring*>(queued.lParam);
                  hwnd_ = nullptr;
                }
                if (controller_) controller_->Close();
                controller_.Reset();
                webview_.Reset();
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(hwnd_, msg, wp, lp);
    }
};

}  // namespace vanta
