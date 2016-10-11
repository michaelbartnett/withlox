#include "imgui_helpers.h"
#include "dearimgui/imgui_internal.h"
#include "str.h"


bool ImGui_InputText(const char *label, Str *str)
{
    ImGuiContext *guictx = ImGui::GetCurrentContext();
    ImGuiID control_id = ImGui::GetID(label);

    if (guictx->ActiveId == control_id)
    {
        if ((str->capacity - str->length) < 2)
        {
            str_ensure_capacity(str, str->capacity + 32);
            guictx->ActiveId = 0;
            ImGui::SetKeyboardFocusHere(0);
        }
    }

    bool value_changed = ImGui::InputText(label, str->data, str->capacity);

    if (value_changed)
    {
        str->length = STRLEN(std::strlen(str->data));
    }

    return value_changed;
}
