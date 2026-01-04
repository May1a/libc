#pragma once
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif

#define $private static

#define $unreachable ({                                                      \
    fprintf(stderr, "Reached unreachable code %s:%d\n", __FILE__, __LINE__); \
    __builtin_unreachable();                                                 \
})

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if !(__has_builtin(auto))
#define auto __auto_type
#endif

#define $cleanup(cleanupFn) __attribute__((cleanup(cleanupFn)))

#if __has_builtin(__builtin_debugtrap)
#define $breakpoint __builtin_debugtrap()
#else
#define $breakpoint abort()
#endif

#if __has_builtin(__builtin_dump_struct)
#define $dumpStruct __builtin_dump_struct
#else
#ifndef $MLIB_Wno_STRUCT_DUMPING_UNSUPPORTED
#warning Your compiler does not support `__builtin_dump_struct`\
             dumping structs will not work!\
             Disable this warning with `$MLIB_Wno_STRUCT_DUMPING_UNSUPPORTED`"
#endif
#define $dumpStruct(_$structPtr_, _$printfn_, ...) _$printfn_("WARNING: Your compiler does not have support for dumping structs\n");
#endif
#if __has_builtin(__nullptr) && !__has_builtin(nullptr)
#define nullptr __nullptr
#elif !__has_builtin(__nullptr) && !__has_builtin(nullptr)
#define nullptr NULL
#endif

#if !defined(OVERWRITE_Wextra_REQUIREMENT) && !__has_warning("-Wextra")
#error You forgot to add `-Wextra` to the compiler options\
       If your compiler does not support checking for warnings\
       you can overwrite this error by defining `OVERWRITE_Wextra_REQUIREMENT`
#endif

#if !defined(OVERWRITE_Wall_REQUIREMENT) && !__has_warning("-Wall")
#error You forgot to add `-Wall` to the compiler options\
       If your compiler does not support checking for warnings\
       you can overwrite this error by defining `OVERWRITE_Wall_REQUIREMENT`
#endif

#define $str(strLit$) ({                   \
    size_t litLen = sizeof(strLit$) - 1;   \
    size_t strCap = (litLen + 1) * 2;      \
    char* strAlloc = malloc(strCap);       \
    $assert(strAlloc);                     \
    memcpy(strAlloc, strLit$, litLen + 1); \
    (Str) {                                \
        .items = strAlloc,                 \
        .len = litLen,                     \
        .cap = strCap,                     \
    };                                     \
})

#define $arr(T, ...)                                 \
    (Array##T)                                       \
    {                                                \
        .len = sizeof((T[])__VA_ARGS__) / sizeof(T), \
        .items = ((T[])__VA_ARGS__)                  \
    }

#define $arrList(T, ...) ({                \
    auto arr = $arr(T, __VA_ARGS__);       \
    size_t cap = arr.len * 2;              \
    T* itemsPtr = malloc(cap * sizeof(T)); \
    $assert(itemsPtr);                     \
    memcpy(itemsPtr, arr.items, arr.len);  \
    (ArrayList##T) {                       \
        .items = itemsPtr,                 \
        .len = arr.len,                    \
        .cap = cap,                        \
    };                                     \
})

#define $assert(...)                                                     \
    if (!(__VA_ARGS__)) {                                                \
        fprintf(stderr, "Assert failed in %s:%d\n", __FILE__, __LINE__); \
        $unreachable;                                                    \
    }

#if !__cplusplus
#define or ||
#define and &&
#endif

/*
 * @brief Appends a single item to an ArrayList
 * @category Collections
 * @macro
 * @generic T
 * @param al : *ArrayList[T] - Pointer to the array list
 * @param item : T - The item to append
 * @requires al != NULL
 * @see $appendMany
 * @see $ArrayList
 */
#define $append(al, item) ({                                                                       \
    if ((al)->cap <= ((al)->len + 1)) {                                                            \
        size_t newCap = (al)->cap * 2;                                                             \
        typeof((al)->items) newPtr = realloc((void*)(al)->items, newCap * sizeof((al)->items[0])); \
        $assert(newPtr);                                                                           \
        (al)->cap = newCap;                                                                        \
        (al)->items = newPtr;                                                                      \
    }                                                                                              \
    (al)->items[(al)->len++] = item;                                                               \
})

typedef struct {
    void* items;
    size_t len;
    size_t cap;
} ArrayList;

typedef struct {
    void* items;
    size_t len;
} Array;

#define $initArrayList(Type) ({              \
    size_t cap = 4;                          \
    Type* item = malloc(sizeof(Type) * cap); \
    $assert(item);                           \
    (ArrayList##Type) {                      \
        .items = item,                       \
        .len = 0,                            \
        .cap = cap,                          \
    };                                       \
})

#define $freeAL(al) ({     \
    (al)->len = 0;         \
    (al)->cap = 0;         \
    free((al)->items);     \
    (al)->items = nullptr; \
})

void _$INTERNAL$freeALListAsVoidPtr(void* alAsVoidPtr)
{
    $freeAL((ArrayList*)alAsVoidPtr);
}

#define $autoAL $cleanup(_$INTERNAL$freeALListAsVoidPtr) auto

#define $autoDA $autoAL

// TODO: Refactor this macro
#define $reverseArrayList(al) ({                                                     \
    $assert((al)->len >= 1);                                                         \
    typeof((al)->items) newPtr = malloc(sizeof(typeof((al)->items[0])) * (al)->len); \
    int j = 0;                                                                       \
    for (int i = ((al)->len - 1); i < (al)->len; (i--, j++)) {                       \
        newPtr[i] = (al)->items[j];                                                  \
    }                                                                                \
    free((al)->items);                                                               \
    (al)->items = newPtr;                                                            \
})

/*
 * @brief Appends multiple items from an array to an ArrayList
 * @category Collections
 * @macro
 * @generic T
 * @param al : *ArrayList[T] - Pointer to the array list
 * @param items : Array[T] - Array with .items and .len fields
 * @requires al != NULL
 * @requires items.len > 0
 * @see $append
 */
#define $appendMany(al, items$) ({                         \
    if ((al)->cap < ((al)->len + items$.len)) {            \
        size_t newCap = (al->len + items$.len) * 2;        \
        void* newPtr = realloc(al->items, newCap);         \
        $assert(newPtr);                                   \
        al->items = newPtr;                                \
        al->cap = newCap;                                  \
    }                                                      \
    memcpy(al->items + al->len, items$.items, items$.len); \
    al->len += items$.len;                                 \
})

typedef struct {
    char* items;
    size_t len;
    size_t cap;
} Str;

typedef struct {
    const char* items;
    size_t len;
} SV;

void appendToStr(Str* str, SV svToAppend)
{
    if (svToAppend.len < 1) {
        printf("[WARNING]: Cannot append empty string\n");
        return;
    }
    if (str->cap < (str->len + svToAppend.len)) {
        size_t newCap = (str->len + svToAppend.len) * 2;
        char* newPtr = realloc(str->items, newCap);
        $assert(newPtr);
        str->items = newPtr;
        str->cap = newCap;
    }
    memcpy(str->items + str->len, svToAppend.items, svToAppend.len);
    str->len += svToAppend.len;
}

#define SVFmt "%.*s"

#define $fmtsv(sv$SV) (int)sv$SV.len, sv$SV.items

SV svFromCstr(const char* cstr)
{
    $assert(cstr);
    return (SV) { .items = cstr, .len = strlen(cstr) };
}

#define $defArrayList(T) \
    typedef struct {     \
        T* items;        \
        size_t len;      \
        size_t cap;      \
    } ArrayList##T;      \
    typedef struct {     \
        T* items;        \
        size_t len;      \
    } Array##T

/*
 * @brief Alias for $defArrayList
 * @category Collections
 * @macro
 * @alias $defArrayList
 * @see $defArrayList
 */
#define $defArray(T) $defArrayList(T)

$defArrayList(char);

$defArrayList(int);
$defArrayList(size_t);
$defArrayList(uint32_t);
$defArrayList(int32_t);
$defArrayList(uint16_t);
$defArrayList(int16_t);
$defArrayList(int8_t);
$defArrayList(uint8_t);

$defArrayList(Str);
$defArrayList(SV);

// ============================================================================
// String utilities
// ============================================================================

/*
 * @brief Checks if a character is whitespace
 * @category Strings
 */
bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
 * @brief Checks if a character is a digit
 * @category Strings
 */
bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

/*
 * @brief Checks if a character is alphabetic
 * @category Strings
 */
bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/*
 * @brief Checks if a character is alphanumeric or underscore
 * @category Strings
 */
bool isAlnum(char c)
{
    return isAlpha(c) or isDigit(c) or c == '_';
}

/*
 * @brief Creates an SV from a C string literal
 * @category Strings
 */
#define $sv(strLit) (SV) { .items = (strLit), .len = sizeof(strLit) - 1 }

/*
 * @brief Compares SV with another SV (macro for string literals)
 * @category Strings
 */
#define svEqLit(sv, lit) svEq(sv, $sv(lit))

/*
 * @brief Checks if SV starts with prefix SV
 * @category Strings
 */
bool svStartsWithSV(SV sv, SV prefix)
{
    if (sv.len < prefix.len)
        return false;
    return memcmp(sv.items, prefix.items, prefix.len) == 0;
}
/*
 * @brief Checks if SV starts with prefix Str
 * @category Strings
 */
bool svStartsWithStr(SV sv, Str prefix)
{
    if (sv.len < prefix.len)
        return false;
    return memcmp(sv.items, prefix.items, prefix.len) == 0;
}

#define svStartsWith(sv, prefix) _Generic((prefix), \
    SV: svStartsWithSV,                             \
    Str: svStartsWithStr,                           \
    default: static_assert("Incorrect type passed to `svStartsWith`, please pass either `Str` or `SV`"))(sv, prefix)

/*
 * @brief Checks if Str starts with prefix Str
 * @category Strings
 */
bool StrStartsWithStr(Str str, Str prefix)
{
    if (str.len < prefix.len)
        return false;
    return memcmp(str.items, prefix.items, prefix.len) == 0;
}
/*
 * @brief Checks if Str starts with prefix Str
 * @category Strings
 */
bool StrStartsWithSV(Str str, SV prefix)
{
    if (str.len < prefix.len)
        return false;
    return memcmp(str.items, prefix.items, prefix.len) == 0;
}

/*
 * @brief ensures that a SV is null terminated
 * @category Strings
 */
void ensureSVNullTermination(SV sv)
{
    if (sv.items[sv.len] != '\0') {
        char* tmpBuf = malloc(sv.len + 1);
        $assert(tmpBuf);
        memcpy(tmpBuf, sv.items, sv.len);
        tmpBuf[sv.len] = '\0';
        sv.items = tmpBuf;
    }
}

void ensureStrNullTermination(Str str)
{
    if (str.items[str.len] != '\0') {
        char* tmpBuf = malloc(str.len + 1);
        $assert(tmpBuf);
        memcpy(tmpBuf, str.items, str.len);
        tmpBuf[str.len] = '\0';
        str.items = tmpBuf;
    }
}

/*
 * @brief ensures null termination (either SV or Str)
 * @category Strings
 */
#define ensureNullTermination(s) _Generic((s), \
    Str: ensureStrNullTermination,             \
    SV: ensureSVNullTermination)(s)

/*
 * @brief Checks if SV starts with a string literal
 * @category Strings
 */
#define svStartsWithLit(sv, lit) svStartsWith(sv, $sv(lit))

/*
 * @brief Creates an SV slice from an existing SV
 * @category Strings
 * @see svSliceTo
 */
SV svSlice(SV sv, size_t start, size_t len)
{
    $assert(start < sv.len);
    if (start + len > sv.len)
        len = sv.len - start;
    return (SV) { .items = sv.items + start, .len = len };
}

void svSliceMut(SV* sv, size_t start, size_t len)
{
    $assert(start < sv->len);
    sv->items += start;
    sv->len = len;
}

/*
 * @brief Creates an SV slice from the start of an existing SV to the specified end
 * @category Strings
 * @see svSlice
 */
SV svSliceTo(SV sv, size_t end)
{
    return (SV) {
        .len = end,
        .items = sv.items,
    };
}

void svSliceToMut(SV* sv, size_t end)
{
    sv->len = end;
}

/*
 * @brief Creates an SV from a portion of a string after a given offset
 * @category Strings
 * @see svSliceTo
 * @see svSlice
 */
SV svSliceFrom(SV sv, size_t start)
{
    $assert(start < sv.len);
    return (SV) { .items = sv.items + start, .len = sv.len - start };
}

void svSliceFromMut(SV* sv, size_t start)
{
    $assert(start < sv->len);
    sv->items += start;
    sv->len -= start;
}

/*
 * @brief Allocates and copies an SV to a new Str with given capacity
 * @category Strings
 */
Str svToStr(SV sv, size_t extraCap)
{
    size_t cap = sv.len + extraCap;
    char* ptr = malloc(cap);
    $assert(ptr);
    memcpy(ptr, sv.items, sv.len);
    return (Str) { .items = ptr, .len = sv.len, .cap = cap };
}

/*
 * @brief Creates an SV view of a Str
 * @category Strings
 */
SV strToSV(Str str)
{
    return (SV) { .items = str.items, .len = str.len };
}

bool svEq(SV sv1, SV sv2)
{
    if (sv1.len != sv2.len)
        return false;
    return memcmp(sv1.items, sv2.items, sv1.len) == 0;
}

ArrayListSV splitSVByChar(SV sv, char splitBy)
{
    ArrayListSV svList = $initArrayList(SV);
    uint16_t prevSplit = 0;
    for (uint16_t i = 0; i < sv.len; ++i) {
        if (sv.items[i] == splitBy) {
            SV slicedSv = svSlice(sv, prevSplit, i - prevSplit);
            $append((&svList), slicedSv);
            prevSplit = i + 1;
        }
    }
    // Add the last segment after the final delimiter
    if (prevSplit < sv.len) {
        SV slicedSv = svSlice(sv, prevSplit, sv.len - prevSplit);
        $append((&svList), slicedSv);
    }
    return svList;
}

ArrayListSV splitStrByChar(Str str, char splitBy) { return splitSVByChar(strToSV(str), splitBy); }

#define splitByChar(toSplit, splitBy_) _Generic((toSplit), \
    SV: splitSVByChar,                                     \
    Str: splitStrByChar,                                   \
    default: static_assert(false, "Invalid type passed to `splitByChar`"))(toSplit, splitBy_)

// TODO: Add toTrimmed functions
#if 0
SV SVtoTrimmedLeft(SV sv)
{
    size_t i = 0;
    while (i < sv.len and isWhitespace(sv.items[i])) {
        i += 1;
    }
    if (i > 0)
        sv.items += i;
    return sv;
}
Str StrtoTrimmedLeft(Str str)
{
    size_t i = 0;
    while (i < str.len and isWhitespace(str.items[i])) {
        i += 1;
    }
    if (i > 0)
        str.items += i;
    return str;
}
#endif

void svTrimLeft(SV* sv)
{
    size_t i = 0;
    while (i < sv->len and isWhitespace(sv->items[i])) {
        i += 1;
    }
    if (i > 0) {
        sv->items += i;
        sv->len -= i;
    }
}
void svTrimRight(SV* sv)
{
    size_t i = 0;
    while ((sv->len - i) > 0 and isWhitespace(sv->items[(sv->len - i)])) {
        i += 1;
    }
    if (i > 0)
        sv->len -= i;
}

void svTrim(SV* sv)
{
    svTrimLeft(sv);
    svTrimRight(sv);
}

/*
 * @brief Result type for file reading
 * @category IO
 */
typedef struct {
    enum {
        ok,
        err,
    } result;
    union {
        Str content;
    };
} FileReadResult;

/*
 * @brief Reads entire file into a Str
 * @category IO
 * @param filename : SV - The filename to read
 * @returns FileReadResult - Contains the file content or error state
 */
FileReadResult readFileToStr(SV filename)
{
    ensureSVNullTermination(filename);

    FILE* f = fopen(filename.items, "rb");
    if (!f) {
        fprintf(stderr, "Reading file %s failed!\n", filename.items);
        return (FileReadResult) { err, .content = $str("ERROR: Could not open file!") };
    }

    fseek(f, 0, SEEK_END);
    long len = ({
        auto ftell$result = ftell(f);
        $assert(ftell$result >= 0);
        ftell$result;
    });
    $assert(len != -1);
    fseek(f, 0, SEEK_SET);

    char* buf = malloc(len + 1);
    if (!buf) {
        $assert(false);
        fclose(f);
        return (FileReadResult) { err, .content = $str("ERROR: MALLOC") };
    }

    size_t readLen = fread(buf, 1, len, f);
    fclose(f);

    buf[readLen] = '\0'; // Still null-terminate for convenience
    return (FileReadResult) {
        .result = ok,
        .content = {
            .items = buf,
            .len = readLen,
            .cap = len + 1,
        }
    };
}

void _$$cleanupFcloseFn(FILE** ptrToFptr)
{
    $assert(ptrToFptr);
    FILE* fPtr = *ptrToFptr;
    $assert(fPtr);
    fclose(*ptrToFptr);
}

#define $deferClose $cleanup(_$$cleanupFcloseFn)

bool writeSVToFile(SV filename, SV content)
{
    ensureNullTermination(filename);
    $deferClose FILE* file = fopen(filename.items, "wb");
    // TODO: Actual error reporting
    if (!file)
        return false;
    fwrite(content.items, 1, content.len, file);
    int fileError = ferror(file);
    // TODO: Actual error reporting
    if (fileError < 0)
        return false;

    return true;
}

bool writeStrToFile(SV filename, Str content) { return writeSVToFile(filename, strToSV(content)); }

#define $ArrayList(T) ArrayList##T
/*
 * @brief Initializes a new ArrayList with default capacity
 * @category Collections
 * @macro
 * @generic T : $defArrayList(T) must be called first
 * @returns ArrayList[T] - A newly initialized array list
 * @example ArrayListint myList = init$ArrayList(int);
 * @see $ArrayList
 * @see $defArrayList
 */
#define init$ArrayList(T)                \
    ($ArrayList(T))                      \
    {                                    \
        .items = malloc(10 * sizeof(T)), \
        .cap = 10,                       \
        .len = 0,                        \
    }

typedef struct {
    bool ok;
    union {
        long value;
        enum {
            ParseErr_unknown,
        } parseErr;
    };
} ParseResult;

/*
 * @brief Parses an integer from an SV
 * @category Strings
 */
ParseResult parseIntFromSV(SV sv)
{
    ensureSVNullTermination(sv);

    size_t res = atol(sv.items);
    if (res == 0 && !svEqLit(sv, "0"))
        return (ParseResult) { false, { ParseErr_unknown } };

    return (ParseResult) { true, { res } };
}
ParseResult parseIntFromStr(Str str)
{
    return parseIntFromSV(strToSV(str));
}

#define parseInt(s) _Generic((s), \
    Str: parseIntFromStr,         \
    SV: parseIntFromSV,           \
    default: static_assert("Incompatible type passed to `parseInt, it can only be used with `Str` and `SV`"))(s)

#if 0
typedef enum {
    LogDebug,
    LogInfo,
    LogWarning,
    LogError,
} LogLevel;

void $log(LogLevel ll, SV msg)
{
    switch (ll) {
    case LogDebug: {
        printf("[DEBUG]: ");
    } break;
    case LogInfo: {
        printf("[Info]: ");
    } break;
    case LogWarning: {
        printf("[Warning]: ");
    } break;
    case LogError: {
        printf("[Error]: ");
    } break;
    }
    write(stdout, msg.items, msg.len);
}
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
