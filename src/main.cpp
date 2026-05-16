#include "skylanders/CharacterCatalog.h"
#include "skylanders/EmbeddedCemuHost.h"
#include "skylanders/EmulatorCore.h"
#include "skylanders/CemuLocator.h"
#include "skylanders/FigureStore.h"
#include "skylanders/GameCatalog.h"
#include "skylanders/GraphicPackManager.h"
#include "skylanders/InputBridge.h"
#include "skylanders/InputRouter.h"
#include "skylanders/NativeOverlayRenderer.h"
#include "skylanders/PortalBridge.h"
#include "skylanders/ProcessManager.h"
#include "skylanders/RomLocator.h"
#include "skylanders/RuntimeModes.h"

#include <algorithm>
#include <array>
#include <cwctype>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#include <xinput.h>

#if SKYLANDERS_WITH_ULTRALIGHT
#include <AppCore/App.h>
#include <AppCore/JSHelpers.h>
#include <AppCore/Overlay.h>
#include <AppCore/Window.h>
#include <Ultralight/Listener.h>
#include <Ultralight/StringSTL.h>
#include <Ultralight/Ultralight.h>

using namespace ultralight;
#endif

namespace {

constexpr UINT kToggleOverlayMessage = WM_APP + 0x534;
constexpr UINT kRealignOverlayMessage = WM_APP + 0x535;
constexpr UINT kCemuExitedMessage = WM_APP + 0x536;
constexpr UINT kGraphicPacksUpdatedMessage = WM_APP + 0x537;

std::filesystem::path executablePath() {
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    buffer.resize(size);
    return buffer;
}

std::string narrow(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string out(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), length, nullptr, nullptr);
    return out;
}

std::string jsonEscape(std::string value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

std::string jsonUnescape(std::string value) {
    std::string out;
    out.reserve(value.size());
    bool escaping = false;
    for (const char ch : value) {
        if (escaping) {
            switch (ch) {
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default: out.push_back(ch); break;
            }
            escaping = false;
        } else if (ch == '\\') {
            escaping = true;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::optional<std::string> regexStringField(const std::string& json, const char* name) {
    std::smatch match;
    const std::regex pattern(std::string("\"") + name + R"json("\s*:\s*"((?:\\.|[^"])*)")json");
    if (!std::regex_search(json, match, pattern)) {
        return std::nullopt;
    }
    return jsonUnescape(match[1].str());
}

std::filesystem::path findProjectRoot(const std::filesystem::path& exePath) {
    auto path = exePath.parent_path();
    for (int i = 0; i < 6 && !path.empty(); ++i) {
        std::error_code ec;
        if (std::filesystem::exists(path / "CMakeLists.txt", ec) &&
            std::filesystem::exists(path / "assets", ec)) {
            return path;
        }
        path = path.parent_path();
    }
    return exePath.parent_path();
}

skylanders::RomLocatorConfig makeRomLocatorConfig(const std::filesystem::path& projectRoot) {
    return {
        projectRoot / "roms",
        projectRoot / "runtime" / "config" / "game_paths.json",
    };
}

skylanders::CemuLocatorConfig makeCemuLocatorConfig(const std::filesystem::path& projectRoot, const std::filesystem::path& exePath) {
    return {
        projectRoot,
        exePath,
        projectRoot / "runtime" / "config" / "setup.json",
    };
}

skylanders::GraphicPackManagerConfig makeGraphicPackManagerConfig(
    const std::filesystem::path& projectRoot,
    const std::filesystem::path& cemuRoot) {
    return {
        cemuRoot,
        projectRoot / "runtime" / "config" / "graphic_packs.json",
        projectRoot / "runtime" / "downloads" / "graphic_packs",
    };
}

void applyResolvedImage(skylanders::GameLaunchSpec& spec, const skylanders::ResolvedGameImage& image) {
    spec.gamePath = image.path;
    spec.titleId = image.titleId;
    spec.region = image.region;
    spec.imageSource = image.source;
}

struct EnumWindowByPidContext {
    unsigned long processId = 0;
    HWND window = nullptr;
};

BOOL CALLBACK enumWindowByPid(HWND hwnd, LPARAM lparam) {
    auto* context = reinterpret_cast<EnumWindowByPidContext*>(lparam);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != context->processId || !IsWindowVisible(hwnd)) {
        return TRUE;
    }

    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if ((style & WS_DISABLED) != 0) {
        return TRUE;
    }

    RECT rect{};
    if (!GetWindowRect(hwnd, &rect) || rect.right <= rect.left || rect.bottom <= rect.top) {
        return TRUE;
    }

    context->window = hwnd;
    return FALSE;
}

HWND findMainWindowForProcess(unsigned long processId) {
    EnumWindowByPidContext context{processId, nullptr};
    EnumWindows(enumWindowByPid, reinterpret_cast<LPARAM>(&context));
    return context.window;
}

HWND waitForMainWindow(unsigned long processId) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (HWND hwnd = findMainWindowForProcess(processId)) {
            return hwnd;
        }
        Sleep(100);
    }
    return nullptr;
}

#if SKYLANDERS_WITH_ULTRALIGHT
class OverlayController {
public:
    void initialize(HWND overlayWindow, ultralight::Overlay* overlay) {
        overlayWindow_ = overlayWindow;
        overlay_ = overlay;
        visible_ = false;

        if (!overlayWindow_) {
            return;
        }

        LONG_PTR exStyle = GetWindowLongPtrW(overlayWindow_, GWL_EXSTYLE);
        exStyle |= WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
        exStyle &= ~WS_EX_APPWINDOW;
        SetWindowLongPtrW(overlayWindow_, GWL_EXSTYLE, exStyle);
        SetWindowPos(overlayWindow_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        ShowWindow(overlayWindow_, SW_HIDE);
    }

    bool attachToProcess(unsigned long processId) {
        cemuWindow_ = waitForMainWindow(processId);
        if (!cemuWindow_) {
            return false;
        }
        align();
        return true;
    }

    void toggle() {
        if (visible_) {
            hide();
        } else {
            show();
        }
    }

    void show() {
        if (!overlayWindow_) {
            return;
        }
        align();
        ShowWindow(overlayWindow_, SW_SHOW);
        SetWindowPos(overlayWindow_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetForegroundWindow(overlayWindow_);
        visible_ = true;
    }

    void hide() {
        if (!overlayWindow_) {
            return;
        }
        ShowWindow(overlayWindow_, SW_HIDE);
        if (cemuWindow_) {
            SetForegroundWindow(cemuWindow_);
        }
        visible_ = false;
    }

    void align() {
        if (!overlayWindow_ || !cemuWindow_) {
            return;
        }

        RECT rect{};
        if (!GetWindowRect(cemuWindow_, &rect)) {
            return;
        }

        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        if (width <= 0 || height <= 0) {
            return;
        }

        SetWindowPos(
            overlayWindow_,
            HWND_TOPMOST,
            rect.left,
            rect.top,
            width,
            height,
            SWP_NOACTIVATE | (visible_ ? SWP_SHOWWINDOW : 0));

        if (overlay_) {
            overlay_->Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }
    }

    bool visible() const noexcept {
        return visible_;
    }

    HWND overlayWindow() const noexcept {
        return overlayWindow_;
    }

private:
    HWND overlayWindow_ = nullptr;
    HWND cemuWindow_ = nullptr;
    ultralight::Overlay* overlay_ = nullptr;
    bool visible_ = false;
};

class OverlayToggleInput {
public:
    ~OverlayToggleInput() {
        stop();
    }

    void start(HWND targetWindow) {
        targetWindow_ = targetWindow;
        if (!targetWindow_ || running_.exchange(true)) {
            return;
        }
        loadXInput();
        thread_ = std::thread(&OverlayToggleInput::pollLoop, this);
    }

    struct Config {
        struct CemuBinding {
            std::string api;
            std::string uuid;
            std::string displayName;
            unsigned long long sourceInputId = 0;
        };

        std::vector<int> keyboardVks;
        std::vector<WORD> xinputMasks;
        std::vector<CemuBinding> cemuBindings;
    };

    void setConfig(Config config) {
        {
            std::lock_guard lock(configMutex_);
            config_ = std::move(config);
        }
        suppressUntilReleased();
    }

    void suppressUntilReleased() {
        suppressUntilReleased_ = true;
        suppressUntilMs_ = monotonicMilliseconds() + 5000;
    }

    void setCemuInputBridge(const skylanders::InputBridge* bridge, unsigned long processId) {
        inputBridge_ = bridge;
        cemuProcessId_ = processId;
    }

    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        if (thread_.joinable()) {
            thread_.join();
        }
        if (xinputModule_) {
            FreeLibrary(xinputModule_);
            xinputModule_ = nullptr;
            xinputGetState_ = nullptr;
        }
    }

private:
    using XInputGetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);

    static long long monotonicMilliseconds() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void loadXInput() {
        for (const wchar_t* dll : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"}) {
            xinputModule_ = LoadLibraryW(dll);
            if (!xinputModule_) {
                continue;
            }
            xinputGetState_ = reinterpret_cast<XInputGetStateFn>(GetProcAddress(xinputModule_, "XInputGetState"));
            if (xinputGetState_) {
                return;
            }
            FreeLibrary(xinputModule_);
            xinputModule_ = nullptr;
        }
    }

    bool isTogglePressed() {
        Config config;
        {
            std::lock_guard lock(configMutex_);
            config = config_;
        }

        bool pressed = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
        for (const int keyboardVk : config.keyboardVks) {
            if (keyboardVk > 0) {
                pressed = pressed || ((GetAsyncKeyState(keyboardVk) & 0x8000) != 0);
            }
        }
        if (xinputGetState_) {
            for (DWORD index = 0; index < XUSER_MAX_COUNT; ++index) {
                XINPUT_STATE state{};
                if (xinputGetState_(index, &state) != ERROR_SUCCESS) {
                    continue;
                }
                const WORD buttons = state.Gamepad.wButtons;
                for (const WORD mask : config.xinputMasks) {
                    if (mask != 0) {
                        pressed = pressed || ((buttons & mask) == mask);
                    }
                }
            }
        }
        if (!pressed && inputBridge_ && cemuProcessId_ != 0) {
            for (const auto& binding : config.cemuBindings) {
                const auto result = inputBridge_->readInput(
                    cemuProcessId_,
                    binding.api,
                    binding.uuid,
                    binding.displayName,
                    binding.sourceInputId);
                if (result.ok && result.json.find("\"pressed\":true") != std::string::npos) {
                    pressed = true;
                    break;
                }
            }
        }
        return pressed;
    }

    void pollLoop() {
        bool wasPressed = false;
        int realignTicks = 0;
        while (running_) {
            const long long nowMs = monotonicMilliseconds();
            if (nowMs < suppressUntilMs_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            const bool pressed = isTogglePressed();
            if (suppressUntilReleased_) {
                if (!pressed) {
                    suppressUntilReleased_ = false;
                    wasPressed = false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            if (pressed && !wasPressed && targetWindow_) {
                PostMessageW(targetWindow_, kToggleOverlayMessage, 0, 0);
            }
            wasPressed = pressed;

            if (++realignTicks >= 10) {
                realignTicks = 0;
                if (targetWindow_) {
                    PostMessageW(targetWindow_, kRealignOverlayMessage, 0, 0);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    HWND targetWindow_ = nullptr;
    HMODULE xinputModule_ = nullptr;
    XInputGetStateFn xinputGetState_ = nullptr;
    Config config_;
    const skylanders::InputBridge* inputBridge_ = nullptr;
    unsigned long cemuProcessId_ = 0;
    std::mutex configMutex_;
    std::atomic_bool suppressUntilReleased_{false};
    std::atomic_llong suppressUntilMs_{0};
    std::atomic_bool running_{false};
    std::thread thread_;
};

class SkylandersApp;
SkylandersApp* g_app = nullptr;
WNDPROC g_previousOverlayProc = nullptr;
#endif

skylanders::ProcessManagerConfig makeProcessConfig(
    const std::filesystem::path& projectRoot,
    skylanders::RomLocator& locator,
    const std::filesystem::path& cemuRoot) {
    skylanders::ProcessManagerConfig config;
    config.cemuRoot = cemuRoot;
    config.runtimeRoot = projectRoot / "runtime";
    config.figureLibraryRoot = projectRoot / "figures";
    config.keysPath = projectRoot / "keys" / "keys.txt";
    const auto setup = skylanders::SetupConfig::load(projectRoot / "runtime" / "config" / "setup.json");
    config.fullscreen = setup.fullscreen.value_or(false);

    for (const auto& entry : skylanders::gameCatalog()) {
        skylanders::GameLaunchSpec spec;
        spec.game = entry.game;
        spec.displayName = entry.displayName;
        spec.stubPath = entry.defaultStubExe;
        if (const auto image = locator.resolve(entry.game)) {
            applyResolvedImage(spec, *image);
        }
        config.games.push_back(std::move(spec));
    }

    return config;
}

#if SKYLANDERS_WITH_ULTRALIGHT
std::string jsValueToUtf8(const ultralight::JSValue& value) {
    const ultralight::String text = value.ToString();
    return ultralight::Convert(text);
}

class SkylandersApp final
    : public ultralight::WindowListener
    , public ultralight::LoadListener
    , public ultralight::ViewListener {
public:
    explicit SkylandersApp(std::filesystem::path exePath)
        : exePath_(std::move(exePath))
        , projectRoot_(findProjectRoot(exePath_))
        , locator_(makeRomLocatorConfig(projectRoot_))
        , cemuLocator_(makeCemuLocatorConfig(projectRoot_, exePath_))
        , cemuResolution_(cemuLocator_.resolve())
        , graphicPacks_(makeGraphicPackManagerConfig(projectRoot_, cemuResolution_ ? cemuResolution_->cemuRoot : std::filesystem::path{}))
        , manager_(makeProcessConfig(projectRoot_, locator_, cemuResolution_ ? cemuResolution_->cemuRoot : std::filesystem::path{})) {
        try {
            spec_ = manager_.resolveFromStub(exePath_);
        } catch (const std::exception&) {
            const auto& fallback = skylanders::gameCatalog().front();
            spec_ = {fallback.game, fallback.displayName, exePath_};
        }

        ultralight::Settings settings;
        settings.file_system_path = ultralight::String((exePath_.parent_path() / "assets").string().c_str());

        ultralight::Config config;
        config.cache_path = ultralight::String((exePath_.parent_path() / "cache").string().c_str());

        app_ = ultralight::App::Create(settings, config);
        window_ = ultralight::Window::Create(
            app_->main_monitor(),
            1280,
            760,
            false,
            ultralight::kWindowFlags_Borderless | ultralight::kWindowFlags_Resizable | ultralight::kWindowFlags_Hidden);
        window_->SetTitle("Skylanders Engine");
        window_->set_listener(this);

        overlay_ = ultralight::Overlay::Create(window_, window_->width(), window_->height(), 0, 0);
        overlayController_.initialize(static_cast<HWND>(window_->native_handle()), overlay_.get());
        installOverlayMessageHandler();
        overlay_->view()->set_load_listener(this);
        overlay_->view()->set_view_listener(this);
        overlay_->view()->LoadURL("file:///index.html");
    }

    ~SkylandersApp() {
        stopGraphicPackAutoUpdate();
        stopProcessWatcher();
        input_.stop();
        closeRunningProcessHandle();

        const HWND hwnd = overlayController_.overlayWindow();
        if (hwnd && g_previousOverlayProc) {
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_previousOverlayProc));
            g_previousOverlayProc = nullptr;
        }

        if (g_app == this) {
            g_app = nullptr;
        }
    }

    void run() {
        app_->Run();
    }

    void OnClose(ultralight::Window*) override {
        app_->Quit();
    }

    void OnResize(ultralight::Window*, uint32_t width, uint32_t height) override {
        overlay_->Resize(width, height);
    }

    void OnChangeCursor(ultralight::View*, ultralight::Cursor cursor) override {
        window_->SetCursor(cursor);
    }

    void OnWindowObjectReady(ultralight::View* caller, uint64_t, bool isMainFrame, const ultralight::String&) override {
        if (!isMainFrame) {
            return;
        }

        ultralight::RefPtr<ultralight::JSContext> context = caller->LockJSContext();
        ultralight::SetJSContext(context->ctx());
        auto global = ultralight::JSGlobalObject();
        global["launchGame"] = BindJSCallbackWithRetval(&SkylandersApp::launchGame);
        global["selectGameFile"] = BindJSCallbackWithRetval(&SkylandersApp::selectGameFile);
        global["toggleOverlay"] = BindJSCallbackWithRetval(&SkylandersApp::toggleOverlayCallback);
        global["showOverlay"] = BindJSCallbackWithRetval(&SkylandersApp::showOverlayCallback);
        global["hideOverlay"] = BindJSCallbackWithRetval(&SkylandersApp::hideOverlayCallback);
        global["isOverlayVisible"] = BindJSCallbackWithRetval(&SkylandersApp::isOverlayVisibleCallback);
        global["updateGraphicPacks"] = BindJSCallbackWithRetval(&SkylandersApp::updateGraphicPacks);
        global["getGraphicPackStatus"] = BindJSCallbackWithRetval(&SkylandersApp::getGraphicPackStatus);

        ultralight::JSObject portal;
        portal["place"] = BindJSCallbackWithRetval(&SkylandersApp::portalPlace);
        portal["placeOrCreate"] = BindJSCallbackWithRetval(&SkylandersApp::portalPlaceOrCreate);
        portal["remove"] = BindJSCallbackWithRetval(&SkylandersApp::portalRemove);
        global["portal"] = ultralight::JSValue(static_cast<JSObjectRef>(portal));

        ultralight::JSObject config;
        config["loadButtonMappings"] = BindJSCallbackWithRetval(&SkylandersApp::loadButtonMappings);
        config["saveButtonMappings"] = BindJSCallbackWithRetval(&SkylandersApp::saveButtonMappings);
        config["loadInputMappings"] = BindJSCallbackWithRetval(&SkylandersApp::loadInputMappings);
        global["config"] = ultralight::JSValue(static_cast<JSObjectRef>(config));

        ultralight::JSObject input;
        input["ping"] = BindJSCallbackWithRetval(&SkylandersApp::inputPing);
        input["listDevices"] = BindJSCallbackWithRetval(&SkylandersApp::inputListDevices);
        input["captureNext"] = BindJSCallbackWithRetval(&SkylandersApp::inputCaptureNext);
        input["startCapture"] = BindJSCallbackWithRetval(&SkylandersApp::inputStartCapture);
        input["pollCapture"] = BindJSCallbackWithRetval(&SkylandersApp::inputPollCapture);
        input["cancelCapture"] = BindJSCallbackWithRetval(&SkylandersApp::inputCancelCapture);
        input["readXinputButtons"] = BindJSCallbackWithRetval(&SkylandersApp::inputReadXinputButtons);
        input["suppressOverlayToggle"] = BindJSCallbackWithRetval(&SkylandersApp::inputSuppressOverlayToggle);
        input["saveProfile"] = BindJSCallbackWithRetval(&SkylandersApp::inputSaveProfile);
        input["loadProfile"] = BindJSCallbackWithRetval(&SkylandersApp::inputLoadProfile);
        global["inputBridge"] = ultralight::JSValue(static_cast<JSObjectRef>(input));
    }

    void OnDOMReady(ultralight::View* caller, uint64_t, bool isMainFrame, const ultralight::String&) override {
        if (!isMainFrame) {
            return;
        }

        jsReady_ = true;
        caller->EvaluateScript(ultralight::String(("window.setNativeState(" + nativeStateJson() + ");").c_str()));
        caller->EvaluateScript("window.prewarmOverlay && window.prewarmOverlay();");
        startGame();
    }

    void toggleOverlay() {
        if (!gameRunning_) {
            return;
        }
        overlayController_.toggle();
        pushNativeState();
    }

    void realignOverlayIfVisible() {
        if (!overlayController_.visible()) {
            return;
        }
        overlayController_.align();
    }

private:
    static LRESULT CALLBACK overlayWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        if (message == kToggleOverlayMessage && g_app) {
            g_app->toggleOverlay();
            return 0;
        }

        if (message == kRealignOverlayMessage && g_app) {
            g_app->realignOverlayIfVisible();
            return 0;
        }

        if (message == kCemuExitedMessage && g_app) {
            g_app->onCemuExited();
            return 0;
        }

        if (message == kGraphicPacksUpdatedMessage && g_app) {
            g_app->onGraphicPacksUpdated();
            return 0;
        }

        if (g_previousOverlayProc) {
            return CallWindowProcW(g_previousOverlayProc, hwnd, message, wparam, lparam);
        }
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    void installOverlayMessageHandler() {
        const HWND hwnd = overlayController_.overlayWindow();
        if (!hwnd) {
            return;
        }

        g_app = this;
        g_previousOverlayProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&SkylandersApp::overlayWndProc)));
    }

    std::filesystem::path keysPath() const {
        return projectRoot_ / "keys" / "keys.txt";
    }

    std::filesystem::path buttonMappingPath() const {
        return projectRoot_ / "runtime" / "config" / "button_mapping.json";
    }

    std::filesystem::path inputMappingPath() const {
        return projectRoot_ / "runtime" / "config" / "input_mapping.json";
    }

    std::filesystem::path controllerProfilesRoot() const {
        return projectRoot_ / "runtime" / "controllerProfiles";
    }

    std::filesystem::path controllerProfilePath(int playerIndex) const {
        return controllerProfilesRoot() / (playerIndex == 1 ? "skylanders_player1.xml" : "skylanders_player2.xml");
    }

    std::string controllerProfileName(int playerIndex) const {
        return playerIndex == 1 ? "skylanders_player1" : "skylanders_player2";
    }

    std::filesystem::path figurePathForCharacter(const skylanders::CharacterCatalogEntry& character) const {
        return projectRoot_ / "figures" / character.figurePath;
    }

    void rebuildProcessManager() {
        graphicPacks_ = skylanders::GraphicPackManager(makeGraphicPackManagerConfig(
            projectRoot_,
            cemuResolution_ ? cemuResolution_->cemuRoot : std::filesystem::path{}));
        manager_ = skylanders::ProcessManager(makeProcessConfig(
            projectRoot_,
            locator_,
            cemuResolution_ ? cemuResolution_->cemuRoot : std::filesystem::path{}));
        try {
            spec_ = manager_.resolveFromStub(exePath_);
        } catch (const std::exception&) {
            const auto& fallback = skylanders::gameCatalog().front();
            spec_ = {fallback.game, fallback.displayName, exePath_};
        }
    }

    bool ensureCemuConfigured() {
        if (cemuResolution_) {
            return true;
        }

        MessageBoxW(
            overlayController_.overlayWindow(),
            L"Cemu was not found. Select a Cemu.exe file to use as a fallback.",
            L"Skylanders Engine",
            MB_OK | MB_ICONWARNING);

        auto resolution = cemuLocator_.promptAndPersist(overlayController_.overlayWindow());
        if (!resolution) {
            lastError_ = "Cemu.exe was not configured.";
            pushNativeState();
            return false;
        }

        cemuResolution_ = *resolution;
        rebuildProcessManager();
        lastError_.clear();
        pushNativeState();
        startGraphicPackAutoUpdate();
        return true;
    }

    bool promptForKeysFile() {
        std::wstring buffer(MAX_PATH * 4, L'\0');

        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = overlayController_.overlayWindow();
        dialog.lpstrFilter = L"Cemu keys.txt\0keys.txt;*.txt\0Text files\0*.txt\0All files\0*.*\0\0";
        dialog.lpstrFile = buffer.data();
        dialog.nMaxFile = static_cast<DWORD>(buffer.size());
        dialog.lpstrTitle = L"Select your Cemu keys.txt";
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (!GetOpenFileNameW(&dialog)) {
            return false;
        }

        const std::filesystem::path selected(buffer.c_str());
        try {
            skylanders::ProcessManager::validateKeysFile(selected);
            const auto destination = keysPath();
            std::filesystem::create_directories(destination.parent_path());
            std::error_code ec;
            const bool sameFile = std::filesystem::exists(destination, ec) && std::filesystem::equivalent(selected, destination, ec);
            if (!sameFile) {
                std::filesystem::copy_file(selected, destination, std::filesystem::copy_options::overwrite_existing);
            }
            lastError_.clear();
            pushNativeState();
            return true;
        } catch (const std::exception& ex) {
            lastError_ = ex.what();
            pushNativeState();
            MessageBoxA(nullptr, lastError_.c_str(), "Skylanders Engine", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    bool ensureKeysConfigured() {
        try {
            skylanders::ProcessManager::validateKeysFile(keysPath());
            return true;
        } catch (const std::exception&) {
            MessageBoxW(
                overlayController_.overlayWindow(),
                L"keys.txt was not found or is invalid. Select your own Cemu keys.txt file.",
                L"Skylanders Engine",
                MB_OK | MB_ICONWARNING);
            return promptForKeysFile();
        }
    }

    bool ensureGameConfigured() {
        if (!spec_.gamePath.empty() && spec_.titleId) {
            return true;
        }

        if (const auto image = locator_.promptAndPersist(spec_.game, nullptr)) {
            applyResolvedImage(spec_, *image);
            lastError_.clear();
            pushNativeState();
            return true;
        }

        lastError_ = "Game image not configured.";
        pushNativeState();
        return false;
    }

    OverlayToggleInput::Config loadOverlayToggleConfig() const {
        OverlayToggleInput::Config config;
        std::ifstream file(inputMappingPath(), std::ios::binary);
        if (!file) {
            config.xinputMasks.push_back(XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_BACK);
            return config;
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        const std::string json = contents.str();

        const std::regex bindingRegex(R"json(\{\s*"type"\s*:\s*"(keyboard|xinput)"[^}]*\})json");
        auto begin = std::sregex_iterator(json.begin(), json.end(), bindingRegex);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            const std::string binding = it->str();
            if ((*it)[1].str() == "keyboard") {
                std::smatch keyMatch;
                if (std::regex_search(binding, keyMatch, std::regex(R"json("keyboardVk"\s*:\s*(\d+))json"))) {
                    config.keyboardVks.push_back(std::stoi(keyMatch[1].str()));
                }
            } else if ((*it)[1].str() == "xinput") {
                std::smatch maskMatch;
                if (std::regex_search(binding, maskMatch, std::regex(R"json("xinputMask"\s*:\s*(\d+))json"))) {
                    config.xinputMasks.push_back(static_cast<WORD>(std::stoi(maskMatch[1].str())));
                }
            }
        }

        const std::regex cemuBindingRegex(R"json(\{\s*"type"\s*:\s*"cemu"[^}]*\})json");
        for (auto it = std::sregex_iterator(json.begin(), json.end(), cemuBindingRegex); it != end; ++it) {
            const std::string binding = it->str();
            OverlayToggleInput::Config::CemuBinding cemuBinding;
            if (auto value = regexStringField(binding, "api")) {
                cemuBinding.api = *value;
            }
            if (auto value = regexStringField(binding, "uuid")) {
                cemuBinding.uuid = *value;
            }
            if (auto value = regexStringField(binding, "displayName")) {
                cemuBinding.displayName = *value;
            }
            std::smatch inputMatch;
            if (std::regex_search(binding, inputMatch, std::regex(R"json("sourceInputId"\s*:\s*(\d+))json"))) {
                cemuBinding.sourceInputId = std::stoull(inputMatch[1].str());
            }
            if (!cemuBinding.api.empty() && !cemuBinding.uuid.empty()) {
                config.cemuBindings.push_back(std::move(cemuBinding));
            }
        }

        if (!config.keyboardVks.empty() || !config.xinputMasks.empty() || !config.cemuBindings.empty()) {
            return config;
        }

        // Backward compatibility with the earlier single-toggle shape.
        std::smatch keyboardMatch;
        if (std::regex_search(json, keyboardMatch, std::regex(R"json("keyboardVk"\s*:\s*(\d+))json"))) {
            const int keyboardVk = std::stoi(keyboardMatch[1].str());
            if (keyboardVk > 0) {
                config.keyboardVks.push_back(keyboardVk);
            }
        }
        std::smatch xinputMatch;
        if (std::regex_search(json, xinputMatch, std::regex(R"json("xinputMask"\s*:\s*(\d+))json"))) {
            const WORD mask = static_cast<WORD>(std::stoi(xinputMatch[1].str()));
            if (mask != 0) {
                config.xinputMasks.push_back(mask);
            }
        }
        if (json.find("\"bindings\"") != std::string::npos) {
            return config;
        }

        if (config.keyboardVks.empty() && config.xinputMasks.empty()) {
            config.xinputMasks.push_back(XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_BACK);
        }
        return config;
    }

    void pushNativeState() {
        if (overlay_ && jsReady_) {
            overlay_->view()->EvaluateScript(ultralight::String(("window.setNativeState(" + nativeStateJson() + ");").c_str()));
        }
    }

    void startGraphicPackAutoUpdate() {
        if (!cemuResolution_ || graphicPackUpdateRunning_ || !graphicPacks_.shouldAutoCheck()) {
            return;
        }

        const HWND targetWindow = overlayController_.overlayWindow();
        graphicPackUpdateRunning_ = true;
        graphicPackUpdateThread_ = std::thread([this, targetWindow]() {
            graphicPacks_.update(false);
            graphicPackUpdateRunning_ = false;
            if (targetWindow) {
                PostMessageW(targetWindow, kGraphicPacksUpdatedMessage, 0, 0);
            }
        });
    }

    void stopGraphicPackAutoUpdate() {
        if (graphicPackUpdateThread_.joinable()) {
            graphicPackUpdateThread_.join();
        }
        graphicPackUpdateRunning_ = false;
    }

    void onGraphicPacksUpdated() {
        pushNativeState();
    }

    bool startGame() {
        if (launchAttempted_) {
            return gameRunning_;
        }
        launchAttempted_ = true;

        try {
            if (!ensureCemuConfigured()) {
                MessageBoxA(
                    nullptr,
                    "No Cemu executable was selected. Launch cancelled.",
                    "Skylanders Engine",
                    MB_OK | MB_ICONWARNING);
                app_->Quit();
                return false;
            }
            startGraphicPackAutoUpdate();
            if (!ensureGameConfigured()) {
                MessageBoxA(
                    nullptr,
                    "No valid Wii U game image was selected. Launch cancelled.",
                    "Skylanders Engine",
                    MB_OK | MB_ICONWARNING);
                app_->Quit();
                return false;
            }
            if (!ensureKeysConfigured()) {
                MessageBoxA(
                    nullptr,
                    "No valid keys.txt was selected. Launch cancelled.",
                    "Skylanders Engine",
                    MB_OK | MB_ICONWARNING);
                app_->Quit();
                return false;
            }
            clearAllPortalSlots();
            lastPortalError_.clear();
#if SKYLANDERS_HAS_CEMU
            if (core_.available()) {
                running_ = {};
                gameRunning_ = core_.start(spec_);
                if (!gameRunning_) {
                    throw std::runtime_error("Embedded Cemu failed to start.");
                }
                pushNativeState();
                return true;
            }
#endif
            running_ = manager_.launch(spec_);
            gameRunning_ = running_.processId != 0;
            if (!gameRunning_) {
                throw std::runtime_error("Cemu failed to start.");
            }

            overlayController_.attachToProcess(running_.processId);
            overlayController_.hide();
            input_.setConfig(loadOverlayToggleConfig());
            input_.setCemuInputBridge(&inputBridge_, running_.processId);
            input_.start(overlayController_.overlayWindow());
            startProcessWatcher();
            pushNativeState();
            return true;
        } catch (const std::exception& ex) {
            lastError_ = ex.what();
            gameRunning_ = false;
            pushNativeState();
            MessageBoxA(nullptr, lastError_.c_str(), "Skylanders Engine", MB_OK | MB_ICONERROR);
            app_->Quit();
            return false;
        }
    }

    void onCemuExited() {
        stopProcessWatcher();
        input_.stop();
        input_.setCemuInputBridge(nullptr, 0);
        overlayController_.hide();
        closeRunningProcessHandle();
        gameRunning_ = false;
        lastError_.clear();
        lastPortalError_.clear();
        clearAllPortalSlots();
        pushNativeState();
        app_->Quit();
    }

    ultralight::JSValue selectGameFile(const ultralight::JSObject&, const ultralight::JSArgs&) {
        return ultralight::JSValue(ensureGameConfigured());
    }

    ultralight::JSValue launchGame(const ultralight::JSObject&, const ultralight::JSArgs&) {
        return ultralight::JSValue(startGame());
    }

    ultralight::JSValue toggleOverlayCallback(const ultralight::JSObject&, const ultralight::JSArgs&) {
        toggleOverlay();
        return ultralight::JSValue(overlayController_.visible());
    }

    ultralight::JSValue showOverlayCallback(const ultralight::JSObject&, const ultralight::JSArgs&) {
        if (gameRunning_) {
            overlayController_.show();
            pushNativeState();
        }
        return ultralight::JSValue(overlayController_.visible());
    }

    ultralight::JSValue hideOverlayCallback(const ultralight::JSObject&, const ultralight::JSArgs&) {
        overlayController_.hide();
        pushNativeState();
        return ultralight::JSValue(true);
    }

    ultralight::JSValue isOverlayVisibleCallback(const ultralight::JSObject&, const ultralight::JSArgs&) {
        return ultralight::JSValue(overlayController_.visible());
    }

    ultralight::JSValue getGraphicPackStatus(const ultralight::JSObject&, const ultralight::JSArgs&) {
        const auto json = skylanders::GraphicPackManager::stateToJson(graphicPacks_.loadState());
        return ultralight::JSValue(ultralight::String(json.c_str()));
    }

    ultralight::JSValue updateGraphicPacks(const ultralight::JSObject&, const ultralight::JSArgs&) {
        const auto result = graphicPacks_.update(true);
        if (!result.ok && !result.error.empty()) {
            lastError_ = result.error;
        } else if (result.ok) {
            lastError_.clear();
        }
        pushNativeState();
        return ultralight::JSValue(result.ok);
    }

    ultralight::JSValue portalPlace(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (!overlayController_.visible() || args.size() < 2) {
            return ultralight::JSValue(false);
        }

        const int slot = static_cast<int>(args[0].ToInteger());
        const auto filePath = std::filesystem::path(jsValueToUtf8(args[1]));
#if SKYLANDERS_HAS_CEMU
        if (core_.available()) {
            return ultralight::JSValue(core_.placeFigure(slot, filePath));
        }
#endif
        if (running_.processId == 0) {
            return ultralight::JSValue(false);
        }
        const auto result = portal_.place(running_.processId, slot, filePath);
        lastPortalError_ = result.ok ? std::string{} : result.error;
        pushNativeState();
        return ultralight::JSValue(result.ok);
    }

    ultralight::JSValue portalPlaceOrCreate(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (!overlayController_.visible() || args.size() < 2) {
            return ultralight::JSValue(false);
        }

        const int slot = static_cast<int>(args[0].ToInteger());
        const auto characterId = jsValueToUtf8(args[1]);
        const auto* character = skylanders::findCharacter(characterId);
        if (!character) {
            lastPortalError_ = "Unknown Skylander character id: " + characterId;
            pushNativeState();
            return ultralight::JSValue(false);
        }

        const auto figurePath = figurePathForCharacter(*character);
        try {
            std::error_code ec;
            if (std::filesystem::exists(figurePath, ec)) {
                const auto validation = skylanders::validateFigureFile(figurePath, character->skyId, character->skyVar);
                if (!validation.valid) {
                    lastPortalError_ = validation.reason;
                    pushNativeState();
                    return ultralight::JSValue(false);
                }
            } else {
                std::filesystem::create_directories(figurePath.parent_path());
            }

#if SKYLANDERS_HAS_CEMU
            if (core_.available()) {
                const bool ok = core_.placeOrCreateFigure(slot, figurePath, character->skyId, character->skyVar);
                if (!ok) {
                    lastPortalError_ = "Cemu could not create or place the selected Skylander.";
                } else {
                    setPortalSlot(slot, *character, figurePath, std::nullopt);
                    lastPortalError_.clear();
                }
                pushNativeState();
                return ultralight::JSValue(ok);
            }
#endif
            if (running_.processId == 0) {
                lastPortalError_ = "Cemu is not running.";
                pushNativeState();
                return ultralight::JSValue(false);
            }

            const auto result = portal_.placeOrCreate(running_.processId, slot, figurePath, character->skyId, character->skyVar);
            if (!result.ok) {
                lastPortalError_ = result.error.empty()
                    ? "Portal bridge unavailable for this Cemu build."
                    : result.error;
            } else {
                setPortalSlot(slot, *character, figurePath, result.portalSlot);
                lastPortalError_.clear();
            }
            pushNativeState();
            return ultralight::JSValue(result.ok);
        } catch (const std::exception& ex) {
            lastPortalError_ = ex.what();
            pushNativeState();
            return ultralight::JSValue(false);
        }
    }

    ultralight::JSValue portalRemove(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (!overlayController_.visible() || args.empty()) {
            return ultralight::JSValue(false);
        }

        const int slot = static_cast<int>(args[0].ToInteger());
#if SKYLANDERS_HAS_CEMU
        if (core_.available()) {
            const bool ok = core_.removeFigure(slot);
            if (ok) {
                clearPortalSlot(slot);
                lastPortalError_.clear();
            } else {
                lastPortalError_ = "Cemu could not clear the selected portal slot.";
            }
            pushNativeState();
            return ultralight::JSValue(ok);
        }
#endif
        if (running_.processId == 0) {
            lastPortalError_ = "Cemu is not running.";
            pushNativeState();
            return ultralight::JSValue(false);
        }
        const auto result = portal_.remove(running_.processId, slot);
        if (result.ok) {
            clearPortalSlot(slot);
            lastPortalError_.clear();
        } else {
            lastPortalError_ = result.error.empty() ? "Cemu could not clear the selected portal slot." : result.error;
        }
        pushNativeState();
        return ultralight::JSValue(result.ok);
    }

    ultralight::JSValue loadButtonMappings(const ultralight::JSObject&, const ultralight::JSArgs&) {
        std::ifstream file(buttonMappingPath(), std::ios::binary);
        if (!file) {
            return ultralight::JSValue(ultralight::String("{}"));
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        return ultralight::JSValue(ultralight::String(contents.str().c_str()));
    }

    ultralight::JSValue saveButtonMappings(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (args.empty()) {
            return ultralight::JSValue(false);
        }

        const auto json = jsValueToUtf8(args[0]);
        try {
            const auto path = buttonMappingPath();
            std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file) {
                return ultralight::JSValue(false);
            }
            file << json;
            return ultralight::JSValue(true);
        } catch (const std::exception&) {
            return ultralight::JSValue(false);
        }
    }

    ultralight::JSValue loadInputMappings(const ultralight::JSObject&, const ultralight::JSArgs&) {
        std::ifstream file(inputMappingPath(), std::ios::binary);
        if (!file) {
            return ultralight::JSValue(ultralight::String("{}"));
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        return ultralight::JSValue(ultralight::String(contents.str().c_str()));
    }

    ultralight::JSValue inputPing(const ultralight::JSObject&, const ultralight::JSArgs&) {
        return ultralight::JSValue(running_.processId != 0 && inputBridge_.ping(running_.processId));
    }

    ultralight::JSValue inputListDevices(const ultralight::JSObject&, const ultralight::JSArgs&) {
        const auto result = inputBridge_.listDevices(running_.processId);
        if (!result.ok) {
            const auto json = std::string("{\"ok\":false,\"error\":\"") + jsonEscape(result.error) + "\"}";
            return ultralight::JSValue(ultralight::String(json.c_str()));
        }
        return ultralight::JSValue(ultralight::String(result.json.c_str()));
    }

    ultralight::JSValue inputCaptureNext(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (args.size() < 5) {
            return ultralight::JSValue(ultralight::String("{\"ok\":false,\"error\":\"Invalid capture request.\"}"));
        }

        const auto api = jsValueToUtf8(args[0]);
        const auto uuid = jsValueToUtf8(args[1]);
        const auto displayName = jsValueToUtf8(args[2]);
        const int mapping = static_cast<int>(args[3].ToInteger());
        const int timeoutMs = static_cast<int>(args[4].ToInteger());
        const auto result = inputBridge_.captureNext(running_.processId, api, uuid, displayName, mapping, timeoutMs);
        if (!result.ok) {
            const auto json = std::string("{\"ok\":false,\"error\":\"") + jsonEscape(result.error) + "\"}";
            return ultralight::JSValue(ultralight::String(json.c_str()));
        }
        return ultralight::JSValue(ultralight::String(result.json.c_str()));
    }

    ultralight::JSValue inputStartCapture(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (args.size() < 5) {
            return ultralight::JSValue(ultralight::String("{\"ok\":false,\"error\":\"Invalid capture request.\"}"));
        }

        const auto api = jsValueToUtf8(args[0]);
        const auto uuid = jsValueToUtf8(args[1]);
        const auto displayName = jsValueToUtf8(args[2]);
        const int mapping = static_cast<int>(args[3].ToInteger());
        const auto captureMode = jsValueToUtf8(args[4]);
        const auto result = inputBridge_.startCapture(running_.processId, api, uuid, displayName, mapping, captureMode);
        const auto json = result.json.empty()
            ? (std::string("{\"ok\":false,\"error\":\"") + jsonEscape(result.error) + "\"}")
            : result.json;
        return ultralight::JSValue(ultralight::String(json.c_str()));
    }

    ultralight::JSValue inputPollCapture(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (args.empty()) {
            return ultralight::JSValue(ultralight::String("{\"ok\":false,\"error\":\"Invalid capture session.\"}"));
        }

        const int captureId = static_cast<int>(args[0].ToInteger());
        const auto result = inputBridge_.pollCapture(running_.processId, captureId);
        const auto json = result.json.empty()
            ? (std::string("{\"ok\":false,\"error\":\"") + jsonEscape(result.error) + "\"}")
            : result.json;
        return ultralight::JSValue(ultralight::String(json.c_str()));
    }

    ultralight::JSValue inputCancelCapture(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (args.empty()) {
            return ultralight::JSValue(false);
        }

        const int captureId = static_cast<int>(args[0].ToInteger());
        const auto result = inputBridge_.cancelCapture(running_.processId, captureId);
        return ultralight::JSValue(result.ok);
    }

    std::string xinputMaskLabel(WORD mask) const {
        struct ButtonName {
            WORD mask;
            const char* label;
        };
        static constexpr ButtonName names[] = {
            {XINPUT_GAMEPAD_DPAD_UP, "D-Pad Up"},
            {XINPUT_GAMEPAD_DPAD_DOWN, "D-Pad Down"},
            {XINPUT_GAMEPAD_DPAD_LEFT, "D-Pad Left"},
            {XINPUT_GAMEPAD_DPAD_RIGHT, "D-Pad Right"},
            {XINPUT_GAMEPAD_START, "Start"},
            {XINPUT_GAMEPAD_BACK, "Back"},
            {XINPUT_GAMEPAD_LEFT_THUMB, "Left Stick"},
            {XINPUT_GAMEPAD_RIGHT_THUMB, "Right Stick"},
            {XINPUT_GAMEPAD_LEFT_SHOULDER, "LB"},
            {XINPUT_GAMEPAD_RIGHT_SHOULDER, "RB"},
            {XINPUT_GAMEPAD_A, "A"},
            {XINPUT_GAMEPAD_B, "B"},
            {XINPUT_GAMEPAD_X, "X"},
            {XINPUT_GAMEPAD_Y, "Y"},
        };

        std::string label;
        for (const auto& name : names) {
            if ((mask & name.mask) == 0) {
                continue;
            }
            if (!label.empty()) {
                label += " + ";
            }
            label += name.label;
        }
        return label.empty() ? "No buttons" : label;
    }

    ultralight::JSValue inputReadXinputButtons(const ultralight::JSObject&, const ultralight::JSArgs&) {
        using XInputGetStateLocalFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);
        HMODULE module = nullptr;
        XInputGetStateLocalFn localXInputGetState = nullptr;
        for (const wchar_t* dll : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"}) {
            module = LoadLibraryW(dll);
            if (!module) {
                continue;
            }
            localXInputGetState = reinterpret_cast<XInputGetStateLocalFn>(GetProcAddress(module, "XInputGetState"));
            if (localXInputGetState) {
                break;
            }
            FreeLibrary(module);
            module = nullptr;
        }

        WORD mask = 0;
        if (localXInputGetState) {
            for (DWORD index = 0; index < XUSER_MAX_COUNT; ++index) {
                XINPUT_STATE state{};
                if (localXInputGetState(index, &state) == ERROR_SUCCESS) {
                    mask = static_cast<WORD>(mask | state.Gamepad.wButtons);
                }
            }
        }
        if (module) {
            FreeLibrary(module);
        }

        const auto label = xinputMaskLabel(mask);
        std::ostringstream json;
        json << "{\"ok\":true,\"xinputMask\":" << mask << ",\"label\":\"" << jsonEscape(label) << "\"}";
        return ultralight::JSValue(ultralight::String(json.str().c_str()));
    }

    ultralight::JSValue inputSuppressOverlayToggle(const ultralight::JSObject&, const ultralight::JSArgs&) {
        input_.suppressUntilReleased();
        return ultralight::JSValue(true);
    }

    ultralight::JSValue inputLoadProfile(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (args.empty()) {
            return ultralight::JSValue(false);
        }

        const int playerIndex = static_cast<int>(args[0].ToInteger());
        if (running_.processId == 0) {
            return ultralight::JSValue(false);
        }
        const auto result = inputBridge_.loadProfile(running_.processId, playerIndex - 1, controllerProfileName(playerIndex));
        return ultralight::JSValue(result.ok);
    }

    ultralight::JSValue inputSaveProfile(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (args.size() < 3) {
            return ultralight::JSValue(false);
        }

        const int playerIndex = static_cast<int>(args[0].ToInteger());
        const auto profileXml = jsValueToUtf8(args[1]);
        const auto configJson = jsValueToUtf8(args[2]);
        if (playerIndex != 1 && playerIndex != 2) {
            return ultralight::JSValue(false);
        }

        try {
            const auto profilePath = controllerProfilePath(playerIndex);
            std::filesystem::create_directories(profilePath.parent_path());
            {
                std::ofstream profile(profilePath, std::ios::binary | std::ios::trunc);
                if (!profile) {
                    return ultralight::JSValue(false);
                }
                profile << profileXml;
            }

            const auto configPath = inputMappingPath();
            std::filesystem::create_directories(configPath.parent_path());
            {
                std::ofstream config(configPath, std::ios::binary | std::ios::trunc);
                if (!config) {
                    return ultralight::JSValue(false);
                }
                config << configJson;
            }

            if (cemuResolution_) {
                const auto activePath = cemuResolution_->cemuRoot / "controllerProfiles" / (controllerProfileName(playerIndex) + ".xml");
                std::filesystem::create_directories(activePath.parent_path());
                std::filesystem::copy_file(profilePath, activePath, std::filesystem::copy_options::overwrite_existing);
            }

            input_.setConfig(loadOverlayToggleConfig());
            if (running_.processId != 0) {
                inputBridge_.loadProfile(running_.processId, playerIndex - 1, controllerProfileName(playerIndex));
            }
            return ultralight::JSValue(true);
        } catch (const std::exception&) {
            return ultralight::JSValue(false);
        }
    }

    struct PortalSlotState {
        std::string characterId;
        std::string displayName;
        std::string element;
        std::filesystem::path figurePath;
        std::optional<int> portalSlot;
    };

    bool portalReady() const {
        if (!gameRunning_) {
            return false;
        }
#if SKYLANDERS_HAS_CEMU
        if (core_.available()) {
            return core_.isRunning();
        }
#endif
        return running_.processId != 0 && portal_.ping(running_.processId);
    }

    void setPortalSlot(
        int slot,
        const skylanders::CharacterCatalogEntry& character,
        const std::filesystem::path& figurePath,
        std::optional<int> portalSlot) {
        if (slot < 0 || slot >= static_cast<int>(portalSlots_.size())) {
            return;
        }
        portalSlots_[slot] = PortalSlotState{
            character.id,
            character.displayName,
            character.element,
            figurePath,
            portalSlot,
        };
    }

    void clearPortalSlot(int slot) {
        if (slot < 0 || slot >= static_cast<int>(portalSlots_.size())) {
            return;
        }
        portalSlots_[slot].reset();
    }

    void clearAllPortalSlots() {
        for (auto& slot : portalSlots_) {
            slot.reset();
        }
    }

    std::string portalSlotsJson() const {
        std::ostringstream json;
        json << "[";
        for (std::size_t i = 0; i < portalSlots_.size(); ++i) {
            if (i != 0) {
                json << ",";
            }
            json << "{";
            json << "\"slot\":" << i << ",";
            if (portalSlots_[i]) {
                const auto& slot = *portalSlots_[i];
                json << "\"empty\":false,";
                json << "\"characterId\":\"" << jsonEscape(slot.characterId) << "\",";
                json << "\"displayName\":\"" << jsonEscape(slot.displayName) << "\",";
                json << "\"element\":\"" << jsonEscape(slot.element) << "\",";
                json << "\"figurePath\":\"" << jsonEscape(slot.figurePath.string()) << "\"";
                if (slot.portalSlot) {
                    json << ",\"portalSlot\":" << *slot.portalSlot;
                }
            } else {
                json << "\"empty\":true";
            }
            json << "}";
        }
        json << "]";
        return json.str();
    }

    std::string nativeStateJson() const {
        std::ostringstream json;
        json << "{";
        json << "\"game\":\"" << jsonEscape(narrow(skylanders::gameIdToString(spec_.game))) << "\",";
        json << "\"displayName\":\"" << jsonEscape(narrow(spec_.displayName)) << "\",";
        json << "\"stub\":\"" << jsonEscape(exePath_.string()) << "\",";
        json << "\"gamePath\":\"" << jsonEscape(spec_.gamePath.string()) << "\",";
        json << "\"hasGamePath\":" << (spec_.gamePath.empty() ? "false" : "true") << ",";
        const bool keysPresent = std::filesystem::exists(projectRoot_ / "keys" / "keys.txt");
        json << "\"keysPresent\":" << (keysPresent ? "true" : "false") << ",";
        json << "\"keysStatus\":\"" << (keysPresent ? "present" : "missing") << "\",";
        json << "\"titleId\":\"" << (spec_.titleId ? jsonEscape(std::to_string(*spec_.titleId)) : "") << "\",";
        json << "\"region\":\"" << jsonEscape(narrow(spec_.region)) << "\",";
        json << "\"embeddedCemu\":" << (core_.available() ? "true" : "false") << ",";
        json << "\"hasCemu\":" << (cemuResolution_ ? "true" : "false") << ",";
        json << "\"cemuRoot\":\"" << (cemuResolution_ ? jsonEscape(cemuResolution_->cemuRoot.string()) : "") << "\",";
        json << "\"cemuExe\":\"" << (cemuResolution_ ? jsonEscape(cemuResolution_->cemuExe.string()) : "") << "\",";
        json << "\"cemuSource\":\"" << (cemuResolution_ ? jsonEscape(skylanders::CemuLocator::sourceToString(cemuResolution_->source)) : "") << "\",";
        json << "\"cemuBackendMode\":\"" << jsonEscape(skylanders::cemuBackendModeToString(setupConfig_.cemuBackendMode.value_or(skylanders::CemuBackendMode::BundledExternal))) << "\",";
        json << "\"overlayBackendMode\":\"" << jsonEscape(skylanders::overlayBackendModeToString(setupConfig_.overlayBackendMode.value_or(skylanders::OverlayBackendMode::ExternalWindow))) << "\",";
        json << "\"nativeOverlay\":" << nativeOverlay_.nativeStateJson() << ",";
        json << "\"running\":" << (gameRunning_ ? "true" : "false") << ",";
        json << "\"inputBridgeReady\":" << ((running_.processId != 0 && inputBridge_.ping(running_.processId)) ? "true" : "false") << ",";
        json << "\"portalReady\":" << (portalReady() ? "true" : "false") << ",";
        json << "\"portalSlots\":" << portalSlotsJson() << ",";
        json << "\"lastPortalError\":\"" << jsonEscape(lastPortalError_) << "\",";
        json << "\"graphicPacks\":" << skylanders::GraphicPackManager::stateToJson(graphicPacks_.loadState()) << ",";
        json << "\"cemuPreparation\":" << skylanders::ProcessManager::preparationReportJson(manager_.lastPreparationReport()) << ",";
        json << "\"overlayVisible\":" << (overlayController_.visible() ? "true" : "false") << ",";
        json << "\"lastError\":\"" << jsonEscape(lastError_) << "\",";
        json << "\"characters\":" << skylanders::characterCatalogJson();
        json << "}";
        return json.str();
    }

    std::filesystem::path exePath_;
    std::filesystem::path projectRoot_;
    skylanders::RomLocator locator_;
    skylanders::CemuLocator cemuLocator_;
    skylanders::SetupConfig setupConfig_ = skylanders::SetupConfig::load(projectRoot_ / "runtime" / "config" / "setup.json");
    std::optional<skylanders::CemuResolution> cemuResolution_;
    skylanders::GraphicPackManager graphicPacks_;
    skylanders::ProcessManager manager_;
    skylanders::EmbeddedCemuHost embeddedHost_;
    skylanders::EmulatorCore core_;
    skylanders::GameLaunchSpec spec_;
    skylanders::PortalBridge portal_;
    skylanders::InputBridge inputBridge_;
    skylanders::InputRouter inputRouter_;
    skylanders::NativeOverlayRenderer nativeOverlay_;
    skylanders::RunningProcess running_;
    std::string lastError_;
    std::string lastPortalError_;
    std::array<std::optional<PortalSlotState>, 4> portalSlots_{};
    OverlayController overlayController_;
    OverlayToggleInput input_;
    std::atomic_bool processWatcherRunning_{false};
    std::thread processWatcher_;
    std::atomic_bool graphicPackUpdateRunning_{false};
    std::thread graphicPackUpdateThread_;
    bool jsReady_ = false;
    bool launchAttempted_ = false;
    bool gameRunning_ = false;

    ultralight::RefPtr<ultralight::App> app_;
    ultralight::RefPtr<ultralight::Window> window_;
    ultralight::RefPtr<ultralight::Overlay> overlay_;

    void startProcessWatcher() {
        stopProcessWatcher();
        const HANDLE processHandle = static_cast<HANDLE>(running_.processHandle);
        const HWND targetWindow = overlayController_.overlayWindow();
        if (!processHandle || !targetWindow) {
            return;
        }

        processWatcherRunning_ = true;
        processWatcher_ = std::thread([this, processHandle, targetWindow]() {
            while (processWatcherRunning_) {
                const DWORD waitResult = WaitForSingleObject(processHandle, 250);
                if (waitResult == WAIT_OBJECT_0) {
                    processWatcherRunning_ = false;
                    PostMessageW(targetWindow, kCemuExitedMessage, 0, 0);
                    return;
                }
                if (waitResult == WAIT_FAILED) {
                    processWatcherRunning_ = false;
                    return;
                }
            }
        });
    }

    void stopProcessWatcher() {
        processWatcherRunning_ = false;
        if (processWatcher_.joinable()) {
            processWatcher_.join();
        }
    }

    void closeRunningProcessHandle() {
        if (running_.processHandle) {
            CloseHandle(static_cast<HANDLE>(running_.processHandle));
            running_.processHandle = nullptr;
        }
        running_.processId = 0;
    }
};
#endif

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const auto exe = executablePath();

#if SKYLANDERS_WITH_ULTRALIGHT
    try {
        SkylandersApp app(exe);
        app.run();
        return 0;
    } catch (const std::exception& ex) {
        MessageBoxA(nullptr, ex.what(), "Skylanders Engine", MB_OK | MB_ICONERROR);
        return 1;
    }
#else
    MessageBoxA(
        nullptr,
        "SkylandersEngine was built without Ultralight. Set ULTRALIGHT_SDK_ROOT and rebuild.",
        "Skylanders Engine",
        MB_OK | MB_ICONWARNING);
    return 1;
#endif
}
