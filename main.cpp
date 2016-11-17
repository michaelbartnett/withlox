#include "logging.h"

#include "pretty.h"
#include "typesys.h"
#include "hashtable.h"
#include "bucketarray.h"
#include "nametable.h"
#include "json_error.h"
#include "str.h"
#include "common.h"
#include "platform.h"
#include "json.h"
#include "linenoise.h"
#include "imgui_helpers.h"
#include "dearimgui/imgui_internal.h"
#include "dearimgui/imgui.h"
#include "dearimgui/imgui_impl_sdl.h"
#include "memory.h"
#include <SDL.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

#include <signal.h>
#include <unistd.h>

#include "tokenizer.h"
#include "clicommands.h"
#include "programstate.h"
#include <algorithm>

#include "typesys_json.h"



void test_json_import(ProgramState *prgstate, int filename_count, char **filenames)
{
    if (filename_count < 1)
    {
        logf("No files specified\n");
        return;
    }


    size_t jsonflags = json_parse_flags_default
                     | json_parse_flags_allow_trailing_comma
                     | json_parse_flags_allow_c_style_comments
                     ;

    TypeDescriptor *result = nullptr;

    for (int i = 0; i < filename_count; ++i)
    {
        const char *filename = filenames[i];
        Str jsonstr = read_file(filename);
        ASSERT(jsonstr.data);

        json_parse_result_s jp_result = {};
        json_value_s *jv = json_parse_ex(jsonstr.data, jsonstr.length,
                                         jsonflags, &jp_result);

        if (!jv)
        {
            logf("Json parse error: %s\nAt %lu:%lu",
                   json_error_code_string(jp_result.error),
                   jp_result.error_line_no,
                   jp_result.error_row_no);
            return;
        }

        TypeDescriptor *typedesc = typedesc_from_json(prgstate, jv);
        NameRef bound_name = nametable::find_or_add(&prgstate->names, filename);
        bind_typedesc_name(prgstate, bound_name, typedesc);

        logln("New type desciptor:");
        pretty_print(typedesc);

        if (!result)
        {
            result = typedesc;
        }
        else
        {
            result = merge_types(prgstate, result, typedesc);
        }
    }

    logln("completed without errors");

    pretty_print(result);
}


void log_parse_error(JsonParseResult pr, StrSlice input, size_t input_offset_from_src)
{
    FormatBuffer fmt_buf;
    fmt_buf.flush_on_destruct();

    fmt_buf.writef("Json parse error: %s\nAt %lu:%lu\n%s\n",
                   pr.error_desc.data,
                   pr.error_line,
                   pr.error_column + (pr.error_line > 1 ? 0 : input_offset_from_src),
                   input.data);

    size_t buffer_offset = pr.error_offset + input_offset_from_src;
    for (size_t i = 0; i < buffer_offset - 1; ++i) fmt_buf.write("~");
    fmt_buf.write("^");
}


void process_console_input(ProgramState *prgstate, StrSlice input_buf)
{
    // mem::ALLOC_STACKTRACE = true;

    tokenizer::State tokstate;
    tokenizer::init(&tokstate, input_buf.data);

    tokenizer::Token first_token = tokenizer::read_string(&tokstate);

    DynArray<Value> cmd_args;
    dynarray::init(&cmd_args, 10);

    bool error = false;

    for (;;)
    {
        size_t offset_from_input = (size_t)(tokstate.current - input_buf.data);

        Value parsed_value;
        StrSlice parse_slice = str_slice(tokstate);
        JsonParseResult parse_result = try_parse_json_as_value(OUTPARAM &parsed_value, prgstate,
                                                               parse_slice.data, parse_slice.length);

        switch (parse_result.status)
        {
            case JsonParseResult::Failed:
                log_parse_error(parse_result, parse_slice, offset_from_input);
                error = true;
                goto AfterArgParseLoop;
            case JsonParseResult::Succeeded:
                dynarray::append(&cmd_args, parsed_value);
                break;
            case JsonParseResult::Eof:
                goto AfterArgParseLoop;
        }

        tokstate.current += parse_result.parse_offset;

        if (tokenizer::past_end(&tokstate))
        {
            goto AfterArgParseLoop;
        }

    }
AfterArgParseLoop:

    if (!error)
    {
        exec_command(prgstate, first_token.text, cmd_args);
    }

    dynarray::deinit(&cmd_args);

    // mem::ALLOC_STACKTRACE = false;
}


void run_terminal_json_cli(ProgramState *prgstate)
{
    for (;;)
    {
        char *input = linenoise(">> ");

        if (!input)
        {
            break;
        }

        // void *alloc_probe = mem::default_allocator()->probe();
        append_log(input);

        tokenizer::State tokstate;
        tokenizer::init(&tokstate, input);

        tokenizer::Token first_token = tokenizer::read_string(&tokstate);

        if (first_token.type == tokenizer::TokenType::Eof)
        {
            std::free(input);
            continue;
        }

        process_console_input(prgstate, str_slice(input));

        // mem::default_allocator()->log_allocs_since_probe(alloc_probe);

        std::free(input);
    }
}




class CliHistory
{
public:

    CliHistory()
        : input_entries()
        , saved_input_buf()
        , pos(0)
        , saved_cursor_pos(0)
        , saved_selection_start(0)
        , saved_selection_end(0)
        {
        }

    void to_front();
    void add(StrSlice input);
    void restore(s64 position, ImGuiTextEditCallbackData *data);
    void backward(ImGuiTextEditCallbackData *data);
    void forward(ImGuiTextEditCallbackData *data);

    DynArray<Str> input_entries;
    Str saved_input_buf;
    s64 pos;
    s32 saved_cursor_pos;
    s32 saved_selection_start;
    s32 saved_selection_end;
};


void CliHistory::to_front()
{
    if (this->saved_input_buf.capacity)
    {
        str_free(&this->saved_input_buf);
    }
    this->pos = -1;
}


void CliHistory::add(StrSlice input)
{
    if (!this->input_entries.data)
    {
        dynarray::init(&this->input_entries, 64);
    }
    dynarray::append(&this->input_entries, str(input));
    this->to_front();
}


void CliHistory::restore(s64 position, ImGuiTextEditCallbackData* data)
{
    if (this->input_entries.count == 0)
    {
        return;
    }

    Str *new_input = &this->input_entries[DYNARRAY_COUNT(position)];
    memcpy(data->Buf, new_input->data, new_input->length);
    data->Buf[new_input->length] = '\0';
    data->BufTextLen = data->SelectionStart = data->SelectionEnd = data->CursorPos = new_input->length;
    data->BufDirty = true;
}


void CliHistory::backward(ImGuiTextEditCallbackData* data)
{
    s64 prev_pos = this->pos;

    if (this->pos == -1)
    {
        this->pos = this->input_entries.count - 1;
        this->saved_input_buf = str(data->Buf);
        this->saved_cursor_pos = data->CursorPos;
        this->saved_selection_start = data->SelectionStart;
        this->saved_selection_end = data->SelectionEnd;
    }
    else if (this->pos > 0)
    {
        --this->pos;
    }

    if (prev_pos != this->pos)
    {
        this->restore(this->pos, data);
    }
}


void CliHistory::forward(ImGuiTextEditCallbackData* data)
{
    s64 prev_pos = this->pos;

    if (this->pos >= 0)
    {
        ++this->pos;
    }

    if (prev_pos == this->input_entries.count - 1)
    {
        memcpy(data->Buf, this->saved_input_buf.data, this->saved_input_buf.length);
        data->Buf[this->saved_input_buf.length] = '\0';
        data->BufTextLen = this->saved_input_buf.length;
        data->CursorPos = this->saved_cursor_pos;
        data->SelectionStart = this->saved_selection_start;
        data->SelectionEnd = this->saved_selection_end;
        data->BufDirty = true;

        this->to_front();
    }
    else if (prev_pos != this->pos)
    {
        this->restore(this->pos, data);
    }
}


int imgui_cli_history_callback(ImGuiTextEditCallbackData *data)
{
    CliHistory *hist = (CliHistory *)data->UserData;

    if (data->EventFlag != ImGuiInputTextFlags_CallbackHistory)
    {
        return 0;
    }

    if (data->EventKey == ImGuiKey_UpArrow)
    {
        hist->backward(data);
    }
    else if (data->EventKey == ImGuiKey_DownArrow)
    {
        hist->forward(data);
    }

    return 0;
}


void draw_imgui_json_cli(ProgramState *prgstate, SDL_Window *window)
{
    static float scroll_y = 0;
    static bool highlight_output_mode = false;
    static bool first_draw = true;
    static CliHistory history;

    ImGuiWindowFlags console_wndflags = 0
        | ImGuiWindowFlags_NoSavedSettings
        // | ImGuiWindowFlags_NoTitleBar
        // | ImGuiWindowFlags_NoMove
        // | ImGuiWindowFlags_NoResize
        ;

    ImGui::SetNextWindowSize(ImGui_SDLWindowSize(window).scaled(0.5), ImGuiSetCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("ConsoleWindow###jsoneditor-console", nullptr, console_wndflags);

    ImGui::TextUnformatted("This is the console.");
    ImGui::Separator();

    ImGui::BeginChild("Output",
                      ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()),
                      false,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);


    ImGuiInputTextFlags output_flags = ImGuiInputTextFlags_ReadOnly;
    Str *biglog = concatenated_log();

    if (highlight_output_mode)
    {
        ImGui::InputTextMultiline("##log-output", biglog->data, biglog->length, ImVec2(-1, -1), output_flags);

        if ((! (ImGui::IsWindowHovered() || ImGui::IsItemHovered()) && ImGui::IsMouseReleased(0))
            || ImGui::IsKeyReleased(SDLK_TAB))
        {
            highlight_output_mode = false;
        }
    }
    else
    {
        ImGui::TextUnformatted(biglog->data, biglog->data + biglog->length);
        scroll_y = ImGui::GetItemRectSize().y;
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
        {
            highlight_output_mode = true;
        }
        ImGui::SetScrollY(scroll_y);
    }



    ImGui::EndChild();

    static char input_buf[1024];
    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    if (first_draw)
    {
        ImGui::SetKeyboardFocusHere();
    }
    if (ImGui::InputText("##input", input_buf, sizeof(input_buf), input_flags, imgui_cli_history_callback, &history))
    {
        StrSlice input_slice = str_slice(input_buf);
        if (input_slice.length > 0)
        {
            history.add(input_slice);
            log(input_buf);

            process_console_input(prgstate, input_slice);

            input_buf[0] = '\0';
        }
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
    ImGui::PopStyleVar();

    first_draw = false;
}



void draw_value_editor(ProgramState *prgstate, Value *value, const char *label);


float ImGui_GetVertSpacing()
{
    return GImGui->Style.ItemSpacing.y;
}

float ImGui_GetHorizSpacing()
{
    return GImGui->Style.ItemSpacing.x;
}

void draw_array_list_editor(ProgramState *prgstate, Value *value, const char *label)
{
    ASSERT(vIS_ARRAY(value));

    if (ImGui::TreeNode("Array##array_list_editor"))
    {
        DynArray<Value> *elems = &value->array_value.elements;
        for (DynArrayCount i = 0, e = elems->count; i < e; ++i)
        {
            ImGui::PushID(S32(i));
            Value *val = elems->data + i;
            if (ImGui::TreeNode("##array_elem", "[%i]", i))
            {
                draw_value_editor(prgstate, val, label);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}

void draw_array_table_editor(ProgramState *prgstate, Value *value, const char *label)
{
    ASSERT(vIS_ARRAY(value));

    TypeDescriptor *elem_type = value->typedesc->array_type.elem_type;

    bool is_compound_ish = (tIS_COMPOUND(elem_type) || (tIS_UNION(elem_type) &&
                                                        all_typecases_compound(&elem_type->union_type)));

    if (!is_compound_ish)
    {
        ImGui::Text("i dunno man");
        return;
    }

    DynArray<NameRef> element_member_names = dynarray::init<NameRef>(16);

    if (tIS_COMPOUND(elem_type))
    {
        for (DynArrayCount i = 0, e = elem_type->compound_type.members.count; i < e; ++i)
        {
            dynarray::append(&element_member_names, elem_type->compound_type.members[i].name);
        }
    }
    else
    {
        for (DynArrayCount i = 0, ie = elem_type->union_type.type_cases.count; i < ie; ++i)
        {
            TypeDescriptor *t = elem_type->union_type.type_cases[i];
            ASSERT(tIS_COMPOUND(t));
            CompoundType *ct = &t->compound_type;

            for (DynArrayCount j = 0, je = ct->members.count; j < je; ++j)
            {
                dynarray::append_if_not_present<NameRef, nameref::Identical>(&element_member_names, ct->members[j].name);
            }
        }
    }

    ImGui::PushID("array_editor");
    ImGui::BeginChild("Array columns headers",
                      ImVec2(0, ImGui::GetItemsLineHeightWithSpacing()+ ImGui_GetVertSpacing()),
                      false, 0);
    ImGui::Separator();
    ImGui::Columns(S32(element_member_names.count), "column_headers");
    ImGui::Separator();

    for (DynArrayCount i = 0; i < element_member_names.count; ++i)
    {
        ImGui::Text("%s", nameref::str_slice(element_member_names[i]).data);
        ImGui::NextColumn();
    }

    float *column_offsets = MAKE_ZEROED_ARRAY(mem::default_allocator(), element_member_names.count, float);
    for (DynArrayCount i = 0; i < element_member_names.count; ++i)
    {
        column_offsets[i] = ImGui::GetColumnOffset(S32(i));
    }

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::EndChild();

    NameRef name_name = nametable::find_or_add(&prgstate->names, "name");
    NameRef name_id = nametable::find_or_add(&prgstate->names, "id");

    ImGui::BeginChild("Array columns data", ImVec2(0, 0), false, 0);

    ImGui::Separator();
    ImGui::Columns(S32(element_member_names.count), "column_rows");
    // ImGui::Columns(S32(element_member_names.count + 1), "column_rows");

    for (DynArrayCount i = 0, ie = value->array_value.elements.count; i < ie; ++i)
    {
        ImGui::Separator();
        ImGui::PushID(S32(i));

        Value *elem_value = &value->array_value.elements[i];
        ASSERT(vIS_COMPOUND(elem_value));

        const char *use_label = label;
        if (!use_label)
        {
            CompoundValueMember *identifier_mem = find_member(elem_value, name_name);
            if (identifier_mem && vIS_STRING(&identifier_mem->value))
            {
                use_label = identifier_mem->value.str_val.data;
            }
            else
            {
                identifier_mem = find_member(elem_value, name_id);
                if (identifier_mem && vIS_STRING(&identifier_mem->value))
                {
                    use_label = identifier_mem->value.str_val.data;
                }
                else
                {
                    use_label = "UNKNOWN_LABEL";
                }
            }
        }

        // ImGui::PushID(-1);
        // bool selectable_pressed = ImGui::Selectable("##selectable", false,
        //                                             ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_DrawFillAvailWidth,
        //                                             ImVec2(0.00001, 0));
        // ImGui::PopID();
        // ImGui::NextColumn();

        for (DynArrayCount j = 0, je = element_member_names.count; j < je; ++j)
        {
            ImGui::PushID(S32(j));

            CompoundValueMember *memval = find_member(elem_value, element_member_names[j]);

            if (!memval)
            {
                ImGui::Text("NOPE");
            }
            else
            {
                draw_value_editor(prgstate, &memval->value, use_label);
            }

            ImGui::PopID();
            ImGui::NextColumn();
        }

        // if (i + 1 < ie) ImGui::Separator();
        ImGui::PopID();
    }

    for (DynArrayCount i = 0; i < element_member_names.count; ++i)
    {
        ImGui::SetColumnOffset(S32(i), column_offsets[i]);
        // ImGui::SetColumnOffset(S32(i+1), column_offsets[i]);
    }
    mem::default_allocator()->dealloc(column_offsets);
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::EndChild();
    ImGui::PopID();

    dynarray::deinit(&element_member_names);
}


void draw_compound_value_editor(ProgramState *prgstate, Value *value)
{
    ASSERT(vIS_COMPOUND(value));

    float max_width = 0;
    float horiz_spacing = ImGui_GetHorizSpacing();

    for (DynArrayCount i = 0, e = value->compound_value.members.count; i < e; ++i)
    {
        CompoundValueMember *member = &value->compound_value.members[i];
        StrSlice namelabel = nameref::str_slice(member->name);
        ImVec2 text_size = ImGui::CalcTextSize(namelabel.data, namelabel.data + namelabel.length);
        max_width = max(max_width, text_size.x);
    }

    for (DynArrayCount i = 0, e = value->compound_value.members.count; i < e; ++i)
    {
        ImGui::PushID(S32(i));
        CompoundValueMember *member = &value->compound_value.members[i];
        if (tIS_COMPOUND(member->value.typedesc))
        {
            ImGui::Text("TODO: Compound in Compound");
        }
        else if (tIS_ARRAY(member->value.typedesc))
        {
            ImGui::Text("TODO: Array in Compound");
        }
        else
        {
            StrSlice namelabel = nameref::str_slice(member->name);
            ImGui::Text("%s", namelabel.data);

            float pos_x = max_width;

            ImGuiWindow *window = ImGui::GetCurrentWindow();
            if (window->DC.TreeDepth > 0)
            {
                pos_x += window->DC.CursorPos.x;
            }

            ImGui::SameLine(pos_x, horiz_spacing);
            draw_value_editor(prgstate, &member->value, namelabel.data);
        }
        ImGui::PopID();
    }
}


static int array_depth = 0;

void draw_value_editor(ProgramState *prgstate, Value *value, const char *label)
{
    switch ((TypeID::Tag)(value->typedesc->type_id))
    {
        case TypeID::None:
            ImGui::Text("%s NONE", label);
            break;

        case TypeID::String:
            ImGui_InputText("##field_value", &value->str_val);
            break;

        case TypeID::Int:
            ImGui::InputInt("##field_value", &value->s32_val);
            break;

        case TypeID::Float:
            ImGui::InputFloat("##field_value", &value->f32_val);
            break;

        case TypeID::Bool:
            ImGui::Checkbox("##field_value", &value->bool_val);
            break;

        case TypeID::Array:
            if (array_depth == 0)
            {
                ++array_depth;
                draw_array_table_editor(prgstate, value, label);
                --array_depth;
            }
            else
            {
                ++array_depth;
                draw_array_list_editor(prgstate, value, label);
                --array_depth;
            }
            break;

        case TypeID::Compound:
            // ImGui::Text("TODO: Compound tree");
            draw_compound_value_editor(prgstate, value);//, label);
            break;

        case TypeID::Union:
            ASSERT_MSG("can't render a union");
            break;
    }
}


bool draw_window_value_editor(ProgramState *prgstate, Collection *collection)
{
    ImGuiWindowFlags wndflags = 0
        | ImGuiWindowFlags_NoSavedSettings
        // | ImGuiWindowFlags_NoMove
        // | ImGuiWindowFlags_NoResize
        ;

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    bool window_open = true;
    ImGui::Begin(collection->load_path.data, &window_open, wndflags);

    draw_value_editor(prgstate, &collection->value, nullptr);

    ImGui::Columns(1);
    // ImGui::Separator();

    ImGui::End();
    ImGui::PopStyleVar();

    return window_open;
}


bool draw_typelist_window(ProgramState *prgstate)
{
    ImGuiWindowFlags wndflags = 0
        | ImGuiWindowFlags_NoSavedSettings
        // | ImGuiWindowFlags_NoMove
        // | ImGuiWindowFlags_NoResize
        ;

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
    bool window_open = true;

    if (! ImGui::Begin("Type Descriptors", &window_open, wndflags))
    {
        goto EndWindow;
    }

    ImGui::Text("There are %i type descriptors loaded", prgstate->type_descriptors.count);

    if (ImGui::BeginChild("Here they are..."))
    {
        FormatBuffer fmtbuf;
        for (BucketItemCount i = 0, ei = prgstate->type_descriptors.capacity; i < ei; ++i)
        {
            TypeDescriptor *typedesc;
            if (! bucketarray::get_if_not_empty(&typedesc, &prgstate->type_descriptors, i))
                continue;

            DynArray<NameRef> *names = find_names_of_typedesc(prgstate, typedesc);

            fmtbuf.clear();
            fmtbuf.write('[');
            if (names)
            {
                for (DynArrayCount j = 0, ej = names->count; j < ej; ++j)
                {
                    StrSlice name = nameref::str_slice((*names)[j]);
                    if (j > 0)
                    {
                        fmtbuf.write(", ");
                    }
                    fmtbuf.write(name.data);
                }
            }
            fmtbuf.write(']');

            ImGui::PushID(S32(i));

            bool tree_open = ImGui::TreeNode("##typedesc_list_entry", "%-8s @ %p, %s",
                                             TypeID::to_string(typedesc->type_id), typedesc, fmtbuf.buffer);

            if (tree_open)
            {
                fmtbuf.clear();
                pretty_print(typedesc, &fmtbuf);

                ImGui::TreeNodeEx("##typedesc_pprint",
                                  ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen,
                                  "%s", fmtbuf.buffer);

                ImGui::TreePop();
            }
            ImGui::PopID();
        }

    }
    ImGui::EndChild();


EndWindow:
    ImGui::End();
    return window_open;
}


void log_display_info()
{
    int num_displays = SDL_GetNumVideoDisplays();
    logf_ln("Num displays: %i", num_displays);

    for (int i = 0; i < num_displays; ++i) {
        SDL_DisplayMode display_mode;
        SDL_GetCurrentDisplayMode(i, &display_mode);
        const char *display_name = SDL_GetDisplayName(i);
        logf_ln("Display[%i]: %s (%i x %i @ %i hz)",
                i, display_name, display_mode.w, display_mode.h, display_mode.refresh_rate);
    }
}


s32 run_nametable_tests();

int main(int argc, char **argv)
{
    mem::memory_init(logf_with_userdata, nullptr);
    FormatBuffer::set_default_flush_fn(log_write_with_userdata, nullptr);

    s32 fails = run_nametable_tests();
    ASSERT(fails == 0);

    ProgramState prgstate;
    prgstate_init(&prgstate);

    load_base_type_descriptors(&prgstate);
    init_cli_commands(&prgstate);

    // mem::default_allocator()->log_allocations();
    UNUSED(argc);
    UNUSED(argv);
    // test_json_import(&prgstate, argc - 1, argv + 1);

    // TTY console
    // run_terminal_json_cli(&prgstate);

    if (0 != SDL_Init(SDL_INIT_VIDEO))
    {
        logf_ln("Failed to initialize SDL: %s", SDL_GetError());
        end_of_program();
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    log_display_info();
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    float wpct = 1;
    float hpct = 1;
    SDL_Window *window = SDL_CreateWindow("jsoneditor",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          (int)(display_mode.w * wpct), (int)(display_mode.h * hpct),
                                          SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    u32 window_id = SDL_GetWindowID(window);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImVec4 clear_color = ImColor(114, 144, 154);
    ImGui_ImplSdl_Init(window);


    /////////// TESTING ///////////
    process_console_input(&prgstate, str_slice("loadjson \"test/nested\""));
    process_console_input(&prgstate, str_slice("edit 0"));
    ///////////////////////////////

    bool show_imgui_testwindow = false;
    SDL_Event event;
    bool running = SDL_PollEvent(&event);
    while (running)
    {
        do {
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                running = false;
            }
            else if (!ImGui_ImplSdl_ProcessEvent(&event))
            {
                // event not handled by imgui
                switch (event.type)
                {
                    case SDL_QUIT:
                        running = false;
                        break;

                    case SDL_WINDOWEVENT:
                        if (event.window.windowID == window_id &&
                            event.window.event == SDL_WINDOWEVENT_CLOSE)
                        {
                            running = false;
                        }
                        break;
                }
            }
        } while (SDL_PollEvent(&event));


        ImGui_ImplSdl_NewFrame(window);

        draw_imgui_json_cli(&prgstate, window);

        for (DynArrayCount i = 0, e = prgstate.editing_collections.count; i < e; ++i)
        {
            bool open = draw_window_value_editor(&prgstate, prgstate.editing_collections[i]);

            if (!open)
            {
                dynarray::swappop(&prgstate.editing_collections, i);
                --i;
                --e;
            }
        }

        draw_typelist_window(&prgstate);

        ImGui::ShowTestWindow(&show_imgui_testwindow);

        glViewport(0, 0,
                   (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_GL_SwapWindow(window);
        if (ImGui::GetIO().WantCaptureMouse)
        {
            mem::zero_obj(event);
        }
        else
        {
            int sdl_wait_event_ok = SDL_WaitEvent(&event);
            ASSERT(sdl_wait_event_ok);
        }
    }

    ImGui_ImplSdl_Shutdown();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    end_of_program();
    return 0;
}
