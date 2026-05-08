#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define cJSON_malloc(size) malloc(size)
#define cJSON_free(ptr) free(ptr)
#define cJSON_realloc(ptr, size) realloc(ptr, size)

static cJSON_Error cJSON_Error_Data = {0, ""};

cJSON_Error *cJSON_GetErrorPtr(void) { return &cJSON_Error_Data; }

static void *(*cJSON_malloc_fn)(size_t) = cJSON_malloc;
static void (*cJSON_free_fn)(void *) = cJSON_free;

void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (!hooks)
    {
        cJSON_malloc_fn = cJSON_malloc;
        cJSON_free_fn = cJSON_free;
        return;
    }
    cJSON_malloc_fn = (hooks->mallocfn) ? hooks->mallocfn : cJSON_malloc;
    cJSON_free_fn = (hooks->freefn) ? hooks->freefn : cJSON_free;
}

static cJSON *cJSON_New_Item(void)
{
    cJSON* node = (cJSON*)cJSON_malloc_fn(sizeof(cJSON));
    if (node) memset(node, 0, sizeof(cJSON));
    return node;
}

void cJSON_Delete(cJSON *c)
{
    cJSON *next;
    while (c)
    {
        next = c->next;
        if (!(c->type & cJSON_IsReference) && c->child) cJSON_Delete(c->child);
        if (!(c->type & cJSON_IsReference) && c->valuestring) cJSON_free_fn(c->valuestring);
        if (!(c->type & cJSON_StringIsConst) && c->string) cJSON_free_fn(c->string);
        cJSON_free_fn(c);
        c = next;
    }
}

cJSON *cJSON_Parse(const char *value)
{
    return cJSON_ParseWithOpts(value, 0, 0);
}

static const char *parse_number(cJSON *item, const char *num)
{
    double n = 0, sign = 1, scale = 0;
    int subscale = 0, signsubscale = 1;

    if (*num == '-') sign = -1, num++;
    if (*num == '0') num++;
    if (*num >= '1' && *num <= '9') do n = (n * 10.0) + (*num++ - '0'); while (*num >= '0' && *num <= '9');
    if (*num == '.' && num[1] >= '0' && num[1] <= '9') {
        num++;
        do n = (n * 10.0) + (*num++ - '0'), scale--; while (*num >= '0' && *num <= '9');
    }
    if (*num == 'e' || *num == 'E') {
        num++;
        if (*num == '+') num++; else if (*num == '-') signsubscale = -1, num++;
        while (*num >= '0' && *num <= '9') subscale = (subscale * 10) + (*num++ - '0');
    }

    n = sign * n * pow(10.0, (scale + subscale * signsubscale));

    item->valuedouble = n;
    item->valueint = (int)n;
    item->type = cJSON_Number;
    return num;
}

static const char *parse_string(cJSON *item, const char *str, const char **out)
{
    const char *ptr = str + 1;
    char *ptr2;
    char *out2;
    int len = 0;

    if (*str != '\"') { *out = str; return 0; }

    while (*ptr != '\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;

    out2 = (char*)cJSON_malloc_fn(len + 1);
    if (!out2) return 0;

    ptr2 = out2;
    ptr = str + 1;
    while (*ptr != '\"' && *ptr) {
        if (*ptr != '\\') *ptr2++ = *ptr++;
        else {
            ptr++;
            switch (*ptr) {
                case 'b': *ptr2++ = '\b'; break;
                case 'f': *ptr2++ = '\f'; break;
                case 'n': *ptr2++ = '\n'; break;
                case 'r': *ptr2++ = '\r'; break;
                case 't': *ptr2++ = '\t'; break;
                default: *ptr2++ = *ptr; break;
            }
            ptr++;
        }
    }
    *ptr2 = 0;
    if (*ptr == '\"') ptr++;
    item->valuestring = out2;
    item->type = cJSON_String;
    *out = ptr;
    return ptr;
}

static const char *skip(const char *in)
{
    while (in && *in && (unsigned char)*in <= 32) in++;
    return in;
}

static cJSON *parse_value(cJSON *item, const char *value);

static const char *parse_array(cJSON *item, const char *value)
{
    cJSON *child;
    if (*value != '[') { value = 0; goto fail; }

    item->type = cJSON_Array;
    value = skip(value + 1);
    if (*value == ']') { value++; goto success; }

    item->child = child = cJSON_New_Item();
    if (!item->child) goto fail;
    value = skip(parse_value(child, skip(value)));
    if (!value) goto fail;

    while (*value == ',') {
        cJSON *new_item;
        if (!(new_item = cJSON_New_Item())) goto fail;
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip(parse_value(child, skip(value + 1)));
        if (!value) goto fail;
    }

    if (*value == ']') { value++; goto success; }
fail:
    value = 0;
success:
    return value;
}

static const char *parse_object(cJSON *item, const char *value)
{
    cJSON *child;
    if (*value != '{') { value = 0; goto fail; }

    item->type = cJSON_Object;
    value = skip(value + 1);
    if (*value == '}') { value++; goto success; }

    item->child = child = cJSON_New_Item();
    if (!item->child) goto fail;
    value = skip(parse_string(child, skip(value), 0));
    if (!value) goto fail;
    child->string = child->valuestring;
    child->valuestring = 0;
    if (*value != ':') { value = 0; goto fail; }
    value = skip(parse_value(child, skip(value + 1)));
    if (!value) goto fail;

    while (*value == ',') {
        cJSON *new_item;
        if (!(new_item = cJSON_New_Item())) goto fail;
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip(parse_string(child, skip(value + 1), 0));
        if (!value) goto fail;
        child->string = child->valuestring;
        child->valuestring = 0;
        if (*value != ':') { value = 0; goto fail; }
        value = skip(parse_value(child, skip(value + 1)));
        if (!value) goto fail;
    }

    if (*value == '}') { value++; goto success; }
fail:
    value = 0;
success:
    return value;
}

static const char *parse_value(cJSON *item, const char *value)
{
    if (!value) return 0;
    if (!strncmp(value, "null", 4)) { item->type = cJSON_NULL; return value + 4; }
    if (!strncmp(value, "false", 5)) { item->type = cJSON_False; return value + 5; }
    if (!strncmp(value, "true", 4)) { item->type = cJSON_True; item->valueint = 1; return value + 4; }
    if (*value == '\"') { if (!parse_string(item, value, &value)) return 0; return value; }
    if (*value == '-' || (*value >= '0' && *value <= '9')) { return parse_number(item, value); }
    if (*value == '[') { return parse_array(item, value); }
    if (*value == '{') { return parse_object(item, value); }
    return 0;
}

cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated)
{
    cJSON *item = cJSON_New_Item();
    if (!item) return 0;

    value = skip(parse_value(item, skip(value)));
    if (!value) { cJSON_Delete(item); return 0; }

    if (require_null_terminated) {
        value = skip(value);
        if (*value) { cJSON_Delete(item); return 0; }
    }

    if (return_parse_end) *return_parse_end = (const char*)value;
    return item;
}

char *cJSON_Print(const cJSON *item)
{
    return cJSON_PrintBuffered(item, 0, 1);
}

char *cJSON_PrintUnformatted(const cJSON *item)
{
    return cJSON_PrintBuffered(item, 0, 0);
}

static char *print_value(const cJSON *item, char *ptr, int fmt);

static char *print_number(const cJSON *item, char *ptr)
{
    double d = item->valuedouble;
    if (d == 0) {
        strcpy(ptr, "0");
        return ptr + 1;
    }
    if (fabs(((double)item->valueint) - d) <= 1e-6 && d <= INT_MAX && d >= INT_MIN) {
        sprintf(ptr, "%d", item->valueint);
    } else if (fmt == 1) {
        sprintf(ptr, "%.15g", d);
    } else {
        sprintf(ptr, "%g", d);
    }
    return ptr + strlen(ptr);
}

static char *print_string(const char *str, char *ptr)
{
    *ptr++ = '\"';
    while (*str) {
        if (*str == '\"' || *str == '\\') *ptr++ = '\\';
        *ptr++ = *str++;
    }
    *ptr++ = '\"';
    *ptr = 0;
    return ptr;
}

static char *print_array(const cJSON *item, char *ptr, int fmt)
{
    cJSON *child = item->child;
    *ptr++ = '[';
    while (child) {
        ptr = print_value(child, ptr, fmt);
        if (child->next) *ptr++ = ',';
        child = child->next;
    }
    *ptr++ = ']';
    *ptr = 0;
    return ptr;
}

static char *print_object(const cJSON *item, char *ptr, int fmt)
{
    cJSON *child = item->child;
    *ptr++ = '{';
    while (child) {
        ptr = print_string(child->string, ptr);
        *ptr++ = ':';
        ptr = print_value(child, ptr, fmt);
        if (child->next) *ptr++ = ',';
        child = child->next;
    }
    *ptr++ = '}';
    *ptr = 0;
    return ptr;
}

static char *print_value(const cJSON *item, char *ptr, int fmt)
{
    switch ((item->type) & 0xFF) {
        case cJSON_NULL:    strcpy(ptr, "null"); return ptr + 4;
        case cJSON_False:   strcpy(ptr, "false"); return ptr + 5;
        case cJSON_True:    strcpy(ptr, "true"); return ptr + 4;
        case cJSON_Number:  return print_number(item, ptr);
        case cJSON_String:  return print_string(item->valuestring, ptr);
        case cJSON_Array:   return print_array(item, ptr, fmt);
        case cJSON_Object:  return print_object(item, ptr, fmt);
    }
    return ptr;
}

char *cJSON_PrintBuffered(const cJSON *item, int prebuffer, int fmt)
{
    char *out = (char*)cJSON_malloc_fn(prebuffer < 64 ? 64 : prebuffer);
    if (!out) return 0;
    print_value(item, out, fmt);
    return out;
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string)
{
    return cJSON_GetObjectItemCaseSensitive(object, string);
}

cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string)
{
    cJSON *child = object ? object->child : 0;
    while (child) {
        if (!strcmp(child->string, string)) return child;
        child = child->next;
    }
    return 0;
}

int cJSON_HasObjectItem(const cJSON *object, const char *string)
{
    return cJSON_GetObjectItemCaseSensitive(object, string) ? 1 : 0;
}

int cJSON_GetArraySize(const cJSON *array)
{
    cJSON *child = array ? array->child : 0;
    int size = 0;
    while (child) size++, child = child->next;
    return size;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int item)
{
    cJSON *child = array ? array->child : 0;
    while (child && item > 0) item--, child = child->next;
    return child;
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    cJSON *child = array->child;
    if (!item) return;
    if (!child) { array->child = item; }
    else {
        while (child && child->next) child = child->next;
        child->next = item;
        item->prev = child;
    }
    array->size++;
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    if (!item) return;
    if (item->string) cJSON_free_fn(item->string);
    item->string = (char*)(cJSON_malloc_fn(strlen(string) + 1));
    if (!item->string) return;
    strcpy(item->string, string);
    cJSON_AddItemToArray(object, item);
}

void cJSON_AddNumberToObject(cJSON *object, const char *name, double number)
{
    cJSON *n = cJSON_CreateNumber(number);
    cJSON_AddItemToObject(object, name, n);
}

void cJSON_AddStringToObject(cJSON *object, const char *name, const char *string)
{
    cJSON *s = cJSON_CreateString(string);
    cJSON_AddItemToObject(object, name, s);
}

void cJSON_AddBoolToObject(cJSON *object, const char *name, int boolean)
{
    cJSON *b = cJSON_CreateBool(boolean);
    cJSON_AddItemToObject(object, name, b);
}

void cJSON_AddNullToObject(cJSON *object, const char *name)
{
    cJSON *n = cJSON_CreateNull();
    cJSON_AddItemToObject(object, name, n);
}

cJSON *cJSON_CreateNull(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_NULL;
    return item;
}

cJSON *cJSON_CreateTrue(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_True;
    return item;
}

cJSON *cJSON_CreateFalse(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_False;
    return item;
}

cJSON *cJSON_CreateBool(int b)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = b ? cJSON_True : cJSON_False;
    return item;
}

cJSON *cJSON_CreateNumber(double num)
{
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;
        item->valueint = (int)num;
    }
    return item;
}

cJSON *cJSON_CreateString(const char *string)
{
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_String;
        item->valuestring = (char*)(cJSON_malloc_fn(strlen(string) + 1));
        if (item->valuestring) strcpy(item->valuestring, string);
        else { cJSON_Delete(item); return 0; }
    }
    return item;
}

cJSON *cJSON_CreateArray(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_Array;
    return item;
}

cJSON *cJSON_CreateObject(void)
{
    cJSON *item = cJSON_New_Item();
    if (item) item->type = cJSON_Object;
    return item;
}