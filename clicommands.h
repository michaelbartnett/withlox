// -*- c++ -*-

#ifndef CLICOMMANDS_H

#include "str.h"
#include "dynarray.h"
#include "typesys.h"

struct ProgramState;

#define CLI_COMMAND_FN_SIG(name) void name(ProgramState *prgstate, void *userdata, DynArray<Value> args)
typedef CLI_COMMAND_FN_SIG(CliCommandFn);

#define CLI_COMMAND_FN_SIG(name) void name(ProgramState *prgstate, void *userdata, DynArray<Value> args)
typedef CLI_COMMAND_FN_SIG(CliCommandFn);


struct CliCommand
{
    CliCommandFn *fn;
    void *userdata;
};


void register_command(ProgramState *prgstate, StrSlice name, CliCommandFn *fnptr, void *userdata);
void register_command(ProgramState *prgstate, const char *name, CliCommandFn *fnptr, void *userdata);
#define REGISTER_COMMAND(prgstate, fn_ident, userdata) register_command((prgstate), # fn_ident, &fn_ident, userdata)


void exec_command(ProgramState *prgstate, StrSlice name, DynArray<Value> args);
void init_cli_commands(ProgramState *prgstate);


#define CLICOMMANDS_H
#endif
