// Exercise the real event handler without a window, Vulkan backend or guest.
#include "../gpu/shim/port_overlay.cpp"

static void Check(bool condition) {
    if (condition) return;
    std::fputs("FAIL: overlay input isolation\n", stderr);
    std::exit(1);
}

static void Key(SDL_Keycode key, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.key = key;
    PortOverlay::HandleEvent(event);
}

static void Mouse(bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.button = SDL_BUTTON_LEFT;
    PortOverlay::HandleEvent(event);
}

static void Gamepad(bool down, SDL_GamepadButton button = SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
    event.gbutton.button = button;
    PortOverlay::HandleEvent(event);
}

static void Trigger(bool down) {
    SDL_Event event{};
    event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    event.gaxis.axis = SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    event.gaxis.value = down ? 20000 : 0;
    PortOverlay::HandleEvent(event);
}

int main() {
    ImGui::CreateContext();
    PortOverlay::initialized = true;
    auto& io = ImGui::GetIO();
    Key(SDLK_INSERT, true);
    Check(PortOverlay::CapturesInput());
    Key(SDLK_DOWN, true);
    Mouse(true);
    Gamepad(true);
    ImGui::UpdateInputEvents(false);
    Check(ImGui::IsKeyDown(ImGuiKey_DownArrow));
    Check(io.MouseDown[0]);
    Check(ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown));

    Key(SDLK_INSERT, true);
    Key(SDLK_DOWN, false);
    Mouse(false);
    Gamepad(false);
    Key(SDLK_INSERT, true);
    ImGui::UpdateInputEvents(false);
    std::printf("reopen: keyboard=%d mouse=%d gamepad=%d\n",
                ImGui::IsKeyDown(ImGuiKey_DownArrow), io.MouseDown[0],
                ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown));
    std::fflush(stdout);
    Check(!ImGui::IsKeyDown(ImGuiKey_DownArrow));
    Check(!io.MouseDown[0]);
    Check(!ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown));

    // Also close before queued presses have reached a rendered ImGui frame.
    Key(SDLK_DOWN, true);
    Mouse(true);
    Key(SDLK_INSERT, true);
    Key(SDLK_DOWN, false);
    Mouse(false);
    Key(SDLK_INSERT, true);
    ImGui::UpdateInputEvents(false);
    Check(!ImGui::IsKeyDown(ImGuiKey_DownArrow));
    Check(!io.MouseDown[0]);
    Key(SDLK_DOWN, true);
    Mouse(true);
    Gamepad(true);
    ImGui::UpdateInputEvents(false);
    Check(ImGui::IsKeyDown(ImGuiKey_DownArrow) && io.MouseDown[0]);
    Check(ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown));
    Key(SDLK_DOWN, false);
    Mouse(false);
    Gamepad(false);
    ImGui::UpdateInputEvents(false);
    Check(!ImGui::IsKeyDown(ImGuiKey_DownArrow) && !io.MouseDown[0]);
    Check(!ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown));

    Key(SDLK_DOWN, true);
    Mouse(true);
    Gamepad(true);
    ImGui::UpdateInputEvents(false);
    Trigger(true);
    Gamepad(true, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
    Check(!PortOverlay::CapturesInput());
    Key(SDLK_DOWN, false);
    Mouse(false);
    Gamepad(false);
    Gamepad(false, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
    Trigger(false);
    // Reverse chord order also opens through the same transition.
    Gamepad(true, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
    Trigger(true);
    Check(PortOverlay::CapturesInput());
    ImGui::UpdateInputEvents(false);
    Check(!ImGui::IsKeyDown(ImGuiKey_DownArrow) && !io.MouseDown[0]);
    Check(!ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown));
    PortOverlay::initialized = false;
    ImGui::DestroyContext();
    std::puts("PASS: overlay close/reopen input isolation");
}
