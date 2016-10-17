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
// #include "dearimgui/imgui_internal.h"
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

/*

Memory Management:

http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html#Appendix_26

http://www.ibm.com/developerworks/aix/tutorials/au-memorymanager/

https://bitbucket.org/bitsquid/foundation/src/7f896236dbafd2cb842655004b1c7bf6e76dcef9?at=default
https://bitbucket.org/bitsquid/foundation/src/7f896236dbafd2cb842655004b1c7bf6e76dcef9/memory.h?at=default&fileviewer=file-view-default
https://bitbucket.org/bitsquid/foundation/src/7f896236dbafd2cb842655004b1c7bf6e76dcef9/memory.cpp?at=default&fileviewer=file-view-default
https://bitbucket.org/bitsquid/foundation/src/7f896236dbafd2cb842655004b1c7bf6e76dcef9/temp_allocator.h?at=default&fileviewer=file-view-default
https://www.google.com/search?q=bitsquid+blog&oq=bitsquid+blog&aqs=chrome..69i57j0j69i59.2205j0j7&sourceid=chrome&ie=UTF-8
http://bitsquid.blogspot.com/2016/04/the-poolroom-figure-seq-figure-arabic-1.html
http://bitsquid.blogspot.com/2012/11/bitsquid-foundation-library.html
http://bitsquid.blogspot.com/2012/09/a-new-way-of-organizing-header-files.html
http://gamesfromwithin.com/the-always-evolving-coding-style
http://gamesfromwithin.com/opengl-and-uikit-demo
https://twitter.com/GeorgeSealy/status/15800523038
http://gamesfromwithin.com/backwards-is-forward-making-better-games-with-test-driven-development
http://gamesfromwithin.com/simple-is-beautiful
http://bitsquid.blogspot.com/2012/01/sensible-error-handling-part-1.html
http://bitsquid.blogspot.com/2012/02/sensible-error-handling-part-2.html
http://bitsquid.blogspot.com/2012/02/sensible-error-handling-part-3.html
http://bitsquid.blogspot.com/2010/12/bitsquid-c-coding-style.html
http://bitsquid.blogspot.com/2011/12/platform-specific-resources.html
http://bitsquid.blogspot.com/2011/12/pragmatic-approach-to-performance.html
http://bitsquid.blogspot.com/2011/08/idea-for-better-watch-windows.html
http://bitsquid.blogspot.com/2011/05/monitoring-your-game.html
http://web.archive.org/web/20120419004126/http://www.altdevblogaday.com/2011/05/17/a-birds-eye-view-of-your-memory-map/
http://bitsquid.blogspot.com/2011/01/managing-coupling.html
http://bitsquid.blogspot.com/2011/02/managing-decoupling-part-2-polling.html
http://bitsquid.blogspot.com/2011/02/some-systems-need-to-manipulate-objects.html
http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html
http://bitsquid.blogspot.com/2011/11/example-in-data-oriented-design-sound.html


 */

/* JSON Libraries

http://www.json.org/
http://lloyd.github.io/yajl/
http://www.json.org/JSON_checker/
https://github.com/udp/json-parser
https://github.com/udp/json-builder
https://github.com/zserge/jsmn
https://docs.google.com/spreadsheets/d/1L8XrSas9_PS5RKduSgXUiHDmH50VQxsvwjYDoRHu_9I/edit#gid=0
https://github.com/kgabis/parson
https://github.com/esnme/ujson4c/
https://github.com/esnme/ultrajson
https://bitbucket.org/yarosla/nxjson/src
https://github.com/cesanta/frozen

https://github.com/nothings/stb/blob/master/docs/other_libs.md
https://github.com/kazuho/picojson
https://github.com/sheredom/json.h
https://github.com/Zguy/Jzon/blob/master/Jzon.h
https://github.com/kgabis/parson
https://github.com/miloyip/nativejson-benchmark
https://github.com/open-source-parsers/jsoncpp
https://github.com/giacomodrago/minijson_writer
https://www.quora.com/What-is-the-best-C-JSON-library
https://github.com/esnme/ujson4c/blob/master/src/ujdecode.c
https://www.google.com/search?q=c%2B%2B+json&oq=c%2B%2B+json&aqs=chrome.0.0l2j69i60j0j69i61l2.1031j0j7&sourceid=chrome&ie=UTF-8#q=c+json

 */

// Debugging
// http://stackoverflow.com/questions/312312/what-are-some-reasons-a-release-build-would-run-differently-than-a-debug-build
// http://www.codeproject.com/Articles/548/Surviving-the-Release-Version


// Type theory
// http://chris-taylor.github.io/blog/2013/02/10/the-algebra-of-algebraic-data-types/
// https://en.wikipedia.org/wiki/Algebraic_data_type
// https://en.wikipedia.org/wiki/Generalized_algebraic_data_type

// GraphQL has a type system. Interesting?
// https://facebook.github.io/graphql/

// class MemStack
// {
// public:
//     MemoryStack(size_t size)
//         : size(size)
//         , top(0)
//     {
//         data = malloc(size);
//     }

//     size_t size;
//     u8 *data;
//     u8 *top;
// /*


// ----
// size_prev:32
// size_this:30
// flags:2  ALLOC/FREE (meaning, this block)
// ----
// payload
// ----
// next header...
// ----



// HEADER

// to push, add header
// to pop, lookup header, remove by size

// FOOTER







void drop_collection(ProgramState *prgstate, BucketIndex bucket_index)
{
    Collection *coll = &prgstate->collections[bucket_index];

    for (DynArrayCount i = 0, e = coll->records.count; i < e; ++i)
    {
        LoadedRecord *lr = coll->records[i];
        str_free(&lr->fullpath);
        value_free_components(&lr->value);
        BucketIndex removed_bucket_index = bucketarray::remove(&prgstate->loaded_records, lr);
        ASSERT(removed_bucket_index.is_valid());
    }

    str_free(&coll->load_path);
    dynarray::deinit(&coll->records);

    DynArrayCount edit_index;
    if (dynarray::try_find_index(&edit_index, &prgstate->editing_collections, coll))
    {
        dynarray::swappop(&prgstate->editing_collections, edit_index);
    }

    bool removed = bucketarray::remove_at(&prgstate->collections, bucket_index);
    ASSERT(removed);
}


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
        // str_overwrite(this->saved_input_buf, data->Buf);
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
    // if (first_draw)
    // {

    // }

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


bool draw_collection_editor(Collection *collection)
{
    ImGuiWindowFlags wndflags = 0
        | ImGuiWindowFlags_NoSavedSettings
        // | ImGuiWindowFlags_NoMove
        // | ImGuiWindowFlags_NoResize
        ;

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    // ImGui::Begin("Collection Editor", nullptr, wndflags);
    bool window_open = true;
    ImGui::Begin(collection->load_path.data, &window_open, wndflags);

    TypeDescriptor *toptype = collection->top_typedesc;

    bool is_compound_ish = tIS_COMPOUND(toptype) || (tIS_UNION(toptype) && all_typecases_compound(&toptype->union_type));

    if (!is_compound_ish)
    {
        ImGui::Text("Top type of a collection must be Compound");
    }
    else
    {
        DynArray<NameRef> names = dynarray::init<NameRef>(toptype->compound_type.members.count);

        if (tIS_COMPOUND(toptype))
        {
            for (DynArrayCount i = 0, e = toptype->compound_type.members.count; i < e; ++i)
            {
                dynarray::append(&names, toptype->compound_type.members[i].name);
            }
        }
        else
        {
            for (DynArrayCount i = 0, ie = toptype->union_type.type_cases.count; i < ie; ++i)
            {
                TypeDescriptor *t = toptype->union_type.type_cases[i];
                ASSERT(tIS_COMPOUND(t));
                CompoundType *ct = &t->compound_type;

                for (DynArrayCount j = 0, je = ct->members.count; j < je; ++j)
                {
                    std::printf("NAME: %s ", str_slice(ct->members[j].name).data);
                    bool inserted = dynarray::append_if_not_present<NameRef, NameRefIdentical>(&names, ct->members[j].name);
                    std::printf("inserted: %s\n", inserted ? "YES" : "NO");
                }
            }
        }

        ImGui::Columns(S32(names.count), "Data yay");
        ImGui::Separator();
        // Str col_label = {};

        for (DynArrayCount i = 0; i < names.count; ++i)
        {
            // str_overwrite(&col_label, str_slice(value->compound_value.members[i].name));
            ImGui::Text("%s", str_slice(names[i]).data);
            ImGui::NextColumn();
        }

        // str_free(&col_label);
        ImGui::Separator();

        // // collect and render column names
        // DynArrayCount column_count = 0;
        // if (collection->records.count > 0)
        // {
        //     Value *value = &collection->records[0]->value;

        //     column_count = value->compound_value.members.count;

        //     ImGui::Columns((int)column_count, "Data Yay");
        //     ImGui::Separator();

        //     Str col_label = {};
        //     for (DynArrayCount i = 0; i < column_count; ++i)
        //     {
        //         str_overwrite(&col_label, str_slice(value->compound_value.members[i].name));
        //         ImGui::Text("%s", col_label.data);
        //         ImGui::NextColumn();
        //     }
        //     str_free(&col_label);
        //     ImGui::Separator();
        // }

        for (DynArrayCount i = 0, ie = collection->records.count; i < ie; ++i)
        {
            ImGui::PushID(S32(i));

            Value *value = &collection->records[i]->value;
            ASSERT(vIS_COMPOUND(value)); // kind of unnecessary since we branch on this

            for (DynArrayCount j = 0, je = names.count; j < je; ++j)
            {
                ImGui::PushID(S32(j));

                CompoundValueMember *memval = find_member(value, names[j]);

                if (!memval)
                {
                    ImGui::Text("NOPE");
                }
                else
                {
                    Value *val = &memval->value;

                    switch ((TypeID::Tag)(val->typedesc->type_id))
                    {
                        case TypeID::String:
                            ImGui_InputText("##field", &val->str_val);
                            break;

                        case TypeID::Int:
                            ImGui::InputInt("##field", &val->s32_val);
                            break;

                        case TypeID::Float:
                            ImGui::InputFloat("##field", &val->f32_val);
                            break;

                        case TypeID::Bool:
                            ImGui::Checkbox("##field", &val->bool_val);
                            break;

                        default:
                            ImGui::Text("TODO: %s", TypeID::to_string(val->typedesc->type_id));
                            break;
                    }
                }

                ImGui::PopID();
                ImGui::NextColumn();
            }

            // for (DynArrayCount j = 0, memcount = value->typedesc->compound_type.members.count;
            //      j < memcount;
            //      ++j)
            // {
            //     ImGui::PushID(S32(j));

            //     Value *memval = &value->compound_value.members[j].value;
            //     TypeDescriptor *memtype = memval->typedesc;

            //     switch ((TypeID::Tag)(memtype->type_id))
            //     {
            //         case TypeID::String:
            //             ImGui_InputText("##field", &memval->str_val);
            //             break;

            //         case TypeID::Int:
            //             ImGui::InputInt("##field", &memval->s32_val);
            //             break;

            //         case TypeID::Float:
            //             ImGui::InputFloat("##field", &memval->f32_val);
            //             break;

            //         case TypeID::Bool:
            //             ImGui::Checkbox("##field", &memval->bool_val);
            //             break;

            //         default:
            //             ImGui::Text("TODO: %s", TypeID::to_string(memtype->type_id));
            //             break;
            //     }

            //     ImGui::PopID();
            //     ImGui::NextColumn();
            // }

            ImGui::PopID();
        }

    }

    ImGui::Columns(1);
    ImGui::Separator();

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
                    StrSlice name = str_slice(names->at(j));
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
    SDL_Window *window = SDL_CreateWindow("jsoneditor",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          (int)(display_mode.w * 0.75), (int)(display_mode.h * 0.75),
                                          SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    u32 window_id = SDL_GetWindowID(window);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImVec4 clear_color = ImColor(114, 144, 154);
    ImGui_ImplSdl_Init(window);

    bool show_imgui_testwindow = false;
    bool running = true;
    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
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
        }


        ImGui_ImplSdl_NewFrame(window);

        draw_imgui_json_cli(&prgstate, window);

        for (DynArrayCount i = 0, e = prgstate.editing_collections.count; i < e; ++i)
        {
            bool open = draw_collection_editor(prgstate.editing_collections[i]);
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
        SDL_Delay(1);
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplSdl_Shutdown();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    end_of_program();
    return 0;
}
