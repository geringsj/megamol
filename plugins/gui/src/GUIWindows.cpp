/*
 * GUIWindows.cpp
 *
 * Copyright (C) 2018 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

/**
 * USED HOTKEYS:
 *
 * - Show/hide Windows: F8-F12
 * - Reset windows:     Shift + F8-F12
 * - Search Paramter:   Shift + Alt + p
 * - Save Project:      Shift + Alt + s
 * - Quit program:      Alt   + F4
 */

#include "stdafx.h"
#include "GUIWindows.h"


using namespace megamol;
using namespace megamol::gui;


GUIWindows::GUIWindows()
    : core_instance(nullptr)
    , param_slots()
    , style_param("style", "Color style, theme")
    , state_param("state", "Current state of all windows. Automatically updated.")
    , autostart_configurator("autostart_configurator", "Start the configurator at start up automatically. ")
    , context(nullptr)
    , impl(Implementation::NONE)
    , window_manager()
    , tf_editor()
    , configurator()
    , utils()
    , file_utils()
    , state()
    , parent_module_fullname()
    , widgtmap_text()
    , widgtmap_int()
    , widgtmap_float()
    , widgtmap_vec2()
    , widgtmap_vec3()
    , widgtmap_vec4()
    , graph_fonts_reserved(0) {

    core::param::EnumParam* styles = new core::param::EnumParam((int)(Styles::DarkColors));
    styles->SetTypePair(Styles::CorporateGray, "Corporate Gray");
    styles->SetTypePair(Styles::CorporateWhite, "Corporate White");
    styles->SetTypePair(Styles::DarkColors, "Dark Colors");
    styles->SetTypePair(Styles::LightColors, "Light Colors");
    this->style_param << styles;
    this->style_param.ForceSetDirty();
    styles = nullptr;

    this->state_param << new core::param::StringParam("");
    this->state_param.Parameter()->SetGUIVisible(false); 

    this->autostart_configurator << new core::param::BoolParam(false);

    this->param_slots.clear();
    this->param_slots.push_back(&this->state_param);
    this->param_slots.push_back(&this->style_param);
    this->param_slots.push_back(&this->autostart_configurator);
    for (auto& configurator_param : this->configurator.GetParams()) {
        this->param_slots.push_back(configurator_param);
    }

    this->hotkeys[GUIWindows::GuiHotkeyIndex::EXIT_PROGRAM] = megamol::gui::HotkeyDataType(
        megamol::core::view::KeyCode(megamol::core::view::Key::KEY_F4, core::view::Modifier::ALT), false);
    this->hotkeys[GUIWindows::GuiHotkeyIndex::PARAMETER_SEARCH] =
        megamol::gui::HotkeyDataType(megamol::core::view::KeyCode(megamol::core::view::Key::KEY_P,
                                         core::view::Modifier::CTRL | core::view::Modifier::ALT),
            false);
    this->hotkeys[GUIWindows::GuiHotkeyIndex::SAVE_PROJECT] =
        megamol::gui::HotkeyDataType(megamol::core::view::KeyCode(megamol::core::view::Key::KEY_S,
                                         core::view::Modifier::CTRL | core::view::Modifier::ALT),
            false);
}


GUIWindows::~GUIWindows() { this->destroyContext(); }


bool GUIWindows::CreateContext_GL(megamol::core::CoreInstance* instance) {

    if (instance == nullptr) {
        vislib::sys::Log::DefaultLog.WriteError(
            "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    this->core_instance = instance;

    if (this->createContext()) {
        // Init OpenGL for ImGui
        const char* glsl_version = "#version 130"; /// "#version 150"
        if (ImGui_ImplOpenGL3_Init(glsl_version)) {
            this->impl = Implementation::OpenGL;
            return true;
        }
    }

    return false;
}


bool GUIWindows::PreDraw(const std::string& module_fullname, vislib::math::Rectangle<int> viewport, double instanceTime) {
    
    if (this->impl == Implementation::NONE) {
        vislib::sys::Log::DefaultLog.WriteError(
            "Found no initialized ImGui implementation. First call CreateContext_XXX() once. [%s, %s, line %d]\n",
            __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    ImGui::SetCurrentContext(this->context);
    this->core_instance->SetCurrentImGuiContext(this->context);
    if (this->context == nullptr) {
        vislib::sys::Log::DefaultLog.WriteError(
            "Found no valid ImGui context. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    if (std::get<1>(this->hotkeys[GUIWindows::GuiHotkeyIndex::EXIT_PROGRAM])) {
        this->shutdown();
        return true;
    }
    this->validateParameter();
    this->checkMultipleHotkeyAssignement();
    this->parent_module_fullname = module_fullname;

    auto viewportWidth = viewport.Width();
    auto viewportHeight = viewport.Height();

    // Set IO stuff for next frame --------------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)viewportWidth, (float)viewportHeight);
    io.DisplayFramebufferScale = ImVec2(1.0, 1.0);

    if ((instanceTime - this->state.last_instance_time) < 0.0) {
        vislib::sys::Log::DefaultLog.WriteWarn(
            "Current instance time results in negative time delta. [%s, %s, line %d]\n", __FILE__, __FUNCTION__,
            __LINE__);
    }
    io.DeltaTime = ((instanceTime - this->state.last_instance_time) > 0.0)
                       ? (static_cast<float>(instanceTime - this->state.last_instance_time))
                       : (io.DeltaTime);
    this->state.last_instance_time = ((instanceTime - this->state.last_instance_time) > 0.0)
                                         ? (instanceTime)
                                         : (this->state.last_instance_time + io.DeltaTime);

    // Changes that need to be applied before next frame ----------------------
    // Loading new font (set in FONT window)
    if (!this->state.font_file.empty()) {
        ImFontConfig config;
        config.OversampleH = 4;
        config.OversampleV = 4;
        config.GlyphRanges = this->state.font_utf8_ranges.data();

        GUIUtils::Utf8Encode(this->state.font_file);
        io.Fonts->AddFontFromFileTTF(this->state.font_file.c_str(), this->state.font_size, &config);
        ImGui_ImplOpenGL3_CreateFontsTexture();
        // Load last added font
        io.FontDefault = io.Fonts->Fonts[(io.Fonts->Fonts.Size - 1)];
        this->state.font_file.clear();
    }

    // Loading new font from state (set in loaded FONT window configuration)
    if (this->state.font_index >= this->graph_fonts_reserved) {
        if (this->state.font_index < static_cast<unsigned int>(io.Fonts->Fonts.Size)) {
            io.FontDefault = io.Fonts->Fonts[this->state.font_index];
        }
        this->state.font_index = GUI_INVALID_ID;
    }

    // Deleting window (set in menu of MAIN window)
    if (!this->state.win_delete.empty()) {
        this->window_manager.DeleteWindowConfiguration(this->state.win_delete);
        this->state.win_delete.clear();
    }

    // Start new ImGui frame --------------------------------------------------
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    return true;
}


bool GUIWindows::PostDraw(void) {

    if (this->impl == Implementation::NONE) {
        vislib::sys::Log::DefaultLog.WriteError(
            "Found no initialized ImGui implementation. First call CreateContext_XXX() once. [%s, %s, line %d]\n",
            __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    if (ImGui::GetCurrentContext() != this->context) {
        vislib::sys::Log::DefaultLog.WriteWarn(
            "Unknown ImGui context ... [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    ImGui::SetCurrentContext(this->context);
    if (this->context == nullptr) {
        vislib::sys::Log::DefaultLog.WriteError(
            "Found no valid ImGui context. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 viewport = ImVec2(io.DisplaySize.x, io.DisplaySize.y);

    // Draw GUI Windows -------------------------------------------------------
    const auto func = [&, this](const std::string& wn, WindowManager::WindowConfiguration& wc) {
        // Loading font (from FONT window configuration - even if FONT window is not shown)
        if (wc.buf_font_reset) {
            if (!wc.font_name.empty()) {
                this->state.font_index = GUI_INVALID_ID;
                for (unsigned int n = this->graph_fonts_reserved; n < static_cast<unsigned int>(io.Fonts->Fonts.Size);
                     n++) {
                    std::string font_name = std::string(io.Fonts->Fonts[n]->GetDebugName());
                    GUIUtils::Utf8Decode(font_name);
                    if (font_name == wc.font_name) {
                        this->state.font_index = n;
                    }
                }
                if (this->state.font_index == GUI_INVALID_ID) {
                    vislib::sys::Log::DefaultLog.WriteWarn(
                        "Could not find font '%s' for loaded state. [%s, %s, line %d]\n", wc.font_name.c_str(),
                        __FILE__, __FUNCTION__, __LINE__);
                }
            }
            wc.buf_font_reset = false;
        }

        // Draw window content
        if (wc.win_show) {
            ImGui::SetNextWindowBgAlpha(1.0f);
            if (!ImGui::Begin(wn.c_str(), &wc.win_show, wc.win_flags)) {
                ImGui::End(); // early ending
                return;
            }

            // Always set configurator window size to current viewport
            if (wc.win_callback == WindowManager::DrawCallbacks::CONFIGURATOR) {
                wc.win_size = viewport;
                wc.win_reset = true;
            }

            // Apply soft reset of window position and size (before calling window callback)
            if (wc.win_soft_reset) {
                this->window_manager.SoftResetWindowSizePos(wn, wc);
                wc.win_soft_reset = false;
            }
            // Apply reset after new state has been loaded (before calling window callback)
            if (wc.win_reset) {
                this->window_manager.ResetWindowOnStateLoad(wn, wc);
                wc.win_reset = false;
            }

            // Calling callback drawing window content
            auto cb = this->window_manager.WindowCallback(wc.win_callback);
            if (cb) {
                cb(wn, wc);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "Missing valid callback for WindowDrawCallback: '%d'.[%s, %s, line %d]\n", (int)wc.win_callback,
                    __FILE__, __FUNCTION__, __LINE__);
            }

            // Saving current window position and size for all window configurations for possible state saving.
            wc.win_position = ImGui::GetWindowPos();
            wc.win_size = ImGui::GetWindowSize();

            ImGui::End();
        }
    };
    this->window_manager.EnumWindows(func);
    
    // Draw global parameter presentation --------------------------------------
    if (this->core_instance != nullptr) {
        this->core_instance->EnumParameters([&, this](const auto& mod, auto& slot) {
            auto parameter = slot.Parameter();
            if (!parameter.IsNull()) {
                auto hoverFlags = ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled |
                                  ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;
                if (!ImGui::IsWindowHovered(hoverFlags)) {
                    if (parameter->GetGUIPresentation() == megamol::core::param::AbstractParam::Presentations::PIN_VALUE_TO_MOUSE) {
                        ImGui::BeginTooltip();
                        std::string label = std::string(slot.Name().PeekBuffer());
                        ImGui::TextDisabled(label.c_str());
                        ImGui::SameLine();
                        ImGui::TextUnformatted(parameter->ValueString().PeekBuffer());
                        ImGui::EndTooltip();
                    }
                }
            }
        });
    } else {
        vislib::sys::Log::DefaultLog.WriteError(
            "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
    }
            
    // Render the current ImGui frame ------------------------------------------
    glViewport(0, 0, static_cast<GLsizei>(viewport.x), static_cast<GLsizei>(viewport.y));
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return true;
}


bool GUIWindows::OnKey(core::view::Key key, core::view::KeyAction action, core::view::Modifiers mods) {

    ImGui::SetCurrentContext(this->context);

    ImGuiIO& io = ImGui::GetIO();

    bool hotkeyPressed = false;

    bool last_return_key = io.KeysDown[static_cast<size_t>(core::view::Key::KEY_ENTER)];
    bool last_num_enter_key = io.KeysDown[static_cast<size_t>(core::view::Key::KEY_KP_ENTER)];

    auto keyIndex = static_cast<size_t>(key);
    switch (action) {
    case core::view::KeyAction::PRESS:
        io.KeysDown[keyIndex] = true;
        break;
    case core::view::KeyAction::RELEASE:
        io.KeysDown[keyIndex] = false;
        break;
    default:
        break;
    }
    io.KeyCtrl = mods.test(core::view::Modifier::CTRL);
    io.KeyShift = mods.test(core::view::Modifier::SHIFT);
    io.KeyAlt = mods.test(core::view::Modifier::ALT);

    // Pass NUM 'Enter' as alternative for 'Return' to ImGui
    bool cur_return_key = ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_ENTER));
    bool cur_num_enter_key = ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_KP_ENTER));
    bool return_pressed = (!last_return_key && cur_return_key);
    bool enter_pressed = (!last_num_enter_key && cur_num_enter_key);
    io.KeysDown[static_cast<size_t>(core::view::Key::KEY_ENTER)] = (return_pressed || enter_pressed);

    // Check for GUIWindows specific hotkeys
    for (auto& h : this->hotkeys) {
        auto key = std::get<0>(h).key;
        auto mods = std::get<0>(h).mods;
        if (ImGui::IsKeyDown(static_cast<int>(key)) && (mods.test(core::view::Modifier::CTRL) == io.KeyCtrl) &&
            (mods.test(core::view::Modifier::ALT) == io.KeyAlt) &&
            (mods.test(core::view::Modifier::SHIFT) == io.KeyShift)) {
            std::get<1>(h) = true;
            hotkeyPressed = true;
        }
    }
    if (hotkeyPressed) return true;

    // Check for additional text modification hotkeys
    if (action == core::view::KeyAction::RELEASE) {
        io.KeysDown[static_cast<size_t>(GuiTextModHotkeys::CTRL_A)] = false;
        io.KeysDown[static_cast<size_t>(GuiTextModHotkeys::CTRL_C)] = false;
        io.KeysDown[static_cast<size_t>(GuiTextModHotkeys::CTRL_V)] = false;
        io.KeysDown[static_cast<size_t>(GuiTextModHotkeys::CTRL_X)] = false;
        io.KeysDown[static_cast<size_t>(GuiTextModHotkeys::CTRL_Y)] = false;
        io.KeysDown[static_cast<size_t>(GuiTextModHotkeys::CTRL_Z)] = false;
    }
    hotkeyPressed = true;
    if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_A))) {
        keyIndex = static_cast<size_t>(GuiTextModHotkeys::CTRL_A);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_C))) {
        keyIndex = static_cast<size_t>(GuiTextModHotkeys::CTRL_C);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_V))) {
        keyIndex = static_cast<size_t>(GuiTextModHotkeys::CTRL_V);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_X))) {
        keyIndex = static_cast<size_t>(GuiTextModHotkeys::CTRL_X);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_Y))) {
        keyIndex = static_cast<size_t>(GuiTextModHotkeys::CTRL_Y);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_Z))) {
        keyIndex = static_cast<size_t>(GuiTextModHotkeys::CTRL_Z);
    } else {
        hotkeyPressed = false;
    }
    if (hotkeyPressed && (action == core::view::KeyAction::PRESS)) {
        io.KeysDown[keyIndex] = true;
        return true;
    }

    // Hotkeys for showing/hiding window(s)
    hotkeyPressed = false;
    const auto func = [&](const std::string& wn, WindowManager::WindowConfiguration& wc) {
        bool windowHotkeyPressed = this->hotkeyPressed(wc.win_hotkey);
        if (windowHotkeyPressed) {
            wc.win_show = !wc.win_show;
        }
        hotkeyPressed = (hotkeyPressed || windowHotkeyPressed);

        auto window_hotkey = wc.win_hotkey;
        auto mods = window_hotkey.mods;
        mods |= megamol::core::view::Modifier::SHIFT;
        window_hotkey = megamol::core::view::KeyCode(window_hotkey.key, mods);
        windowHotkeyPressed = this->hotkeyPressed(window_hotkey);
        if (windowHotkeyPressed) {
            wc.win_soft_reset = true;
        }
        hotkeyPressed = (hotkeyPressed || windowHotkeyPressed);
    };
    this->window_manager.EnumWindows(func);
    if (hotkeyPressed) return true;

    // Check for configurator hotkeys
    if (this->configurator.CheckHotkeys()) {
        return true;
    }

    // Always consume keyboard input if requested by any imgui widget (e.g. text input).
    // User expects hotkey priority of text input thus needs to be processed before parameter hotkeys.
    if (io.WantCaptureKeyboard) {
        return true;
    }

    // Check for parameter hotkeys
    hotkeyPressed = false;
    std::vector<std::string> modules_list;
    const auto modfunc = [&](const std::string& wn, WindowManager::WindowConfiguration& wc) {
        for (auto& m : wc.param_modules_list) {
            modules_list.emplace_back(m);
        }
    };
    this->window_manager.EnumWindows(modfunc);
    const core::Module* current_mod = nullptr;
    bool consider_module = false;
    if (this->core_instance != nullptr) {
        this->core_instance->EnumParameters([&, this](const auto& mod, auto& slot) {
            if (current_mod != &mod) {
                current_mod = &mod;
                consider_module = this->considerModule(mod.FullName().PeekBuffer(), modules_list);
            }

            if (consider_module) {
                auto parameter = slot.Parameter();
                if (!parameter.IsNull()) {
                    if (auto* p = slot.template Param<core::param::ButtonParam>()) {
                        auto keyCode = p->GetKeyCode();

                        // Break loop after first occurrence of parameter hotkey
                        if (hotkeyPressed) return;

                        if (this->hotkeyPressed(keyCode)) {
                            p->setDirty();
                        }
                    }
                }
            }
        });
    } else {
        vislib::sys::Log::DefaultLog.WriteError(
            "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return hotkeyPressed;
}


bool GUIWindows::OnChar(unsigned int codePoint) {
    ImGui::SetCurrentContext(this->context);

    ImGuiIO& io = ImGui::GetIO();
    io.ClearInputCharacters();
    if (codePoint > 0 && codePoint < 0x10000) {
        io.AddInputCharacter((unsigned short)codePoint);
    }

    return false;
}


bool GUIWindows::OnMouseMove(double x, double y) {
    ImGui::SetCurrentContext(this->context);

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(static_cast<float>(x), static_cast<float>(y));

    auto hoverFlags = ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled |
                      ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;

    // Always consumed if any imgui windows is hovered.
    bool consumed = ImGui::IsWindowHovered(hoverFlags);
    return consumed;
}


bool GUIWindows::OnMouseButton(
    core::view::MouseButton button, core::view::MouseButtonAction action, core::view::Modifiers mods) {

    ImGui::SetCurrentContext(this->context);

    bool down = (action == core::view::MouseButtonAction::PRESS);
    auto buttonIndex = static_cast<size_t>(button);
    ImGuiIO& io = ImGui::GetIO();

    auto hoverFlags = ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled |
                      ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;

    // Trigger saving state when mouse hoverd any window and on button mouse release event
    if ((!down) && (io.MouseDown[buttonIndex]) && hoverFlags) {
        this->state.win_save_state = true;
        this->state.win_save_delay = 0.0f;
    }

    io.MouseDown[buttonIndex] = down;

    // Always consumed if any imgui windows is hovered.
    bool consumed = ImGui::IsWindowHovered(hoverFlags);
    return consumed;
}


bool GUIWindows::OnMouseScroll(double dx, double dy) {
    ImGui::SetCurrentContext(this->context);

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)dx;
    io.MouseWheel += (float)dy;

    auto hoverFlags = ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled |
                      ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem;

    // Always consumed if any imgui windows is hovered.
    bool consumed = ImGui::IsWindowHovered(hoverFlags);
    return consumed;
}


bool GUIWindows::createContext(void) {

    // Create ImGui context ---------------------------------------------------
    // Check for existing context and share FontAtlas with new context (required by ImGui).
    bool other_context = (ImGui::GetCurrentContext() != nullptr);
    ImFontAtlas* current_fonts = nullptr;
    if (other_context) {
        ImGuiIO& current_io = ImGui::GetIO();
        current_fonts = current_io.Fonts;
    }
    IMGUI_CHECKVERSION();
    this->context = ImGui::CreateContext(current_fonts);
    if (this->context == nullptr) {
        vislib::sys::Log::DefaultLog.WriteError(
            "Unable to create ImGui context. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }
    ImGui::SetCurrentContext(this->context);

    // Register window callbacks in window manager ----------------------------
    this->window_manager.RegisterDrawWindowCallback(
        WindowManager::DrawCallbacks::MAIN, [&, this](const std::string& wn, WindowManager::WindowConfiguration& wc) {
            this->drawMainWindowCallback(wn, wc);
        });
    this->window_manager.RegisterDrawWindowCallback(
        WindowManager::DrawCallbacks::PARAM, [&, this](const std::string& wn, WindowManager::WindowConfiguration& wc) {
            this->drawParametersCallback(wn, wc);
        });
    this->window_manager.RegisterDrawWindowCallback(
        WindowManager::DrawCallbacks::FPSMS, [&, this](const std::string& wn, WindowManager::WindowConfiguration& wc) {
            this->drawFpsWindowCallback(wn, wc);
        });
    this->window_manager.RegisterDrawWindowCallback(
        WindowManager::DrawCallbacks::FONT, [&, this](const std::string& wn, WindowManager::WindowConfiguration& wc) {
            this->drawFontWindowCallback(wn, wc);
        });
    this->window_manager.RegisterDrawWindowCallback(
        WindowManager::DrawCallbacks::TF, [&, this](const std::string& wn, WindowManager::WindowConfiguration& wc) {
            this->drawTFWindowCallback(wn, wc);
        });
    this->window_manager.RegisterDrawWindowCallback(WindowManager::DrawCallbacks::CONFIGURATOR,
        [&, this](
            const std::string& wn, WindowManager::WindowConfiguration& wc) { this->drawConfiguratorCallback(wn, wc); });

    // Create window configurations
    WindowManager::WindowConfiguration buf_win;
    buf_win.win_reset = true;
    // MAIN Window ------------------------------------------------------------
    buf_win.win_show = true;
    buf_win.win_hotkey = core::view::KeyCode(core::view::Key::KEY_F12);
    buf_win.win_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoTitleBar;
    buf_win.win_callback = WindowManager::DrawCallbacks::MAIN;
    buf_win.win_position = ImVec2(0.0f, 0.0f);
    buf_win.win_size = ImVec2(400.0f, 600.0f);
    buf_win.win_reset_size = buf_win.win_size;
    this->window_manager.AddWindowConfiguration("Main Window", buf_win);

    // FPS/MS Window ----------------------------------------------------------
    buf_win.win_show = false;
    buf_win.win_hotkey = core::view::KeyCode(core::view::Key::KEY_F11);
    buf_win.win_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
    buf_win.win_callback = WindowManager::DrawCallbacks::FPSMS;
    // buf_win.win_size = autoresize
    this->window_manager.AddWindowConfiguration("Performance Metrics", buf_win);

    // FONT Window ------------------------------------------------------------
    buf_win.win_show = false;
    buf_win.win_hotkey = core::view::KeyCode(core::view::Key::KEY_F10);
    buf_win.win_flags = ImGuiWindowFlags_AlwaysAutoResize;
    buf_win.win_callback = WindowManager::DrawCallbacks::FONT;
    // buf_win.win_size = autoresize
    this->window_manager.AddWindowConfiguration("Font Settings", buf_win);

    // TRANSFER FUNCTION Window -----------------------------------------------
    buf_win.win_show = false;
    buf_win.win_hotkey = core::view::KeyCode(core::view::Key::KEY_F9);
    buf_win.win_flags = ImGuiWindowFlags_AlwaysAutoResize;
    buf_win.win_callback = WindowManager::DrawCallbacks::TF;
    // buf_win.win_size = autoresize
    this->window_manager.AddWindowConfiguration("Transfer Function Editor", buf_win);

    // CONFIGURATOR Window -----------------------------------------------
    buf_win.win_show = false;
    // State of configurator should not be stored (visibility is configured via auto load parameter and will always be i
    // viewport size).
    buf_win.win_store_config = false;
    buf_win.win_hotkey = core::view::KeyCode(core::view::Key::KEY_F8);
    buf_win.win_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    buf_win.win_callback = WindowManager::DrawCallbacks::CONFIGURATOR;
    // buf_win.win_size is set to current viewport later
    this->window_manager.AddWindowConfiguration("Configurator", buf_win);

    // Style settings ---------------------------------------------------------
    ImGui::SetColorEditOptions(ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB |
                               ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar |
                               ImGuiColorEditFlags_AlphaPreview);

    // IO settings ------------------------------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    io.IniSavingRate = 5.0f;                              //  in seconds
    io.IniFilename = nullptr;                             // "imgui.ini"; - disabled, using own window settings profile
    io.LogFilename = "imgui_log.txt";                     // (set to nullptr to disable)
    io.FontAllowUserScaling = false;                      // disable font scaling using ctrl + mouse wheel
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // allow keyboard navigation

    // Init global state -------------------------------------------------------
    this->state.font_file = "";
    this->state.font_size = 13.0f;
    this->state.font_index = GUI_INVALID_ID;
    this->state.win_save_state = false;
    this->state.win_save_delay = 0.0f;
    this->state.win_delete = "";
    this->state.last_instance_time = 0.0f;
    this->state.params_expert = false;
    this->state.hotkeys_check_once = true;
    // Adding additional utf-8 glyph ranges
    // (there is no error if glyph has no representation in font atlas)
    this->state.font_utf8_ranges.clear();
    this->state.font_utf8_ranges.emplace_back(0x0020);
    this->state.font_utf8_ranges.emplace_back(0x03FF); // Basic Latin + Latin Supplement + Greek Alphabet
    this->state.font_utf8_ranges.emplace_back(0x20AC);
    this->state.font_utf8_ranges.emplace_back(0x20AC); // Euro
    this->state.font_utf8_ranges.emplace_back(0x2122);
    this->state.font_utf8_ranges.emplace_back(0x2122); // TM
    this->state.font_utf8_ranges.emplace_back(0x212B);
    this->state.font_utf8_ranges.emplace_back(0x212B); // Angstroem
    this->state.font_utf8_ranges.emplace_back(0x0391);
    this->state.font_utf8_ranges.emplace_back(0); // (range termination)

    // Load initial fonts only once for all imgui contexts --------------------
    if (!other_context) {
        const float default_font_size = 12.0f;
        ImFontConfig config;
        config.OversampleH = 4;
        config.OversampleV = 4;
        config.GlyphRanges = this->state.font_utf8_ranges.data();
        std::string configurator_font;
        std::string default_font;
        // Add other known fonts
        std::vector<std::string> font_paths;
        if (this->core_instance != nullptr) {
            const vislib::Array<vislib::StringW>& search_paths =
                this->core_instance->Configuration().ResourceDirectories();
            for (size_t i = 0; i < search_paths.Count(); ++i) {
                std::wstring search_path(search_paths[i].PeekBuffer());
                std::string font_path =
                    FileUtils::SearchFileRecursive<std::wstring, std::string>(search_path, "Roboto-Regular.ttf");
                if (!font_path.empty()) {
                    font_paths.emplace_back(font_path);
                    configurator_font = font_path;
                    default_font = font_path;
                }
                font_path =
                    FileUtils::SearchFileRecursive<std::wstring, std::string>(search_path, "SourceCodePro-Regular.ttf");
                if (!font_path.empty()) {
                    font_paths.emplace_back(font_path);
                }
            }
        } else {
            vislib::sys::Log::DefaultLog.WriteError(
                "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        }
        // Configurator Graph Font: Add default font at first n indices for exclusive use in configurator graph.
        /// Workaround: Using different font sizes for different graph zooming factors to improve font readability when zooming.
        const auto graph_font_scalings = this->configurator.GetGraphFontScalings();
        this->graph_fonts_reserved = graph_font_scalings.size();
        if (configurator_font.empty()) {
            for (unsigned int i = 0; i < this->graph_fonts_reserved; i++) {
                io.Fonts->AddFontDefault(&config);
            }
        } else {
            for (unsigned int i = 0; i < this->graph_fonts_reserved; i++) {
                io.Fonts->AddFontFromFileTTF(
                    configurator_font.c_str(), default_font_size * graph_font_scalings[i], &config);
            }
        }
        // Add other fonts for gui.
        io.Fonts->AddFontDefault(&config);
        io.FontDefault = io.Fonts->Fonts[(io.Fonts->Fonts.Size - 1)];
        for (auto& font_path : font_paths) {
            io.Fonts->AddFontFromFileTTF(font_path.c_str(), default_font_size, &config);
            if (default_font == font_path) {
                io.FontDefault = io.Fonts->Fonts[(io.Fonts->Fonts.Size - 1)];
            }
        }
    }

    // ImGui Key Map
    io.KeyMap[ImGuiKey_Tab] = static_cast<int>(core::view::Key::KEY_TAB);
    io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(core::view::Key::KEY_LEFT);
    io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(core::view::Key::KEY_RIGHT);
    io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(core::view::Key::KEY_UP);
    io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(core::view::Key::KEY_DOWN);
    io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(core::view::Key::KEY_PAGE_UP);
    io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(core::view::Key::KEY_PAGE_DOWN);
    io.KeyMap[ImGuiKey_Home] = static_cast<int>(core::view::Key::KEY_HOME);
    io.KeyMap[ImGuiKey_End] = static_cast<int>(core::view::Key::KEY_END);
    io.KeyMap[ImGuiKey_Insert] = static_cast<int>(core::view::Key::KEY_INSERT);
    io.KeyMap[ImGuiKey_Delete] = static_cast<int>(core::view::Key::KEY_DELETE);
    io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(core::view::Key::KEY_BACKSPACE);
    io.KeyMap[ImGuiKey_Space] = static_cast<int>(core::view::Key::KEY_SPACE);
    io.KeyMap[ImGuiKey_Enter] = static_cast<int>(core::view::Key::KEY_ENTER);
    io.KeyMap[ImGuiKey_Escape] = static_cast<int>(core::view::Key::KEY_ESCAPE);
    io.KeyMap[ImGuiKey_A] = static_cast<int>(GuiTextModHotkeys::CTRL_A);
    io.KeyMap[ImGuiKey_C] = static_cast<int>(GuiTextModHotkeys::CTRL_C);
    io.KeyMap[ImGuiKey_V] = static_cast<int>(GuiTextModHotkeys::CTRL_V);
    io.KeyMap[ImGuiKey_X] = static_cast<int>(GuiTextModHotkeys::CTRL_X);
    io.KeyMap[ImGuiKey_Y] = static_cast<int>(GuiTextModHotkeys::CTRL_Y);
    io.KeyMap[ImGuiKey_Z] = static_cast<int>(GuiTextModHotkeys::CTRL_Z);

    return true;
}


bool GUIWindows::destroyContext(void) {

    this->core_instance = nullptr;

    if (this->impl != Implementation::NONE) {
        if (this->context != nullptr) {
            switch (this->impl) {
            case (Implementation::OpenGL):
                ImGui_ImplOpenGL3_Shutdown();
                break;
            default:
                break;
            }
            ImGui::DestroyContext(this->context);
        }
    }
    return true;
}


void GUIWindows::validateParameter() {
    if (this->style_param.IsDirty()) {
        auto style = static_cast<Styles>(this->style_param.Param<core::param::EnumParam>()->Value());
        switch (style) {
        case Styles::CorporateGray:
            CorporateGreyStyle();
            break;
        case Styles::CorporateWhite:
            CorporateWhiteStyle();
            break;
        case Styles::DarkColors:
            ImGui::StyleColorsDark();
            break;
        case Styles::LightColors:
            ImGui::StyleColorsLight();
            break;
        }
        this->style_param.ResetDirty();
    }

    ImGuiIO& io = ImGui::GetIO();
    this->state.win_save_delay += io.DeltaTime;
    if (this->state_param.IsDirty()) {
        std::string state = std::string(this->state_param.Param<core::param::StringParam>()->Value().PeekBuffer());
        this->window_manager.StateFromJSON(state);
        this->parameters_gui_state_from_json(state);
        this->state_param.ResetDirty();
    } else if (this->state.win_save_state &&
               (this->state.win_save_delay > 2.0f)) { // Delayed saving after triggering saving state (in seconds).
        this->save_state_to_parameter();
        this->state.win_save_state = false;
    }

    if (this->autostart_configurator.IsDirty()) {
        bool autostart = this->autostart_configurator.Param<core::param::BoolParam>()->Value();
        if (autostart) {
            const auto configurator_func = [](const std::string& wn, WindowManager::WindowConfiguration& wc) {
                if (wc.win_callback != WindowManager::DrawCallbacks::CONFIGURATOR) {
                    wc.win_show = false;
                } else {
                    wc.win_show = true;
                }
            };
            this->window_manager.EnumWindows(configurator_func);
        }
        this->autostart_configurator.ResetDirty();
    }
}


void GUIWindows::drawMainWindowCallback(const std::string& wn, WindowManager::WindowConfiguration& wc) {

    // Menu -------------------------------------------------------------------
    /// Requires window flag ImGuiWindowFlags_MenuBar
    if (ImGui::BeginMenuBar()) {
        this->drawMenu(wn, wc);
        ImGui::EndMenuBar();
    }

    // Parameters -------------------------------------------------------------
    ImGui::TextUnformatted("Parameters");
    std::string color_param_help = "[Hover] Show Parameter Description Tooltip\n"
                                   "[Right-Click] Context Menu\n"
                                   "[Drag & Drop] Move Module to other Parameter Window\n"
                                   "[Enter],[Tab],[Left-Click outside Widget] Confirm input changes";
    this->utils.HelpMarkerToolTip(color_param_help);
    ImGui::Separator();

    this->drawParametersCallback(wn, wc);
}


void GUIWindows::drawTFWindowCallback(const std::string& wn, WindowManager::WindowConfiguration& wc) {

    this->tf_editor.DrawTransferFunctionEditor();
}


void GUIWindows::drawConfiguratorCallback(const std::string& wn, WindowManager::WindowConfiguration& wc) {

    this->configurator.Draw(wc, this->core_instance);
}


void GUIWindows::drawParametersCallback(const std::string& wn, WindowManager::WindowConfiguration& wc) {

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f); // set general proportional item width

    // Options
    ImGuiID overrideState = GUI_INVALID_ID; 
    if (ImGui::Button("Expand All")) {
        overrideState = 1; // open
    }

    ImGui::SameLine();

    if (ImGui::Button("Collapse All")) {
        overrideState = 0; // close
    }

    // Mode
    ImGui::BeginGroup();
    this->utils.PointCircleButton("Mode");
    if (ImGui::BeginPopupContextItem("gui_param_mode_button_context", 0)) { // 0 = left mouse button
        if (ImGui::MenuItem("Basic", nullptr, (this->state.params_expert == false))) {
            this->state.params_expert = false;
        }
        if (ImGui::MenuItem("Expert", nullptr, (this->state.params_expert == true))) {
            this->state.params_expert = true;
        }
        ImGui::EndPopup();
    }
    ImGui::EndGroup();
    std::string mode_help = "Expert mode enables buttons for additional parameter presentation options.\n"
        "Note: Changes are NOT (yet) stored in project files.";
    this->utils.HelpMarkerToolTip(mode_help);

    ImGui::SameLine();

    // Toggel Hotkeys
    bool show_only_hotkeys = wc.param_show_hotkeys;
    ImGui::Checkbox("Show Hotkeys", &show_only_hotkeys);
    wc.param_show_hotkeys = show_only_hotkeys;

    // Paramter substring name filtering (only for main parameter view)
    if (wc.win_callback == WindowManager::DrawCallbacks::MAIN) {
        if (std::get<1>(this->hotkeys[GUIWindows::GuiHotkeyIndex::PARAMETER_SEARCH])) {
            this->utils.SetSearchFocus(true);
            std::get<1>(this->hotkeys[GUIWindows::GuiHotkeyIndex::PARAMETER_SEARCH]) = false;
        }
        std::string help_test =
            "[" + std::get<0>(this->hotkeys[GUIWindows::GuiHotkeyIndex::PARAMETER_SEARCH]).ToString() +
            "] Set keyboard focus to search input field.\n"
            "Case insensitive substring search in\nparameter names.\nGlobally in all parameter views.\n";
        this->utils.StringSearch("guiwindow_parameter_earch", help_test);
    }

    // Module filtering (only for main parameter view)
    if ((this->core_instance != nullptr) && (wc.win_callback == WindowManager::DrawCallbacks::MAIN)) {
        std::map<int, std::string> opts;
        opts[static_cast<int>(WindowManager::FilterModes::ALL)] = "All";
        opts[static_cast<int>(WindowManager::FilterModes::INSTANCE)] = "Instance";
        opts[static_cast<int>(WindowManager::FilterModes::VIEW)] = "View";
        unsigned int opts_cnt = (unsigned int)opts.size();
        if (ImGui::BeginCombo("Filter Modules", opts[(int)wc.param_module_filter].c_str())) {
            for (unsigned int i = 0; i < opts_cnt; ++i) {
                if (ImGui::Selectable(opts[i].c_str(), (static_cast<int>(wc.param_module_filter) == i))) {
                    wc.param_module_filter = static_cast<WindowManager::FilterModes>(i);
                    wc.param_modules_list.clear();
                    if ((wc.param_module_filter == WindowManager::FilterModes::INSTANCE) ||
                        (wc.param_module_filter == WindowManager::FilterModes::VIEW)) {
                        std::string viewname;
                        // The goal is to find view module with shortest call connection path to this module.
                        // Since enumeration of modules goes bottom up, result for first abstract view is
                        // stored and following hits are ignored.
                        if (!this->parent_module_fullname.empty()) {
                            const auto view_func = [&, this](core::Module* viewmod) {
                                auto v = dynamic_cast<core::view::AbstractView*>(viewmod);
                                if (v != nullptr) {
                                    std::string vname = v->FullName().PeekBuffer();
                                    bool found = false;
                                    const auto find_func = [&, this](core::Module* guimod) {
                                        std::string modname = guimod->FullName().PeekBuffer();
                                        if (this->parent_module_fullname == modname) {
                                            found = true;
                                        }
                                    };
                                    this->core_instance->EnumModulesNoLock(viewmod, find_func);
                                    if (found && viewname.empty()) {
                                        viewname = vname;
                                    }
                                }
                            };
                            this->core_instance->EnumModulesNoLock(nullptr, view_func);                            
                        }
                        if (!viewname.empty()) {
                            if (wc.param_module_filter == WindowManager::FilterModes::INSTANCE) {
                                // Considering modules depending on the INSTANCE NAME of the first view this module is connected to. 
                                std::string instname = ""; 
                                auto instance_idx = viewname.rfind("::");
                                if (instance_idx != std::string::npos) { 
                                    instname = viewname.substr(0, instance_idx+2);
                                }
                                if (!instname.empty()) { // Consider all modules if view is not assigned to any instance
                                    const auto func = [&, this](core::Module* mod) {
                                        std::string modname = mod->FullName().PeekBuffer();
                                        bool foundInstanceName = (modname.find(instname) != std::string::npos);
                                        bool noInstanceNamePresent = false; /// Always consider modules with no namspace (modname.find("::", 2) == std::string::npos);
                                        if (foundInstanceName || noInstanceNamePresent) {
                                            wc.param_modules_list.emplace_back(modname);
                                        }
                                    };
                                    this->core_instance->EnumModulesNoLock(nullptr, func);
                                }
                            } else { // (wc.param_module_filter == WindowManager::FilterModes::VIEW)
                                // Considering modules depending on their connection to the VIEW MODULE this GUI is incorporated.
                                const auto add_func = [&, this](core::Module* mod) { 
                                    std::string modname = mod->FullName().PeekBuffer(); 
                                    wc.param_modules_list.emplace_back(modname);
                                };
                                this->core_instance->EnumModulesNoLock(viewname, add_func);
                            }
                        } else {
                            vislib::sys::Log::DefaultLog.WriteWarn(
                                "Could not find abstract view "
                                "module this gui is connected to. [%s, %s, line %d]\n",
                                __FILE__, __FUNCTION__, __LINE__);
                        }
                    }
                }
                std::string hover = "Show all Modules."; // == WindowManager::FilterModes::ALL
                if (i == static_cast<int>(WindowManager::FilterModes::INSTANCE)) {
                    hover = "Show Modules with same Instance Name as current View and Modules with no Instance Name.";
                } else if (i == static_cast<int>(WindowManager::FilterModes::VIEW)) {
                    hover = "Show Modules subsequently connected to the View Module the Gui Module is connected to.";
                }
                this->utils.HoverToolTip(hover);
            }
            ImGui::EndCombo();
        }
        this->utils.HelpMarkerToolTip("Selected filter is not refreshed on graph changes.\n"
                                      "Select filter again to trigger refresh.");
    }

    ImGui::Separator();

    // Create child window for sepearte scroll bar and keeping header always visible on top of parameter list
    ImGui::BeginChild("###ParameterList");

    // Listing modules and their parameters
    const core::Module* current_mod = nullptr;
    bool current_mod_open = false;
    const size_t dnd_size = 2048; // Set same max size of all module labels for drag and drop.
    std::string param_namespace = "";
    unsigned int param_indent_stack = 0;
    bool param_namespace_open = true;

    if (this->core_instance != nullptr) {
        this->core_instance->EnumParameters([&, this](const auto& mod, auto& slot) {
            auto currentSearchString = this->utils.GetSearchString();

            // Check for new module
            if (current_mod != &mod) {
                current_mod = &mod;
                std::string label = mod.FullName().PeekBuffer();
                
                if (current_mod_open) {
                    // Vertical spacing 
                    ImGui::Dummy(ImVec2(1.0f, ImGui::GetFrameHeightWithSpacing()));
                }
                                
                // Reset parameter namespace stuff
                param_namespace = "";
                param_namespace_open = true;
                while (param_indent_stack > 0) {
                    param_indent_stack--;
                    ImGui::Unindent();
                }

                // Check if module should be considered.
                if (!this->considerModule(label, wc.param_modules_list)) {
                    current_mod_open = false;
                    return;
                }

                // Determine header state and change color depending on active parameter search
                auto headerId = ImGui::GetID(label.c_str());
                auto headerState = overrideState;
                if (headerState == GUI_INVALID_ID) {
                    headerState = ImGui::GetStateStorage()->GetInt(headerId, 0); // 0=close 1=open
                }
                if (!currentSearchString.empty()) {
                    headerState = 1;
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_PopupBg));
                }
                ImGui::GetStateStorage()->SetInt(headerId, headerState);

                current_mod_open = ImGui::CollapsingHeader(label.c_str(), nullptr);
                if (!currentSearchString.empty()) {
                    ImGui::PopStyleColor();
                }

                // Module description as hover tooltip
                auto mod_desc = this->core_instance->GetModuleDescriptionManager().Find(mod.ClassName());
                if (mod_desc != nullptr) {
                    this->utils.HoverToolTip(
                        std::string(mod_desc->Description()), ImGui::GetID(label.c_str()), 0.5f, 5.0f);
                }

                // Context menu
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Copy to new Window")) {
                        // using instance time as hidden unique id
                        std::string window_name =
                            "Parameters###parameters" + std::to_string(this->state.last_instance_time);

                        WindowManager::WindowConfiguration buf_win;
                        buf_win.win_show = true;
                        buf_win.win_flags = ImGuiWindowFlags_HorizontalScrollbar;
                        buf_win.win_callback = WindowManager::DrawCallbacks::PARAM;
                        buf_win.param_show_hotkeys = false;
                        buf_win.param_modules_list.emplace_back(label);
                        this->window_manager.AddWindowConfiguration(window_name, buf_win);
                    }

                    // Deleting module's parameters is not available in main parameter window.
                    if (wc.win_callback != WindowManager::DrawCallbacks::MAIN) {
                        if (ImGui::MenuItem("Delete from List")) {
                            std::vector<std::string>::iterator find_iter =
                                std::find(wc.param_modules_list.begin(), wc.param_modules_list.end(), label);
                            // Break if module name is not contained in list
                            if (find_iter != wc.param_modules_list.end()) {
                                wc.param_modules_list.erase(find_iter);
                            }
                        }
                    }
                    ImGui::EndPopup();
                }


                // Drag source
                label.resize(dnd_size);
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    ImGui::SetDragDropPayload(
                        "DND_COPY_MODULE_PARAMETERS", label.c_str(), (label.size() * sizeof(char)));
                    ImGui::TextUnformatted(label.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            if (current_mod_open) {
                auto parameter = slot.Parameter();
                std::string param_name = slot.Name().PeekBuffer();
                bool showSearchedParameter = true;
                if (!currentSearchString.empty()) {
                    showSearchedParameter = this->utils.FindCaseInsensitiveSubstring(param_name, currentSearchString);
                }

                bool param_visible = ((parameter->IsGUIVisible() || this->state.params_expert) && showSearchedParameter);
                if (!parameter.IsNull() && param_visible) {

                    // Parameter namespace header
                    auto pos = param_name.find("::");
                    std::string current_param_namespace = "";
                    if (pos != std::string::npos) {
                        current_param_namespace = param_name.substr(0, pos);
                    }
                    if (current_param_namespace != param_namespace) {
                        param_namespace = current_param_namespace;
                        while (param_indent_stack > 0) {
                            param_indent_stack--;
                            ImGui::Unindent();
                        }
                        ///ImGui::Separator();
                        if (!param_namespace.empty()) {
                            ImGui::Indent();
                            std::string label = param_namespace + "###" + param_namespace + "__" + param_name;
                            if (!currentSearchString.empty()) {
                                auto headerId = ImGui::GetID(label.c_str());
                                ImGui::GetStateStorage()->SetInt(headerId, 1);
                            }
                            param_namespace_open =
                                ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                            param_indent_stack++;
                        } else {
                            param_namespace_open = true;
                        }
                    }

                    // Draw parameter
                    if (param_namespace_open) {
                        if (wc.param_show_hotkeys) {
                            this->drawParameterHotkey(mod, slot);
                        } else {
                            this->drawParameter(mod, slot);
                        }
                    }
                }
            }
        });
    } else {
        vislib::sys::Log::DefaultLog.WriteError(
            "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
    }


    // Reset parameter namespace stuff
    while (param_indent_stack > 0) {
        param_indent_stack--;
        ImGui::Unindent();
    }

    // Drop target
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize()));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_COPY_MODULE_PARAMETERS")) {

            IM_ASSERT(payload->DataSize == (dnd_size * sizeof(char)));
            std::string payload_id = (const char*)payload->Data;

            // Insert dragged module name only if not contained in list
            if (!this->considerModule(payload_id, wc.param_modules_list)) {
                wc.param_modules_list.emplace_back(payload_id);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();

    ImGui::PopItemWidth();
}


void GUIWindows::drawFpsWindowCallback(const std::string& wn, WindowManager::WindowConfiguration& wc) {
    ImGuiIO& io = ImGui::GetIO();

    // Leave some space in histogram for text of current value
    wc.buf_current_delay += io.DeltaTime;
    int buffer_size = static_cast<int>(wc.buf_values.size());
    if (wc.ms_refresh_rate > 0.0f) {
        if (wc.buf_current_delay >= (1.0f / wc.ms_refresh_rate)) {
            if (buffer_size != wc.ms_max_history_count) {
                if (buffer_size > wc.ms_max_history_count) {
                    wc.buf_values.erase(
                        wc.buf_values.begin(), wc.buf_values.begin() + (buffer_size - wc.ms_max_history_count));

                } else if (buffer_size < wc.ms_max_history_count) {
                    wc.buf_values.insert(wc.buf_values.begin(), (wc.ms_max_history_count - buffer_size), 0.0f);
                }
            }
            if (buffer_size > 0) {
                wc.buf_values.erase(wc.buf_values.begin());
                wc.buf_values.emplace_back(io.DeltaTime * 1000.0f); // scale to milliseconds

                float max_fps = 0.0f;
                float max_ms = 0.0f;
                for (auto& v : wc.buf_values) {
                    if (v > 0.0f) {
                        max_fps = ((1.0f / v * 1000.f) > max_fps) ? (1.0f / v * 1000.f) : (max_fps);
                    }
                    max_ms = (v > max_ms) ? (v) : (max_ms);
                }

                wc.buf_plot_fps_scaling = max_fps;
                wc.buf_plot_ms_scaling = max_ms;
            }
            wc.buf_current_delay = 0.0f;
        }
    }

    // Draw window content
    if (ImGui::RadioButton("fps", (wc.ms_mode == WindowManager::TimingModes::FPS))) {
        wc.ms_mode = WindowManager::TimingModes::FPS;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("ms", (wc.ms_mode == WindowManager::TimingModes::MS))) {
        wc.ms_mode = WindowManager::TimingModes::MS;
    }

    ImGui::SameLine(0.0f, 50.0f);
    ImGui::Checkbox("Options", &wc.ms_show_options);

    std::vector<float> value_array = wc.buf_values;
    if (wc.ms_mode == WindowManager::TimingModes::FPS) {
        for (auto& v : value_array) {
            v = (v > 0.0f) ? (1.0f / v * 1000.f) : (0.0f);
        }
    }
    float* value_ptr = (&value_array)->data();

    std::string overlay;
    if (buffer_size > 0) {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(3) << value_array.back();
        overlay = stream.str();
    }

    float plot_scale_factor = 1.5f;
    if (wc.ms_mode == WindowManager::TimingModes::FPS) {
        plot_scale_factor *= wc.buf_plot_fps_scaling;
    } else if (wc.ms_mode == WindowManager::TimingModes::MS) {
        plot_scale_factor *= wc.buf_plot_ms_scaling;
    }

    ImGui::PlotLines("###msplot", value_ptr, buffer_size, 0, overlay.c_str(), 0.0f, plot_scale_factor,
        ImVec2(0.0f, 50.0f)); 

    if (wc.ms_show_options) {
        if (ImGui::InputFloat(
                "Refresh Rate", &wc.ms_refresh_rate, 1.0f, 10.0f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            wc.ms_refresh_rate = std::max(1.0f, wc.ms_refresh_rate);
        }

        if (ImGui::InputInt("History Size", &wc.ms_max_history_count, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue)) {
            wc.ms_max_history_count = std::max(1, wc.ms_max_history_count);
        }

        if (ImGui::Button("Current Value")) {
#ifdef GUI_USE_GLFW
            auto glfw_win = ::glfwGetCurrentContext();
            ::glfwSetClipboardString(glfw_win, overlay.c_str());
#elif _WIN32
            ImGui::SetClipboardText(overlay.c_str());
#else // LINUX
            vislib::sys::Log::DefaultLog.WriteWarn(
                "No clipboard use provided. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            vislib::sys::Log::DefaultLog.WriteInfo("Current Performance Monitor Value:\n%s", overlay.c_str());
#endif
        }
        ImGui::SameLine();

        if (ImGui::Button("All Values")) {
            std::stringstream stream;
            stream << std::fixed << std::setprecision(3);
            auto reverse_end = value_array.rend();
            for (std::vector<float>::reverse_iterator i = value_array.rbegin(); i != reverse_end; ++i) {
                stream << (*i) << "\n";
            }
#ifdef GUI_USE_GLFW
            auto glfw_win = ::glfwGetCurrentContext();
            ::glfwSetClipboardString(glfw_win, stream.str().c_str());
#elif _WIN32
            ImGui::SetClipboardText(stream.str().c_str());
#else // LINUX
            vislib::sys::Log::DefaultLog.WriteWarn(
                "No clipboard use provided. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            vislib::sys::Log::DefaultLog.WriteInfo("All Performance Monitor Values:\n%s", stream.str().c_str());
#endif
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Copy to Clipborad");
        std::string help = "Values are copied in chronological order (newest first)";
        this->utils.HelpMarkerToolTip(help);
    }
}


void GUIWindows::drawFontWindowCallback(const std::string& wn, WindowManager::WindowConfiguration& wc) {
    ImGuiIO& io = ImGui::GetIO();

    ImFont* font_current = ImGui::GetFont();
    if (ImGui::BeginCombo("Select available Font", font_current->GetDebugName())) {
        for (int n = 0; n < (io.Fonts->Fonts.Size - 1); n++) { // ! n < size-1 for skipping last added font which is
                                                               // exclusively used by configurator for the graph.
            if (ImGui::Selectable(io.Fonts->Fonts[n]->GetDebugName(), (io.Fonts->Fonts[n] == font_current)))
                io.FontDefault = io.Fonts->Fonts[n];
        }
        ImGui::EndCombo();
    }

    // Saving current font to window configuration.
    wc.font_name = std::string(font_current->GetDebugName());
    GUIUtils::Utf8Decode(wc.font_name);

    ImGui::Separator();
    ImGui::TextUnformatted("Load Font from File");
    std::string help = "Same font can be loaded multiple times with different font size.";
    this->utils.HelpMarkerToolTip(help);

    std::string label = "Font Size";
    ImGui::InputFloat(label.c_str(), &wc.buf_font_size, 1.0f, 10.0f, "%.2f", ImGuiInputTextFlags_None);
    // Validate font size
    if (wc.buf_font_size <= 0.0f) {
        wc.buf_font_size = 5.0f; // minimum valid font size
    }

    label = "Font File Name (.ttf)";
    /// XXX: UTF8 conversion and allocation every frame is horrific inefficient.
    GUIUtils::Utf8Encode(wc.buf_font_file);
    ImGui::InputText(label.c_str(), &wc.buf_font_file, ImGuiInputTextFlags_AutoSelectAll);
    GUIUtils::Utf8Decode(wc.buf_font_file);
    // Validate font file before offering load button
    if (FileUtils::FilesExistingExtension<std::string>(wc.buf_font_file, std::string(".ttf"))) {
        if (ImGui::Button("Add Font")) {
            this->state.font_file = wc.buf_font_file;
            this->state.font_size = wc.buf_font_size;
        }
    } else {
        ImGui::TextColored(GUI_COLOR_TEXT_ERROR, "Please enter valid font file name.");
    }
}


void GUIWindows::drawMenu(const std::string& wn, WindowManager::WindowConfiguration& wc) {

    bool open_popup_project = false;
    if (ImGui::BeginMenu("File")) {
        // Load/save parameter values to LUA file
        if (ImGui::MenuItem("Save Project",
                std::get<0>(this->hotkeys[GUIWindows::GuiHotkeyIndex::SAVE_PROJECT]).ToString().c_str())) {
            open_popup_project = true;
        }

        if (ImGui::MenuItem("Exit", "ALT + 'F4'")) {
            // Exit program
            this->shutdown();
        }
        ImGui::EndMenu();
    }

    // Windows
    if (ImGui::BeginMenu("Windows")) {
        const auto func = [&, this](const std::string& wn, WindowManager::WindowConfiguration& wc) {
            bool win_open = wc.win_show;
            std::string hotkey_label = wc.win_hotkey.ToString();
            if (!hotkey_label.empty()) {
                hotkey_label = "(SHIFT +) " + hotkey_label;
            }
            if (ImGui::MenuItem(wn.c_str(), hotkey_label.c_str(), &win_open)) {
                wc.win_show = !wc.win_show;
            }
            // Add conext menu for deleting windows without hotkey (= custom parameter windows).
            if (wc.win_hotkey.key == core::view::Key::KEY_UNKNOWN) {
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Delete Window")) {
                        this->state.win_delete = wn;
                    }
                    ImGui::EndPopup();
                }
                this->utils.HoverToolTip("[Right-Click] Open Context Menu for Deleting Window Permanently.");
            } else {
                this->utils.HoverToolTip("['Window Hotkey'] Show/Hide Window.\n[Shift]+['Window Hotkey'] Reset Size "
                                         "and Position of Window.");
            }
        };
        this->window_manager.EnumWindows(func);

        ImGui::EndMenu();
    }

    // Help
    bool open_popup_about = false;
    if (ImGui::BeginMenu("Help")) {
        // if (ImGui::MenuItem("Usability Hints")) {
        //}

        if (ImGui::MenuItem("About")) {
            open_popup_about = true;
        }
        ImGui::EndMenu();
    }

    // Popups -----------------------------------------------------------------

    // ABOUT
    if (open_popup_about) {
        ImGui::OpenPopup("About");
    }
    bool open = true;
    if (ImGui::BeginPopupModal("About", &open, (ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))) {

        const std::string eMail = "megamol@visus.uni-stuttgart.de";
        const std::string webLink = "https://megamol.org/";
        const std::string gitLink = "https://github.com/UniStuttgart-VISUS/megamol";

        std::string about = std::string("MegaMol - Version ") + std::to_string(MEGAMOL_CORE_MAJOR_VER) + (".") +
                            std::to_string(MEGAMOL_CORE_MINOR_VER) + ("\ngit# ") + std::string(MEGAMOL_CORE_COMP_REV) +
                            ("\nDear ImGui - Version ") + std::string(IMGUI_VERSION) + ("\n");
        std::string mailstr = std::string("Contact: ") + eMail;
        std::string webstr = std::string("Web: ") + webLink;
        std::string gitstr = std::string("Git-Hub: ") + gitLink;

        ImGui::TextUnformatted(about.c_str());

        ImGui::Separator();
        if (ImGui::Button("Copy E-Mail")) {
#ifdef GUI_USE_GLFW
            auto glfw_win = ::glfwGetCurrentContext();
            ::glfwSetClipboardString(glfw_win, eMail.c_str());
#elif _WIN32
            ImGui::SetClipboardText(eMail.c_str());
#else // LINUX
            vislib::sys::Log::DefaultLog.WriteWarn(
                "No clipboard use provided. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            vislib::sys::Log::DefaultLog.WriteInfo("E-Mail address:\n%s", eMail.c_str());
#endif
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(mailstr.c_str());


        if (ImGui::Button("Copy Website")) {
#ifdef GUI_USE_GLFW
            auto glfw_win = ::glfwGetCurrentContext();
            ::glfwSetClipboardString(glfw_win, webLink.c_str());
#elif _WIN32
            ImGui::SetClipboardText(webLink.c_str());
#else // LINUX
            vislib::sys::Log::DefaultLog.WriteWarn(
                "No clipboard use provided. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            vislib::sys::Log::DefaultLog.WriteInfo("Website link:\n%s", webLink.c_str());
#endif
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(webstr.c_str());

        if (ImGui::Button("Copy GitHub")) {
#ifdef GUI_USE_GLFW
            auto glfw_win = ::glfwGetCurrentContext();
            ::glfwSetClipboardString(glfw_win, gitLink.c_str());
#elif _WIN32
            ImGui::SetClipboardText(gitLink.c_str());
#else // LINUX
            vislib::sys::Log::DefaultLog.WriteWarn(
                "No clipboard use provided. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            vislib::sys::Log::DefaultLog.WriteInfo("GitHub link:\n%s", gitLink.c_str());
#endif
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(gitstr.c_str());

        ImGui::Separator();
        about = "Copyright (C) 2009-2019 by Universitaet Stuttgart "
                "(VIS).\nAll rights reserved.";
        ImGui::TextUnformatted(about.c_str());

        ImGui::Separator();
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Save project pop-up
    open_popup_project = (open_popup_project || std::get<1>(this->hotkeys[GUIWindows::GuiHotkeyIndex::SAVE_PROJECT]));
    if (open_popup_project) {
        std::get<1>(this->hotkeys[GUIWindows::GuiHotkeyIndex::SAVE_PROJECT]) = false;
    }
    if (this->file_utils.FileBrowserPopUp(
            FileUtils::FileBrowserFlag::SAVE, "Save Project", open_popup_project, wc.main_project_file)) {
        // Serialize current state to parameter.
        this->save_state_to_parameter();
        // Serialize project to file
        FileUtils::SaveProjectFile(wc.main_project_file, this->core_instance);
    }
}


void GUIWindows::drawParameter(const core::Module& mod, core::param::ParamSlot& slot) {
    
    ImGuiStyle& style = ImGui::GetStyle();
    std::string help;

    auto parameter = slot.Parameter();
    if (!parameter.IsNull()) {

        std::string param_name = slot.Name().PeekBuffer();
        std::string param_id = std::string(mod.FullName().PeekBuffer()) + "::" + param_name;
        auto pos = param_name.find("::");
        if (pos != std::string::npos) {
            param_name = param_name.substr(pos + 2);
        }
        std::string param_label_hidden = "###" + param_id;
        std::string param_label = param_name + param_label_hidden;
        std::string param_desc = slot.Description().PeekBuffer();
        std::string float_format = "%.7f";

        ImGui::BeginGroup();
        
        // Expert Options
        ImGui::PushID(param_label.c_str()); 
        if (this->state.params_expert) {
            // Visibility
            bool param_visible = parameter->IsGUIVisible();
            if (ImGui::RadioButton("###visible", param_visible)) {
                parameter->SetGUIVisible(!param_visible);
            }
            this->utils.HoverToolTip("Visibility");

            ImGui::SameLine();

            // Read-only option
            bool param_readonly = parameter->IsGUIReadOnly();
            if (ImGui::Checkbox("###readonly", &param_readonly)) {
                parameter->SetGUIReadOnly(param_readonly);
            }
            this->utils.HoverToolTip("Read-Only");
    
            ImGui::SameLine();
    
            // Presentation
            bool default_present =
                (slot.Parameter()->GetGUIPresentation() == megamol::core::param::AbstractParam::Presentations::DEFAULT);              
            this->utils.PointCircleButton("", !default_present);
            this->utils.HoverToolTip("Presentation");
            if (ImGui::BeginPopupContextItem("param_present_button_context", 0)) { // 0 = left mouse button
                for (size_t i = 0; i < megamol::core::param::AbstractParam::Presentations::__COUNT__; i++) {
                    std::string param_present_str;
                    auto param_present = static_cast<megamol::core::param::AbstractParam::Presentations>(i);
                    switch (param_present) {
                    case (megamol::core::param::AbstractParam::Presentations::DEFAULT):
                        param_present_str = "Default";
                        break;
                    case (megamol::core::param::AbstractParam::Presentations::PIN_VALUE_TO_MOUSE):
                        param_present_str = "Pin Value to Mouse";
                        break;
                    default:
                        break;
                    }
                    if (param_present_str.empty()) break;
                    if (ImGui::MenuItem(param_present_str.c_str(), nullptr,
                            (param_present == slot.Parameter()->GetGUIPresentation()))) {
                        slot.Parameter()->SetGUIPresentation(param_present);
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine();
        }
        ImGui::PopID();
        
        // Parameter
        ImGui::PushID(param_label.c_str());
        
        // Set different style if parameter is read-only
        bool readOnly = parameter->IsGUIReadOnly();
        if (readOnly) {
            GUIUtils::ReadOnlyWigetStyle(true);
        }
        
        if (auto* p = slot.template Param<core::param::BoolParam>()) {
            auto value = p->Value();
            if (ImGui::Checkbox(param_label.c_str(), &value)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.template Param<core::param::ButtonParam>()) {
            std::string hotkey = "";
            std::string buttonHotkey = p->GetKeyCode().ToString();
            if (!buttonHotkey.empty()) {
                hotkey = " (" + buttonHotkey + ")";
            }
            auto insert_pos = param_label.find("###");
            param_label.insert(insert_pos, hotkey);
            if (ImGui::Button(param_label.c_str())) {
                p->setDirty();
            }
        } else if (auto* p = slot.template Param<core::param::ColorParam>()) {
            core::param::ColorParam::ColorType value = p->Value();
            auto color_flags = ImGuiColorEditFlags_AlphaPreview; // | ImGuiColorEditFlags_Float;
            if (ImGui::ColorEdit4(param_label.c_str(), (float*)value.data(), color_flags)) {
                p->SetValue(value);
            }
            help = "[Click] on the colored square to open a color picker.\n"
                   "[CTRL+Click] on individual component to input value.\n"
                   "[Right-Click] on the individual color widget to show options.";
        } else if (auto* p = slot.template Param<core::param::TransferFunctionParam>()) {
            drawTransferFunctionEdit(param_id, param_label, *p);
        } else if (auto* p = slot.template Param<core::param::EnumParam>()) {
            /// XXX: no UTF8 fanciness required here?
            auto map = p->getMap();
            auto key = p->Value();
            if (ImGui::BeginCombo(param_label.c_str(), map[key].PeekBuffer())) {
                auto iter = map.GetConstIterator();
                while (iter.HasNext()) {
                    auto pair = iter.Next();
                    bool isSelected = (pair.Key() == key);
                    if (ImGui::Selectable(pair.Value().PeekBuffer(), isSelected)) {
                        p->SetValue(pair.Key());
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else if (auto* p = slot.template Param<core::param::FlexEnumParam>()) {
            /// XXX: no UTF8 fanciness required here?
            auto value = p->Value();
            if (ImGui::BeginCombo(param_label.c_str(), value.c_str())) {
                for (auto& valueOption : p->getStorage()) {
                    bool isSelected = (valueOption == value);
                    if (ImGui::Selectable(valueOption.c_str(), isSelected)) {
                        p->SetValue(valueOption);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else if (auto* p = slot.template Param<core::param::FloatParam>()) {
            auto it = this->widgtmap_float.find(param_id);
            if (it == this->widgtmap_float.end()) {
                this->widgtmap_float.emplace(param_id, p->Value());
                it = this->widgtmap_float.find(param_id);
            }
            ImGui::InputFloat(
                param_label.c_str(), &it->second, 1.0f, 10.0f, float_format.c_str(), ImGuiInputTextFlags_None);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                it->second = std::max(p->MinValue(), std::min(it->second, p->MaxValue()));
                p->SetValue(it->second);
            } else if (!ImGui::IsItemActive() && !ImGui::IsItemEdited()) {
                it->second = p->Value();
            }
        } else if (auto* p = slot.template Param<core::param::IntParam>()) {
            auto it = this->widgtmap_int.find(param_id);
            if (it == this->widgtmap_int.end()) {
                this->widgtmap_int.emplace(param_id, p->Value());
                it = this->widgtmap_int.find(param_id);
            }
            ImGui::InputInt(param_label.c_str(), &it->second, 1, 10, ImGuiInputTextFlags_None);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                it->second = std::max(p->MinValue(), std::min(it->second, p->MaxValue()));
                p->SetValue(it->second);
            } else if (!ImGui::IsItemActive() && !ImGui::IsItemEdited()) {
                it->second = p->Value();
            }
        } else if (auto* p = slot.template Param<core::param::Vector2fParam>()) {
            auto it = this->widgtmap_vec2.find(param_id);
            if (it == this->widgtmap_vec2.end()) {
                this->widgtmap_vec2.emplace(param_id, p->Value());
                it = this->widgtmap_vec2.find(param_id);
            }
            ImGui::InputFloat2(
                param_label.c_str(), it->second.PeekComponents(), float_format.c_str(), ImGuiInputTextFlags_None);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                auto x = std::max(p->MinValue().X(), std::min(it->second.X(), p->MaxValue().X()));
                auto y = std::max(p->MinValue().Y(), std::min(it->second.Y(), p->MaxValue().Y()));
                it->second = vislib::math::Vector<float, 2>(x, y);
                p->SetValue(it->second);
            } else if (!ImGui::IsItemActive() && !ImGui::IsItemEdited()) {
                it->second = p->Value();
            }
        } else if (auto* p = slot.template Param<core::param::Vector3fParam>()) {
            auto it = this->widgtmap_vec3.find(param_id);
            if (it == this->widgtmap_vec3.end()) {
                this->widgtmap_vec3.emplace(param_id, p->Value());
                it = this->widgtmap_vec3.find(param_id);
            }
            ImGui::InputFloat3(
                param_label.c_str(), it->second.PeekComponents(), float_format.c_str(), ImGuiInputTextFlags_None);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                auto x = std::max(p->MinValue().X(), std::min(it->second.X(), p->MaxValue().X()));
                auto y = std::max(p->MinValue().Y(), std::min(it->second.Y(), p->MaxValue().Y()));
                auto z = std::max(p->MinValue().Z(), std::min(it->second.Z(), p->MaxValue().Z()));
                it->second = vislib::math::Vector<float, 3>(x, y, z);
                p->SetValue(it->second);
            } else if (!ImGui::IsItemActive() && !ImGui::IsItemEdited()) {
                it->second = p->Value();
            }
        } else if (auto* p = slot.template Param<core::param::Vector4fParam>()) {
            auto it = this->widgtmap_vec4.find(param_id);
            if (it == this->widgtmap_vec4.end()) {
                this->widgtmap_vec4.emplace(param_id, p->Value());
                it = this->widgtmap_vec4.find(param_id);
            }
            ImGui::InputFloat4(
                param_label.c_str(), it->second.PeekComponents(), float_format.c_str(), ImGuiInputTextFlags_None);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                auto x = std::max(p->MinValue().X(), std::min(it->second.X(), p->MaxValue().X()));
                auto y = std::max(p->MinValue().Y(), std::min(it->second.Y(), p->MaxValue().Y()));
                auto z = std::max(p->MinValue().Z(), std::min(it->second.Z(), p->MaxValue().Z()));
                auto w = std::max(p->MinValue().W(), std::min(it->second.W(), p->MaxValue().W()));
                it->second = vislib::math::Vector<float, 4>(x, y, z, w);
                p->SetValue(it->second);
            } else if (!ImGui::IsItemActive() && !ImGui::IsItemEdited()) {
                it->second = p->Value();
            }
        } else if (auto* p = slot.template Param<core::param::TernaryParam>()) {
            auto value = p->Value();
            if (ImGui::RadioButton("True", value.IsTrue())) {
                p->SetValue(vislib::math::Ternary::TRI_TRUE);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("False", value.IsFalse())) {
                p->SetValue(vislib::math::Ternary::TRI_FALSE);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Unknown", value.IsUnknown())) {
                p->SetValue(vislib::math::Ternary::TRI_UNKNOWN);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextUnformatted(param_name.c_str());
        } else if (auto* p = slot.Param<core::param::StringParam>()) {
            /// XXX: UTF8 conversion and allocation every frame is horrific inefficient.
            auto it = this->widgtmap_text.find(param_id);
            if (it == this->widgtmap_text.end()) {
                std::string utf8Str = std::string(p->ValueString().PeekBuffer());
                GUIUtils::Utf8Encode(utf8Str);
                this->widgtmap_text.emplace(param_id, utf8Str);
                it = this->widgtmap_text.find(param_id);
            }
            // Determine multi line count of string
            int lcnt = static_cast<int>(std::count(it->second.begin(), it->second.end(), '\n'));
            lcnt = std::min(static_cast<int>(GUI_MAX_MULITLINE), lcnt);
            ImVec2 ml_dim = ImVec2(
                ImGui::CalcItemWidth(), ImGui::GetFrameHeight() + (ImGui::GetFontSize() * static_cast<float>(lcnt)));
            ImGui::InputTextMultiline(
                param_label_hidden.c_str(), &it->second, ml_dim, ImGuiInputTextFlags_CtrlEnterForNewLine);
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                std::string utf8Str = it->second;
                GUIUtils::Utf8Decode(utf8Str);
                p->SetValue(vislib::StringA(utf8Str.c_str()));
            } else if (!ImGui::IsItemActive() && !ImGui::IsItemEdited()) {
                std::string utf8Str = std::string(p->ValueString().PeekBuffer());
                GUIUtils::Utf8Encode(utf8Str);
                it->second = utf8Str;
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(param_name.c_str());
            help = "[Ctrl + Enter] for new line.\nPress [Return] to confirm changes.";
        } else if (auto* p = slot.Param<core::param::FilePathParam>()) {
            /// XXX: UTF8 conversion and allocation every frame is horrific inefficient.
            auto it = this->widgtmap_text.find(param_id);
            if (it == this->widgtmap_text.end()) {
                std::string utf8Str = std::string(p->ValueString().PeekBuffer());
                GUIUtils::Utf8Encode(utf8Str);
                this->widgtmap_text.emplace(param_id, utf8Str);
                it = this->widgtmap_text.find(param_id);
            }
            ImGui::PushItemWidth(
                ImGui::GetContentRegionAvail().x * 0.65f - ImGui::GetFrameHeight() - style.ItemSpacing.x);
            bool button_edit = this->file_utils.FileBrowserButton(it->second);
            ImGui::SameLine();
            ImGui::InputText(param_label.c_str(), &it->second, ImGuiInputTextFlags_None);
            if (button_edit || ImGui::IsItemDeactivatedAfterEdit()) {
                GUIUtils::Utf8Decode(it->second);
                p->SetValue(vislib::StringA(it->second.c_str()));
            } else if (!ImGui::IsItemActive() && !ImGui::IsItemEdited()) {
                std::string utf8Str = std::string(p->ValueString().PeekBuffer());
                GUIUtils::Utf8Encode(utf8Str);
                it->second = utf8Str;
            }
            ImGui::PopItemWidth();
        } else {
            vislib::sys::Log::DefaultLog.WriteWarn(
                "Unknown Parameter Type. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            return;
        }
        
        // Reset to default style (use saved state)
        if (readOnly) {
            GUIUtils::ReadOnlyWigetStyle(false);
        }        
        ImGui::PopID();

        this->utils.HoverToolTip(param_desc, ImGui::GetID(param_label.c_str()), 0.5f);
        this->utils.HelpMarkerToolTip(help);
        
        ImGui::EndGroup();
    }
}

void GUIWindows::drawTransferFunctionEdit(
    const std::string& id, const std::string& label, megamol::core::param::TransferFunctionParam& p) {
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::BeginGroup();
    ImGui::PushID(id.c_str());

    // Reduced display of value and editor state.
    if (p.Value().empty()) {
        ImGui::TextDisabled("{    (empty)    }");
    } else {
        // XXX: A gradient texture would be nice here (sharing some editor code?)
        ImGui::TextUnformatted("{ ............. }");
    }

    bool isActive = (&p == this->tf_editor.GetActiveParameter());
    bool updateEditor = false;

    // Copy transfer function.
    if (ImGui::Button("Copy")) {
#ifdef GUI_USE_GLFW
        auto glfw_win = ::glfwGetCurrentContext();
        ::glfwSetClipboardString(glfw_win, p.Value().c_str());
#elif _WIN32
        ImGui::SetClipboardText(p.Value().c_str());
#else // LINUX
        vislib::sys::Log::DefaultLog.WriteWarn(
            "No clipboard use provided. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        vislib::sys::Log::DefaultLog.WriteInfo("Transfer Function JSON String:\n%s", p.Value().c_str());
#endif
    }
    ImGui::SameLine();

    //  Paste transfer function.
    if (ImGui::Button("Paste")) {
#ifdef GUI_USE_GLFW
        auto glfw_win = ::glfwGetCurrentContext();
        p.SetValue(::glfwGetClipboardString(glfw_win));
#elif _WIN32
        p.SetValue(ImGui::GetClipboardText());
#else // LINUX
        vislib::sys::Log::DefaultLog.WriteWarn(
            "No clipboard use provided. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
#endif
        updateEditor = true;
    }

    // Edit transfer function.
    ImGui::SameLine();
    ImGui::PushID("Edit_");
    ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[isActive ? ImGuiCol_ButtonHovered : ImGuiCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[isActive ? ImGuiCol_Button : ImGuiCol_ButtonHovered]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button("Edit")) {
        updateEditor = true;
        isActive = true;
        this->tf_editor.SetActiveParameter(&p);
        // Open window calling the transfer function editor callback
        const auto func = [](const std::string& wn, WindowManager::WindowConfiguration& wc) {
            if (wc.win_callback == WindowManager::DrawCallbacks::TF) {
                wc.win_show = true;
            }
        };
        this->window_manager.EnumWindows(func);
    }
    ImGui::PopStyleColor(3);
    ImGui::PopID();

    // Propagate the transfer function to the editor.
    if (isActive && updateEditor) {
        this->tf_editor.SetTransferFunction(p.Value());
    }

    ImGui::PopID();

    ImGui::SameLine();
    ImGui::TextEx(label.c_str(), ImGui::FindRenderedTextEnd(label.c_str()));
    ImGui::EndGroup();
}


void GUIWindows::drawParameterHotkey(const core::Module& mod, core::param::ParamSlot& slot) {
    auto parameter = slot.Parameter();
    if (!parameter.IsNull()) {
        if (auto* p = slot.template Param<core::param::ButtonParam>()) {
            std::string label = slot.Name().PeekBuffer();
            std::string desc = slot.Description().PeekBuffer();
            std::string keycode = p->GetKeyCode().ToString();

            ImGui::Columns(2, "hotkey_columns", false);

            ImGui::TextUnformatted(label.c_str());
            this->utils.HoverToolTip(desc);

            ImGui::NextColumn();

            ImGui::TextUnformatted(keycode.c_str());
            this->utils.HoverToolTip(desc);

            // Reset colums
            ImGui::Columns(1);

            ImGui::Separator();
        }
    }
}


bool GUIWindows::considerModule(const std::string& modname, std::vector<std::string>& modules_list) {
    bool retval = false;
    // Empty module list means that all modules should be considered.
    if (modules_list.empty()) {
        retval = true;
    } else {
        retval = (std::find(modules_list.begin(), modules_list.end(), modname) != modules_list.end());
    }
    return retval;
}


void GUIWindows::checkMultipleHotkeyAssignement(void) {
    if (this->state.hotkeys_check_once) {

        std::list<core::view::KeyCode> hotkeylist;
        hotkeylist.clear();

        // Fill with camera hotkeys for which no button parameters exist
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_W));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_A));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_S));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_D));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_C));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_V));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_Q));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_E));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_UP));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_DOWN));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_LEFT));
        hotkeylist.emplace_back(core::view::KeyCode(core::view::Key::KEY_RIGHT));

        for (auto& h : this->hotkeys) {
            hotkeylist.emplace_back(std::get<0>(h));
        }

        if (this->core_instance != nullptr) {
            this->core_instance->EnumParameters([&, this](const auto& mod, auto& slot) {
                auto parameter = slot.Parameter();
                if (!parameter.IsNull()) {
                    if (auto* p = slot.template Param<core::param::ButtonParam>()) {
                        auto hotkey = p->GetKeyCode();

                        // Ignore not set hotekey
                        if (hotkey.key == core::view::Key::KEY_UNKNOWN) {
                            return;
                        }

                        // check in hotkey map
                        bool found = false;
                        for (auto& kc : hotkeylist) {
                            if ((kc.key == hotkey.key) && (kc.mods.equals(hotkey.mods))) {
                                found = true;
                            }
                        }
                        if (!found) {
                            hotkeylist.emplace_back(hotkey);
                        } else {
                            vislib::sys::Log::DefaultLog.WriteWarn(
                                "The hotkey [%s] of the parameter \"%s::%s\" has already been assigned. "
                                ">>> If this hotkey is pressed, there will be no effect on this parameter!",
                                hotkey.ToString().c_str(), mod.FullName().PeekBuffer(), slot.Name().PeekBuffer());
                        }
                    }
                }
            });
        } else {
            vislib::sys::Log::DefaultLog.WriteError(
                "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        }

        this->state.hotkeys_check_once = false;
    }
}


bool megamol::gui::GUIWindows::hotkeyPressed(megamol::core::view::KeyCode keycode) {
    ImGuiIO& io = ImGui::GetIO();

    return (ImGui::IsKeyDown(static_cast<int>(keycode.key))) &&
           (keycode.mods.test(core::view::Modifier::ALT) == io.KeyAlt) &&
           (keycode.mods.test(core::view::Modifier::CTRL) == io.KeyCtrl) &&
           (keycode.mods.test(core::view::Modifier::SHIFT) == io.KeyShift);
}


void megamol::gui::GUIWindows::shutdown(void) {

    if (this->core_instance != nullptr) {
        vislib::sys::Log::DefaultLog.WriteInfo("GUIWindows: Triggering MegaMol instance shutdown.");
        this->core_instance->Shutdown();
    } else {
        vislib::sys::Log::DefaultLog.WriteError(
            "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
    }
}


void megamol::gui::GUIWindows::save_state_to_parameter(void) {
     
    try {
        std::string window_state;
        std::string param_state;
        if (this->window_manager.StateToJSON(window_state) && this->parameters_gui_state_to_json(param_state)) {
                
            nlohmann::json window_json;
            window_json = nlohmann::json::parse(window_state);
            
            nlohmann::json parameter_json;
            parameter_json = nlohmann::json::parse(param_state); 
                   
            // Merge both JSON states
            window_json.update(parameter_json);
                   
            std::string state;               
            state = window_json.dump(2); 
            
            this->state_param.Param<core::param::StringParam>()->SetValue(state.c_str(), false);
        }
    
    } catch (nlohmann::json::type_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return;
        //} catch (nlohmann::json::exception& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
        //} catch (nlohmann::json::parse_error& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
    } catch (nlohmann::json::invalid_iterator& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return;
    } catch (nlohmann::json::out_of_range& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return;
    } catch (nlohmann::json::other_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return;
    } catch (...) {
        vislib::sys::Log::DefaultLog.WriteError("Unknown Error - Unable to parse JSON string. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return;
    }    
}
     

bool megamol::gui::GUIWindows::parameters_gui_state_from_json(const std::string& json_string) {
    
    try {
        if (this->core_instance == nullptr) {
            vislib::sys::Log::DefaultLog.WriteError(
                "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            return false;
        }
        
        bool found = false;
        bool valid = true;

        nlohmann::json json;
        json = nlohmann::json::parse(json_string);

        if (!json.is_object()) {
            vislib::sys::Log::DefaultLog.WriteError("State is no valid JSON object. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            return false;
        }
        
        const std::string header = "Parameter_GUI_States";
        
        for (auto& h : json.items()) {
            if (h.key() == header) {
                found = true;
                for (auto& w : h.value().items()) {
                    std::string json_param_name = w.key();
                    
                    auto gui_state = w.value();
                    
                    // gui_visibility
                    bool gui_visibility;                    
                    if (gui_state.at("gui_visibility").is_boolean()) {
                        gui_state.at("gui_visibility").get_to(gui_visibility);
                    } else {
                        vislib::sys::Log::DefaultLog.WriteError(
                            "JSON state: Failed to read 'gui_visibility' as boolean.[%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
                        valid = false;
                    }
                    
                    // gui_read-only
                    bool gui_read_only;                                      
                    if (gui_state.at("gui_read-only").is_boolean()) {
                        gui_state.at("gui_read-only").get_to(gui_read_only);
                    } else {
                        vislib::sys::Log::DefaultLog.WriteError(
                            "JSON state: Failed to read 'gui_read-only' as boolean.[%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
                        valid = false;
                    }                    
                    
                    // gui_presentation_mode
                    megamol::core::param::AbstractParam::Presentations gui_presentation_mode;
                    if (gui_state.at("gui_presentation_mode").is_number_integer()) {
                        gui_presentation_mode = static_cast<megamol::core::param::AbstractParam::Presentations>(gui_state.at("gui_presentation_mode").get<int>());
                    } else {
                        vislib::sys::Log::DefaultLog.WriteError(
                            "JSON state: Failed to read 'gui_presentation_mode' as integer.[%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
                        valid = false;
                    }
                    
                    if (valid) {
                        this->core_instance->EnumParameters([&, this](const auto& mod, auto& slot) {
                            auto parameter = slot.Parameter();
                            if (!parameter.IsNull()) {
                                std::string param_name = std::string(slot.Name().PeekBuffer());
                                if (json_param_name == param_name) {
                                    parameter->SetGUIVisible(gui_visibility);
                                    parameter->SetGUIReadOnly(gui_read_only);
                                    parameter->SetGUIPresentation(gui_presentation_mode);
                                }
                            }
                        });
                    }
                }
            }
        }
        
        if (!found) {
            vislib::sys::Log::DefaultLog.WriteWarn("Could not find parameter gui state in JSON. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            return false;
        }        

    } catch (nlohmann::json::type_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
        //} catch (nlohmann::json::exception& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
        //} catch (nlohmann::json::parse_error& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
    } catch (nlohmann::json::invalid_iterator& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (nlohmann::json::out_of_range& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (nlohmann::json::other_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (...) {
        vislib::sys::Log::DefaultLog.WriteError("Unknown Error - Unable to parse JSON string. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    return true;
}


bool megamol::gui::GUIWindows::parameters_gui_state_to_json(std::string& json_string) {

    try {
        
        if (this->core_instance == nullptr) {
            vislib::sys::Log::DefaultLog.WriteError(
                "Pointer to core instance is nullptr. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
            return false;
        }
                
        const std::string header = "Parameter_GUI_States";                
        nlohmann::json json;
        
        json_string.clear();
        
        this->core_instance->EnumParameters([&, this](const auto& mod, auto& slot) {
            auto parameter = slot.Parameter();
            if (!parameter.IsNull()) {
                std::string param_name = std::string(slot.Name().PeekBuffer());

                json[header][param_name]["gui_visibility"] = parameter->IsGUIVisible();
                json[header][param_name]["gui_read-only"] = parameter->IsGUIReadOnly();
                json[header][param_name]["gui_presentation_mode"] = static_cast<int>(parameter->GetGUIPresentation());
            }
        });

        json_string = json.dump(2); // Dump with indent of 2 spaces and new lines.

    } catch (nlohmann::json::type_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
        //} catch (nlohmann::json::exception& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
        //} catch (nlohmann::json::parse_error& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
    } catch (nlohmann::json::invalid_iterator& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (nlohmann::json::out_of_range& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (nlohmann::json::other_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (...) {
        vislib::sys::Log::DefaultLog.WriteError("Unknown Error - Unable to write JSON of state. [%s, %s, line %d]\n", __FILE__, __FUNCTION__, __LINE__);
        return false;
    }

    return true;
}
