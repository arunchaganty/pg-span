//  -*- tab-width:4; c-basic-offset:4; indent-tabs-mode:nil;  -*-
/*
 * PostgreSQL type definitions for span type
 * Written by:
 * + Arun Chaganty <arunchaganty@gmail.com>
 * Based on the span implementation by:
 * + Sam Vilain <sam@vilain.net>
 * + Tom Davis <tom@recursivedream.com>
 * + Xavier Caron <xcaron@gmail.com>
 *
 * Copyright 2017 The pg-span Maintainers. This program is Free
 * Software; see the LICENSE file for the license conditions.
 */

#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "postgres.h"
#include "utils/builtins.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* IO methods */
Datum       span_in(PG_FUNCTION_ARGS);
Datum       span_out(PG_FUNCTION_ARGS);

Datum       span_eq(PG_FUNCTION_ARGS);
Datum       hash_span(PG_FUNCTION_ARGS);
Datum       span_ne(PG_FUNCTION_ARGS);
Datum       span_lt(PG_FUNCTION_ARGS);
Datum       span_le(PG_FUNCTION_ARGS);
Datum       span_ge(PG_FUNCTION_ARGS);
Datum       span_gt(PG_FUNCTION_ARGS);
Datum       span_cmp(PG_FUNCTION_ARGS);

/* these typecasts are necessary for passing to functions that take text */
Datum       text_to_span(PG_FUNCTION_ARGS);
Datum       span_to_text(PG_FUNCTION_ARGS);

Datum       is_span(PG_FUNCTION_ARGS);

Datum       span_smaller(PG_FUNCTION_ARGS);
Datum       span_larger(PG_FUNCTION_ARGS);

/* these functions allow users to access individual parts of the span */
Datum       get_span_doc(PG_FUNCTION_ARGS);
Datum       get_span_begin(PG_FUNCTION_ARGS);
Datum       get_span_end(PG_FUNCTION_ARGS);

/* memory/heap structure (not for binary marshalling) - fixed 48 bytes */
#define DOC_ID_LENGTH 40
typedef struct span
{
    char doc_id[DOC_ID_LENGTH];
    int32 char_begin;
    int32 char_end;
} span;
const int32 SPAN_SIZE = 8 + DOC_ID_LENGTH;

#define PG_GETARG_SPAN_P(n) (span *)PG_GETARG_POINTER(n)

// forward declarations, mostly to shut the compiler up but some are
// actually necessary.
char*   emit_span(const span* val);
span*   parse_span(const char* str, bool* err, int* msgno);
int     _span_cmp(const span* a, const span* b);

const char* ERROR_CODES[] = {
    "",
    "invalid input syntax for span: \"%s\"",
    "missing or nonpositive index: \"%s\"",
    "start index greater than end index: \"%s\"",
    "unexpected characters at end of string: \"%s\"",
    "doc id empty or too long: \"%s\"",
};


// Construct a span object from given input.
span* parse_span(const char *str, bool* err, int* msgno) { 
    span *rv = palloc(SPAN_SIZE);
    memset(rv->doc_id, 0, DOC_ID_LENGTH);

    char* end_ptr;

    end_ptr = strchr(str, ':');
    if (end_ptr == NULL) {
        if (err) *err = true;
        if (msgno) *msgno = 1;
        return NULL;
    }

    int len = end_ptr-str;
    if (len == 0 || len > 39) {
        if (err) *err = true;
        if (msgno) *msgno = 5;
        return NULL;
    }

    strncpy(rv->doc_id, str, len);
    str = end_ptr+1;

    rv->char_begin = (int32) strtol(str, &end_ptr, 10);
    if (rv->char_begin <= 0) {
        if (err) *err = true;
        if (msgno) *msgno = 2;
        return NULL;
    }
    if (*end_ptr != '-') {
        if (err) *err = true;
        if (msgno) *msgno = 1;
        return NULL;
    }

    str = end_ptr+1;
    rv->char_end = (int32_t) strtol(str, &end_ptr, 10);
    if (rv->char_end <= 0) {
        if (err) *err = true;
        if (msgno) *msgno = 2;
        return NULL;
    }
    if (rv->char_end < rv->char_begin) {
        if (err) *err = true;
        if (msgno) *msgno = 3;
        return NULL;
    }
    if (*end_ptr != 0) {
        if (err) *err = true;
        if (msgno) *msgno = 4;
        return NULL;
    }

    if (err) *err = false;
    if (msgno) *msgno = 0;

    return rv;
}

char* emit_span(const span* s) {
    int len;
    char tmpbuf[64];
    len = snprintf(tmpbuf, sizeof(tmpbuf), "%s:%d-%d", s->doc_id, s->char_begin, s->char_end);
    return pstrdup(tmpbuf);
}

/*
 * Pg bindings
 */

/* input function: C string */
PG_FUNCTION_INFO_V1(span_in);
Datum
span_in(PG_FUNCTION_ARGS)
{
    char *str = PG_GETARG_CSTRING(0);
    bool err; int msgno;

    span *result = parse_span(str, &err, &msgno);
    if (err) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg(ERROR_CODES[msgno], str)));
    }
    PG_RETURN_POINTER(result);
}

/* output function: C string */
PG_FUNCTION_INFO_V1(span_out);
Datum
span_out(PG_FUNCTION_ARGS)
{
    span* in = PG_GETARG_SPAN_P(0);
    char *result;
    result = emit_span(in);

    PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(text_to_span);
Datum
text_to_span(PG_FUNCTION_ARGS)
{
    text* sv = PG_GETARG_TEXT_PP(0);
    bool err; int msgno;
    char* str = text_to_cstring(sv);
    span* rv = parse_span(str, &err, &msgno);
    if (err) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg(ERROR_CODES[msgno], str)));
    }
    PG_RETURN_POINTER(rv);
}

PG_FUNCTION_INFO_V1(span_to_text);
Datum
span_to_text(PG_FUNCTION_ARGS)
{
    span* sv = PG_GETARG_SPAN_P(0);
    char* xxx = emit_span(sv);
    text* res = cstring_to_text(xxx);
    pfree(xxx);
    PG_RETURN_TEXT_P(res);
}

PG_FUNCTION_INFO_V1(get_span_doc);
Datum
get_span_doc(PG_FUNCTION_ARGS)
{
    span* sv = PG_GETARG_SPAN_P(0);
    text* res = cstring_to_text(sv->doc_id);
    PG_RETURN_TEXT_P(res);
}

PG_FUNCTION_INFO_V1(get_span_begin);
Datum
get_span_begin(PG_FUNCTION_ARGS)
{
    span* sv = PG_GETARG_SPAN_P(0);
    PG_RETURN_INT32(sv->char_begin);
}

PG_FUNCTION_INFO_V1(get_span_end);
Datum
get_span_end(PG_FUNCTION_ARGS)
{
    span* sv = PG_GETARG_SPAN_P(0);
    PG_RETURN_INT32(sv->char_end);
}

/* comparisons */
int _span_cmp(const span* a, const span* b)
{
    int rv;
    rv = strcmp(a->doc_id, b->doc_id);
    if (rv != 0) return rv;
    rv = (a->char_begin < b->char_begin) ? -1 : ((a->char_begin == b->char_begin) ? 0 : 1);
    if (rv != 0) return rv;
    rv = (a->char_end < b->char_end) ? -1 : ((a->char_end == b->char_end) ? 0 : 1);
    return rv;
}

PG_FUNCTION_INFO_V1(span_eq);
Datum
span_eq(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    PG_RETURN_BOOL(diff == 0);
}

PG_FUNCTION_INFO_V1(span_ne);
Datum
span_ne(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    PG_RETURN_BOOL(diff != 0);
}

PG_FUNCTION_INFO_V1(span_le);
Datum
span_le(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    PG_RETURN_BOOL(diff <= 0);
}

PG_FUNCTION_INFO_V1(span_lt);
Datum
span_lt(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    PG_RETURN_BOOL(diff < 0);
}

PG_FUNCTION_INFO_V1(span_ge);
Datum
span_ge(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    PG_RETURN_BOOL(diff >= 0);
}

PG_FUNCTION_INFO_V1(span_gt);
Datum
span_gt(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    PG_RETURN_BOOL(diff > 0);
}

PG_FUNCTION_INFO_V1(span_cmp);
Datum
span_cmp(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    PG_RETURN_INT32(diff);
}

/* from catalog/pg_proc.h */
#define hashtext 400
#define hashint4 450

/* so the '=' function can be 'hashes' */
PG_FUNCTION_INFO_V1(hash_span);
Datum
hash_span(PG_FUNCTION_ARGS)
{
    span* in = PG_GETARG_SPAN_P(0);
    Datum in_datum;
    uint32 hash = 0;
    in_datum = CStringGetTextDatum(in->doc_id);
    hash = OidFunctionCall1(hashtext, in_datum);
    hash = (hash << (7+(0<<1))) & (hash >> (25-(0<<1)));
    hash ^= OidFunctionCall1(hashint4, in->char_begin);
    hash = (hash << (7+(1<<1))) & (hash >> (25-(1<<1)));
    hash ^= OidFunctionCall1(hashint4, in->char_end);

    PG_RETURN_INT32(hash);
}

PG_FUNCTION_INFO_V1(span_larger);
Datum
span_larger(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    if (diff >= 0) {
        PG_RETURN_POINTER(a);
    }
    else {
        PG_RETURN_POINTER(b);
    }
}

PG_FUNCTION_INFO_V1(span_smaller);
Datum
span_smaller(PG_FUNCTION_ARGS)
{
    span* a = PG_GETARG_SPAN_P(0);
    span* b = PG_GETARG_SPAN_P(1);
    int diff = _span_cmp(a, b);
    if (diff <= 0) {
        PG_RETURN_POINTER(a);
    }
    else {
        PG_RETURN_POINTER(b);
    }
}

PG_FUNCTION_INFO_V1(is_span);
Datum
is_span(PG_FUNCTION_ARGS)
{
    text* sv = PG_GETARG_TEXT_PP(0);
    bool err; int msgno;
    char* str = text_to_cstring(sv);
    span* rv = parse_span(str, &err, &msgno);
    if (rv != NULL) pfree(rv);
    PG_RETURN_BOOL(!err);
}
