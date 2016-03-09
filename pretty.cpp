#include "pretty.h"
#include "common.h"


void pretty_print(TypeDescriptor *type_desc, int indent)
{
    TypeID::Tag type_id = (TypeID::Tag)type_desc->type_id;
    switch (type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            printf_ln("%s", to_string(type_id));
            break;

        case TypeID::Compound:
            println("{");
            indent += 2;

            for (u32 i = 0; i < type_desc->members.count; ++i)
            {
                TypeMember *member = get(type_desc->members, i);
                printf_indent(indent, "%s: ", str_slice(member->name).data);
                pretty_print(member->typedesc_ref, indent);
            }

            indent -= 2;
            println_indent(indent, "}");
            break;
    }
}


void pretty_print(Value *value)
{
    TypeDescriptor *type_desc = get_typedesc(value->typedesc_ref);

    switch ((TypeID::Tag)type_desc->type_id)
    {
        case TypeID::None:
            println("NONE");
            break;

        case TypeID::String:
            printf_ln("%s", value->str_val.data);
            break;

        case TypeID::Int:
            printf_ln("%i", value->s32_val);
            break;

        case TypeID::Float:
            printf_ln("%f", value->f32_val);
            break;

        case TypeID::Bool:
            printf_ln("%s", (value->bool_val ? "True" : "False"));
            break;

        case TypeID::Compound:
            println("Printing compounds not supported yet");
    }
}


void pretty_print(tokenizer::Token token)
{
    char token_output[256];
    size_t len = std::min((size_t)token.text.length, sizeof(token_output) - 1);
    if (len == 0)
    {
        token_output[0] = 0;
    }
    else
    {
        std::strncpy(token_output, token.text.data, len);
        token_output[len] = 0;
    }

    printf_ln("Token(%s, \"%s\")", tokenizer::to_string(token.type), token_output);
}

