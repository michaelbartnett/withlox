// -*- c++ -*-
#ifndef PRETTY_H

#include "typesys.h"

#include "tokenizer.h"
#include "logging.h"
#include "formatbuffer.h"

void pretty_print(TypeDescriptor *type_desc, int indent = 0);
void pretty_print(TypeDescriptor *type_desc, int indent);
void pretty_print(TypeDescriptor *type_desc, FormatBuffer *fmt_buf, int indent = 0);
void pretty_print(Value *value, int indent = 0);
void pretty_print(Value *value, FormatBuffer *fmt_buf, int indent = 0);
void pretty_print(tokenizer::Token token);
void pretty_print(tokenizer::Token token, FormatBuffer *fmt_buf);


#define PRETTY_H
#endif
