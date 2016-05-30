#include "dynarray.h"
#include "hashtable.h"
#include "clicommands.h"
#include "programstate.h"
#include "logging.h"
#include "platform.h"
#include "formatbuffer.h"
#include "pretty.h"
#include "types.h"


void exec_command(ProgramState *prgstate, StrSlice name, DynArray<Value> args)
{
    CliCommand *cmd = ht_find(&prgstate->command_map, name);

    if (!cmd)
    {
        logf_ln("Command '%s' not found", name.data);
        return;
    }

    cmd->fn(prgstate, cmd->userdata, args);
}



void register_command(ProgramState *prgstate, StrSlice name, CliCommandFn *fnptr, void *userdata)
{
    assert(fnptr);
    CliCommand cmd = {fnptr, userdata};
    Str allocated_name = str(name);
    if (ht_set(&prgstate->command_map, str_slice(allocated_name), cmd))
    {
        logf_ln("Warning, overriding command: %s", allocated_name.data);
        str_free(&allocated_name);
    }
}


void register_command(ProgramState *prgstate, const char *name, CliCommandFn *fnptr, void *userdata)
{
    register_command(prgstate, str_slice(name), fnptr, userdata);
}


CLI_COMMAND_FN_SIG(say_hello)
{
    UNUSED(prgstate);
    UNUSED(userdata);
    UNUSED(args);

    logln("You are calling the say_hello command");
}


CLI_COMMAND_FN_SIG(cls)
{
    UNUSED(prgstate);
    UNUSED(userdata);
    UNUSED(args);
    clear_concatenated_log();
}


CLI_COMMAND_FN_SIG(curdir)
{
    UNUSED(prgstate);
    UNUSED(userdata);

    if (args.count != 0)
    {
        logln("Usage: curdir");
        return;
    }

    Str str = {};
    PlatformError error = current_dir(&str);

    if (error.is_error())
    {
        logf_ln("Error getting current directory: %s", error.message.data);
    }
    else
    {
        logf_ln("Current directory is %s", str.data);
    }

    error.release();
}


CLI_COMMAND_FN_SIG(listdir)
{
    UNUSED(prgstate);
    UNUSED(userdata);
    // UNUSED(args);

    if (args.count == 0 || get_typedesc(args[0].typeref)->type_id != TypeID::String)
    {
        logln("Usage: listdir \"path/to/directory\"");
        return;
    }

    DirLister dirlister(str_slice(args[0].str_val));

    while (dirlister.next())
    {

        logf("[%02lli %s] %s, fullpath: %s",
             dirlister.stream_loc,
             (dirlister.current.is_file ? "F" : "D"),
             dirlister.current.name.data,
             dirlister.current.access_path.data);
    }

    if (dirlister.has_error())
    {
        logf_ln("Error listing directories: %s", dirlister.error.message.data);
    }
}


CLI_COMMAND_FN_SIG(changedir)
{
    UNUSED(prgstate);
    UNUSED(userdata);

    if (args.count == 0 || get_typedesc(args[0].typeref)->type_id != TypeID::String)
    {
        logln("Usage: listdir \"path/to/directory\"");
        return;
    }

    PlatformError error = change_dir(args[0].str_val);

    if (error.is_error())
    {
        logf_ln("Error changing directories: %s", error.message.data);
    }
    else
    {
        logln("Ok");
    }
}


CLI_COMMAND_FN_SIG(loaddir)
{
    UNUSED(prgstate);
    UNUSED(userdata);
    UNUSED(args);

    logln("not implemented yet");
}


CLI_COMMAND_FN_SIG(list_allocs)
{
    UNUSED(prgstate);
    UNUSED(userdata);
    UNUSED(args);

    mem::IAllocator *allocator = mem::default_allocator();
    allocator->log_allocations();
}


CLI_COMMAND_FN_SIG(checktype)
{
    UNUSED(userdata);

    if (args.count != 2)
    {
        logf_ln("Error: expected 2 arguments, got %i instead", args.count);
        return;
    }

    Value *name_arg = &args[0];
    TypeDescriptor *type_desc = get_typedesc(name_arg->typeref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    TypeRef *ptr_typeref = ht_find(&prgstate->type_map, str_slice(name_arg->str_val));
    if (!ptr_typeref)
    {
        logf_ln("No value bound to name: '%s'", name_arg->str_val.data);
        return;
    }
    TypeRef typeref = *ptr_typeref;

    // fmt_buf.writeln("");
    Value *value = &args[1];
    // fmt_buf.write("Value: ");
    // pretty_print(value, &fmt_buf);
    // fmt_buf.writef("Parsed value's type: %i ", value->typeref.index);
    // pretty_print(value->typeref, &fmt_buf);

    TypeCheckInfo check = check_type_compatible(value->typeref, typeref);

    if (check.passed)
    {
        logln("passed");
    }
    else
    {
        FormatBuffer fmtbuf;
        fmtbuf.flush_on_destruct();

        fmtbuf.writef_ln("failed: %s", to_string(check.result));

        fmtbuf.write("Named type: ");
        pretty_print(typeref, &fmtbuf, 0);
        fmtbuf.write("\nInput value's type: ");
        pretty_print(value->typeref, &fmtbuf, 0);
    }
}


CLI_COMMAND_FN_SIG(print_type)
{
    UNUSED(userdata);

    if (args.count != 1)
    {
        logf_ln("Error: expected 1 argument, got %i instead", args.count);
        return;
    }

    Value *name_arg = &args[0];
    TypeDescriptor *type_desc = get_typedesc(name_arg->typeref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    TypeRef *typeref = ht_find(&prgstate->type_map, str_slice(name_arg->str_val));
    if (typeref)
    {
        pretty_print(*typeref);
    }
    else
    {
        logf_ln("No value bound to name: '%s'", name_arg->str_val.data);
    }
}


CLI_COMMAND_FN_SIG(bindinfer)
{
    UNUSED(userdata);

    if (args.count != 2)
    {
        logf_ln("Error: expected 2 arguments, got %i instead", args.count);
        return;
    }

    Value *name_arg = &args[0];

    TypeDescriptor *type_desc = get_typedesc(name_arg->typeref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    // This is maybe a bit too manual, but I want everything to be POD, no destructors
    StrToTypeMap::Entry *entry;
    bool was_occupied = ht_find_or_add_entry(&entry, &prgstate->type_map, str_slice(name_arg->str_val));
    if (!was_occupied)
    {
        // allocate dedicated space
        Str allocated = str(entry->key);
        entry->key = str_slice(allocated);
    }

    entry->value = args[1].typeref;

    FormatBuffer fbuf;
    fbuf.flush_on_destruct();
    fbuf.writef("Bound '%s' to type: ", entry->key.data);
    pretty_print(entry->value, &fbuf);
}


CLI_COMMAND_FN_SIG(print_values)
{
    UNUSED(prgstate);
    UNUSED(userdata);

    if (args.count < 1)
    {
        logln("No arguments");
        return;
    }

    FormatBuffer fmt_buf;
    fmt_buf.flush_on_destruct();

    for (u32 i = 0; i < args.count; ++i)
    {
        fmt_buf.writeln("");
        Value *value = &args[i];
        fmt_buf.write("Value: ");
        pretty_print(value, &fmt_buf);
        fmt_buf.writef("Parsed value's type: %i ", value->typeref.index);
        pretty_print(value->typeref, &fmt_buf);
    }
}


CLI_COMMAND_FN_SIG(get_value_type)
{
    UNUSED(userdata);

    if (args.count < 1)
    {
        logln("Error: expected 1 argument");
        return;
    }

    Value *name_arg = &args[0];
    TypeDescriptor *type_desc = get_typedesc(name_arg->typeref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    Value *value = ht_find(&prgstate->value_map, str_slice(name_arg->str_val));
    if (value)
    {
        pretty_print(value->typeref);
    }
    else
    {
        logf_ln("No value bound to name: '%s'", name_arg->str_val.data);
    }


}


CLI_COMMAND_FN_SIG(get_value)
{
    UNUSED(userdata);

    if (args.count < 1)
    {
        logln("Error: expected 1 argument");
        return;
    }

    Value *name_arg = &args[0];
    TypeDescriptor *type_desc = get_typedesc(name_arg->typeref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    Value *value = ht_find(&prgstate->value_map, str_slice(name_arg->str_val));
    if (value)
    {
        pretty_print(value);
    }
    else
    {
        logf_ln("No value bound to name: '%s'", name_arg->str_val.data);
    }
}


CLI_COMMAND_FN_SIG(set_value)
{
    UNUSED(userdata);

    if (args.count < 2)
    {
        logf_ln("Error: expected 2 arguments, got %i instead", args.count);
        return;
    }

    Value *name_arg = &args[0];

    TypeDescriptor *type_desc = get_typedesc(name_arg->typeref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    // This is maybe a bit too manual, but I want everything to be POD, no destructors
    StrToValueMap::Entry *entry;
    bool was_occupied = ht_find_or_add_entry(&entry, &prgstate->value_map, str_slice(name_arg->str_val));
    if (!was_occupied)
    {
        // allocate dedicated space
        Str allocated = str(entry->key);
        entry->key = str_slice(allocated);
    }

    entry->value = clone(&args[1]);

    FormatBuffer fbuf;
    fbuf.flush_on_destruct();
    fbuf.writeln("Storing value:");
    pretty_print(&entry->value, &fbuf);
}


CLI_COMMAND_FN_SIG(find_type)
{
    UNUSED(userdata);

    if (args.count == 0)
    {
        logf_ln("Expected one argument, got zero instead");
        return;
    }

    Value *arg = &args[0];

    if (! typeref_identical(arg->typeref, prgstate->prim_string))
    {
        TypeDescriptor *argtype = get_typedesc(arg->typeref);
        logf_ln("Argument must be a string, got a %s intead",
                  TypeID::to_string(argtype->type_id));
        return;
    }

    TypeRef typeref = find_typeref_by_name(prgstate, arg->str_val);
    if (typeref.index)
    {
        pretty_print(typeref);
    }
    else
    {
        logf_ln("Type not found: %s", arg->str_val.data);
    }
}


CLI_COMMAND_FN_SIG(list_args)
{
    UNUSED(prgstate);
    UNUSED(userdata);

    logln("list_args:");
    for (u32 i = 0; i < args.count; ++i)
    {
        Value *val = &args[i];
        pretty_print(val->typeref);
        pretty_print(val);
    }
}


CLI_COMMAND_FN_SIG(loadjson)
{
    UNUSED(prgstate);
    UNUSED(userdata);
    UNUSED(args);

    if (args.count != 1)
    {
        log("Usage: loadjson \"<path/to/directory/with/json/files>\"");
    }

    DynArray<Value> loaded_values = {};
    ParseResult parse_result = load_json_dir(&loaded_values, prgstate, str_slice(args[0].str_val.data));

    switch (parse_result.status)
    {
        case ParseResult::Eof:
            log("File was empty");
            break;

        case ParseResult::Failed:
            break;

        case ParseResult::Succeeded:
        {
            FormatBuffer fmt_buf;
            fmt_buf.flush_on_destruct();

            for (u32 i = 0; i < loaded_values.count; ++i)
            {
                fmt_buf.writeln("");
                Value *value = &loaded_values[i];
                fmt_buf.write("Value: ");
                pretty_print(value, &fmt_buf);
                fmt_buf.writef("Parsed value's type: %i ", value->typeref.index);
                pretty_print(value->typeref, &fmt_buf);
                value_free(value);
            }
            break;
        }
    }
}


CLI_COMMAND_FN_SIG(catfile)
{
    UNUSED(prgstate);
    UNUSED(userdata);

    if (args.count != 1 || get_typedesc(args[0].typeref)->type_id != TypeID::String)
    {
        logln("Usage: catfile \"<filename>\"");
        return;
    }

    Str dest = {};
    FileReadResult rr = read_text_file(&dest, args[0].str_val.data);

    if (rr.error_kind != FileReadResult::NoError)
    {
        logf_ln("Failed to read file '%s': %s", args[0].str_val.data, rr.platform_error.message.data);
    }
    else
    {
        logln(dest.data);
    }

    rr.release();
}


CLI_COMMAND_FN_SIG(abspath)
{
    UNUSED(prgstate);
    UNUSED(userdata);
    UNUSED(args);

    for (DynArrayCount i = 0; i < args.count; ++i)
    {
        if (get_typedesc(args[i].typeref)->type_id != TypeID::String)
        {
            logf("Argument %i was a string\n Expected usage: abspath \"<path1\" [, \"<path2>\"]...", i);
        }
    }

    Str dest = {};
    for (DynArrayCount i = 0; i < args.count; ++i)
    {
        PlatformError err = resolve_path(OUTPARAM &dest, args[i].str_val.data);
        if (err.is_error())
        {
            logf("Error resolving path %s: %s", args[i].str_val.data, err.message.data);
        }
        else
        {
            log(dest.data);
        }
        err.release();
    }
}


// CLI_COMMAND_FN_SIG($newcommandname)
// {
//     UNUSED(prgstate);
//     UNUSED(userdata);
//     UNUSED(args);
// }



void init_cli_commands(ProgramState *prgstate)
{
    REGISTER_COMMAND(prgstate, say_hello, nullptr);
    REGISTER_COMMAND(prgstate, list_args, nullptr);
    REGISTER_COMMAND(prgstate, find_type, nullptr);
    REGISTER_COMMAND(prgstate, set_value, nullptr);
    REGISTER_COMMAND(prgstate, get_value, nullptr);
    REGISTER_COMMAND(prgstate, get_value_type, nullptr);
    REGISTER_COMMAND(prgstate, print_values, nullptr);
    REGISTER_COMMAND(prgstate, bindinfer, nullptr);
    REGISTER_COMMAND(prgstate, print_type, nullptr);
    REGISTER_COMMAND(prgstate, checktype, nullptr);
    REGISTER_COMMAND(prgstate, list_allocs, nullptr);
    REGISTER_COMMAND(prgstate, listdir, nullptr);
    REGISTER_COMMAND(prgstate, changedir, nullptr);
    REGISTER_COMMAND(prgstate, curdir, nullptr);
    REGISTER_COMMAND(prgstate, cls, nullptr);
    REGISTER_COMMAND(prgstate, catfile, nullptr);
    REGISTER_COMMAND(prgstate, loadjson, nullptr);
    REGISTER_COMMAND(prgstate, abspath, nullptr);
    // REGISTER_COMMAND(prgstate, $COMMAND, nullptr);
}
