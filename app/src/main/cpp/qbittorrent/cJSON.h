#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

#define cJSON_IsReference 1
#define cJSON_StringIsConst 2

#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6
#define cJSON_Raw 7

#define cJSON_FirstEntry(q) ((q) ? (q)->child : NULL)
#define cJSON_NextEntry(e) ((e) ? (e)->next : NULL)
#define cJSON_GetArraySize(a) ((a) ? (a)->size : 0)
#define cJSON_GetArrayItem(a, i) ((a) && (i) <= (a)->size ? (a)->child + i : NULL)
#define cJSON_GetObjectItem(obj, s) cJSON_GetObjectItemCaseSensitive((obj), (s))
#define cJSON_HasObjectItem(obj, s) ((cJSON_GetObjectItemCaseSensitive((obj), (s)) != NULL) ? 1 : 0)

#define cJSON_IsInvalid(x) (((x) != NULL) && (((x)->type & 0xFF) == 0))

typedef struct cJSON
{
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
    int size;
    int memsize;
    int length;
    int offset;
    char *buffer;
    unsigned char imm;
} cJSON;

typedef struct cJSON_Hooks
{
    void *(*mallocfn)(size_t sz);
    void (*freefn)(void *ptr);
} cJSON_Hooks;

typedef struct cJSON_Error
{
    unsigned char offset;
    char errmsg[32];
} cJSON_Error;

extern cJSON_Error cJSON_GetErrorPtr(void);

void cJSON_InitHooks(cJSON_Hooks* hooks);
cJSON *cJSON_Parse(const char *value);
char *cJSON_Print(const cJSON *item);
char *cJSON_PrintUnformatted(const cJSON *item);
char *cJSON_PrintBuffered(const cJSON *item, int prebuffer, int fmt);
cJSON *cJSON_ParseWithLength(const char *value, size_t buffer_length);
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated);
cJSON *cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, int require_null_terminated);
void cJSON_Delete(cJSON *c);
cJSON *cJSON_GetArrayItem(cJSON *array, int item);
int cJSON_GetArraySize(cJSON *array);
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string);
int cJSON_HasObjectItem(const cJSON *object, const char *string);
const char *cJSON_GetErrorPtr(void);

cJSON *cJSON_CreateNull(void);
cJSON *cJSON_CreateTrue(void);
cJSON *cJSON_CreateFalse(void);
cJSON *cJSON_CreateBool(int b);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(const char *string);
cJSON *cJSON_CreateRaw(const char *raw);
cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateObject(void);

void cJSON_AddItemToArray(cJSON *array, cJSON *item);
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
void cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);
void cJSON_AddNumberToObject(cJSON *object, const char *name, double number);
void cJSON_AddStringToObject(cJSON *object, const char *name, const char *string);
void cJSON_AddRawToObject(cJSON *object, const char *name, const char *raw);
void cJSON_AddBoolToObject(cJSON *object, const char *name, int boolean);
void cJSON_AddNullToObject(cJSON *object, const char *name);

typedef int cJSON_Bool;

cJSON *cJSON_DetachItemFromArray(cJSON *array, int which);
void cJSON_DeleteItemFromArray(cJSON *array, int which);
cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string);
void cJSON_DeleteItemFromObject(cJSON *object, const char *string);

cJSON *cJSON_Duplicate(const cJSON *item, int recurse);
cJSON *cJSON_DuplicateCaseSensitive(const cJSON *item, int recurse);
cJSON_Bool cJSON_Compare(const cJSON *a, const cJSON *b, cJSON_Bool case_sensitive);

void cJSON_Minify(char *json);

char *cJSON_PrintPreallocated(cJSON *item, char *buf, const int len, const int fmt);
cJSON *cJSON_ParseWithLength(const char *value, size_t buffer_length);
cJSON *cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, int require_null_terminated);

#define cJSON_IsNumber(x) (((x) != NULL) && (((x)->type & 0xFF) == cJSON_Number))
#define cJSON_IsString(x) (((x) != NULL) && (((x)->type & 0xFF) == cJSON_String))
#define cJSON_IsArray(x) (((x) != NULL) && (((x)->type & 0xFF) == cJSON_Array))
#define cJSON_IsObject(x) (((x) != NULL) && (((x)->type & 0xFF) == cJSON_Object))
#define cJSON_IsRaw(x) (((x) != NULL) && (((x)->type & 0xFF) == cJSON_Raw))

#define cJSON_IsNull(x) (((x) != NULL) && (((x)->type & 0xFF) == cJSON_NULL))
#define cJSON_IsTrue(x) (((x) != NULL) && (((x)->type & 0xFF) == cJSON_True))
#define cJSON_IsFalse(x) (((x) != NULL) && (((x)->type & 0xFF) == cJSON_False))
#define cJSON_IsBool(x) (((x) != NULL) && ((((x)->type & 0xFF) == cJSON_True) || (((x)->type & 0xFF) == cJSON_False)))

#ifdef __cplusplus
}
#endif