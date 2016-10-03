#include "pretty.h"
#include "logging.h"
#include "common.h"


/*
good print tests

{"foo":"bar", "fee":[{"fi":[[1, 2, 3], [4, 5, 6]]}, {"fi":[[7, 8, 9], [10, 11, 12]]}]}
{"boo": {"baaa": {"foo": [1, 2, 3]}, "burr": [{"heyo":{"noway":[]}}]}}

 */


void pretty_print(TypeDescriptor *type_desc, FormatBuffer *fmt_buf, int indent)
{
    TypeID::Tag type_id = (TypeID::Tag)type_desc->type_id;
    TYPESWITCH (type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            fmt_buf->writeln(to_string(type_id));
            break;

        case TypeID::Array:
        {
            TypeDescriptor *elem_typedesc = type_desc->array_type.elem_type;
            TypeID::Tag elem_type_id = (TypeID::Tag)elem_typedesc->type_id;
            TYPESWITCH (elem_typedesc->type_id)
            {
                case TypeID::None:
                case TypeID::String:
                case TypeID::Int:
                case TypeID::Float:
                case TypeID::Bool:
                    fmt_buf->writef_ln("[%s]", to_string(elem_type_id));
                    break;

                case TypeID::Array:
                case TypeID::Compound:
                case TypeID::Union:
                {
                    fmt_buf->writeln("[");

                    indent += 2;

                    fmt_buf->write_indent(indent, "");

                    pretty_print(type_desc->array_type.elem_type, fmt_buf, indent);

                    indent -= 2;

                    fmt_buf->writeln_indent(indent, "]");
                    break;
                }
            }

            break;
        }

        case TypeID::Compound:
            if (type_desc->compound_type.members.count == 0)
            {
                fmt_buf->writeln("{}");
            }
            else
            {
                fmt_buf->writeln("{");

                indent += 2;

                for (u32 i = 0; i < type_desc->compound_type.members.count; ++i)
                {
                    CompoundTypeMember *member = &type_desc->compound_type.members[i];
                    fmt_buf->writef_indent(indent, "%s: ", str_slice(member->name).data);
                    pretty_print(member->typedesc, fmt_buf, indent);
                }

                indent -= 2;
                fmt_buf->writeln_indent(indent, "}");
            }
            break;

        case TypeID::Union:
        {
            fmt_buf->writeln("Union (");

            indent += 2;

            for (DynArrayCount i = 0,
                     count = type_desc->union_type.type_cases.count;
                 i < count;
                 ++i)
            {
                fmt_buf->write_indent(indent, "| ");
                pretty_print(type_desc->union_type.type_cases[i], fmt_buf, indent);
            }

            indent -= 2;

            fmt_buf->writeln_indent(indent, ")");
        }
    }
}


void pretty_print(TypeDescriptor *type_desc, int indent)
{
    FormatBuffer fmt_buf;
    fmt_buf.flush_on_destruct();
    pretty_print(type_desc, &fmt_buf, indent);
    fmt_buf.flush_to_log();
}


void pretty_print(Value *value, FormatBuffer *fmt_buf, int indent)
{
    TypeDescriptor *type_desc = value->typedesc;

    TYPESWITCH (type_desc->type_id)
    {
        case TypeID::None:
            fmt_buf->writef_ln("%s", "null");
            break;

        case TypeID::String:
            fmt_buf->writef_ln("'%s'", value->str_val.data);
            break;

        case TypeID::Int:
            fmt_buf->writef_ln("%i", value->s32_val);
            break;

        case TypeID::Float:
            fmt_buf->writef_ln("%f", value->f32_val);
            break;

        case TypeID::Bool:
            fmt_buf->writef_ln("%s", (value->bool_val ? "True" : "False"));
            break;

        case TypeID::Array:
            if (value->array_value.elements.count == 0)
            {
                fmt_buf->writeln("[]");
            }
            else
            {
                fmt_buf->writeln("[");

                indent += 2;


                for (DynArrayCount i = 0; i < value->array_value.elements.count; ++i)
                {
                    fmt_buf->write_indent(indent, "");
                    Value *value_element = &value->array_value.elements[i];
                    pretty_print(value_element, fmt_buf, indent);
                }

                indent -= 2;

                fmt_buf->writeln_indent(indent, "]");
            }
            break;

        case TypeID::Compound:
            if (type_desc->compound_type.members.count == 0)
            {
                fmt_buf->writeln("{}");
            }
            else
            {
                fmt_buf->writeln("{");

                indent += 2;

                for (u32 i = 0; i < value->compound_value.members.count; ++i)
                {
                    CompoundValueMember *member = &value->compound_value.members[i];
                    fmt_buf->writef_indent(indent, "'%s': ", str_slice(member->name).data);
                    pretty_print(&member->value, fmt_buf, indent);
                }

                indent -= 2;
                fmt_buf->writeln_indent(indent, "}");
            }
            break;

        case TypeID::Union:
            assert(!(bool)"There should never be a Value with type_id Union");
            break;
    }
}


void pretty_print(Value *value, int indent)
{
    FormatBuffer fmt_buf;
    fmt_buf.flush_on_destruct();
    pretty_print(value, &fmt_buf, indent);
    fmt_buf.flush_to_log();
}


void pretty_print(tokenizer::Token token)
{
    FormatBuffer fmt_buf;
    fmt_buf.flush_on_destruct();
    pretty_print(token, &fmt_buf);
    fmt_buf.flush_to_log();
}

void pretty_print(tokenizer::Token token, FormatBuffer *fmt_buf)
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

    fmt_buf->writef("Token(%s, \"%s\")", tokenizer::to_string(token.type), token_output);
}
