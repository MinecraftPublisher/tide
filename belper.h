#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define auto __auto_type
typedef char *string;

#define fix(i)                                                                                     \
    ({                                                                                             \
        int *__I__ = i;                                                                            \
        *__I__;                                                                                    \
    })

void memoryfailure(char *__FILE, int __LINE) {
    printf("Memory allocation failure %s %i\n", __FILE, __LINE);
    exit(EXIT_FAILURE);
}

#define sanity(x)                                                                                  \
    if (x == NULL) memoryfailure(__FILE__, __LINE__)

#define PCAT(x, y)    x##y
#define CAT(x, y)     PCAT(x, y)
#define CAT3(x, y, z) CAT(CAT(x, y), z)

typedef struct bit_asm_context_tag bit_asm_context_t;

// -------------------------- DOOM CODE --------------------------
// ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
// TOUCHING THIS CODE WILL RESULT IN BRAIN CANCER & OTHER DISEASES
// ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
// -------------------------- DOOM CODE --------------------------
#define bh_create_sized(arr_type)                                                                  \
    typedef struct CAT(sized_, arr_type) {                                                         \
        unsigned long long size;                                                                   \
        arr_type          *array;                                                                  \
    } CAT(sized_, arr_type);                                                                       \
                                                                                                   \
    CAT(sized_, arr_type) * CAT(bh_init_, arr_type)() {                                            \
        CAT(sized_, arr_type) *__arr__ = malloc(sizeof(CAT(sized_, arr_type)));                    \
        sanity(__arr__);                                                                           \
        __arr__->size  = 0;                                                                        \
        __arr__->array = malloc(sizeof(arr_type));                                                 \
        sanity(__arr__->array);                                                                    \
        return __arr__;                                                                            \
    }                                                                                              \
                                                                                                   \
    CAT(sized_, arr_type)                                                                          \
    *CAT(bh_append_, arr_type)(CAT(sized_, arr_type) * arr, arr_type element) {                    \
        arr->size++;                                                                               \
        arr->array = realloc(arr->array, sizeof(arr_type) * arr->size);                            \
        sanity(arr->array);                                                                        \
        arr->array[ arr->size - 1 ] = element;                                                     \
        return arr;                                                                                \
    }                                                                                              \
                                                                                                   \
    CAT(sized_, arr_type) * CAT(bh_remove_, arr_type)(CAT(sized_, arr_type) * arr) {               \
        arr->size--;                                                                               \
        arr->array[ arr->size ] = 0;                                                               \
        arr->array              = realloc(arr->array, sizeof(arr_type) * arr->size);               \
        sanity(arr->array);                                                                        \
        return arr;                                                                                \
    }
// ---------------------- END OF DOOM CODE -----------------------
// ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
// TOUCHING THIS CODE WILL RESULT IN BRAIN CANCER & OTHER DISEASES
// ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
// ---------------------- END OF DOOM CODE -----------------------

string replaceAll(string str, const string old, const string with) {
    if (!strcmp(old, "")) return str;

    size_t old_len = strlen(old);
    size_t new_len = strlen(with);
    size_t diff    = old_len >= new_len ? 0 : new_len - old_len;

    size_t len         = strlen(str);
    size_t num_matches = 0;

    // Count the number of matches
    for (size_t i = 0; i <= len - old_len; i++) {
        if (!strncmp(&str[ i ], old, old_len)) { num_matches++; }
    }

    // Allocate memory for the new string
    string new = malloc(sizeof(char) * (len + num_matches * diff + 1));
    sanity(new);

    // Copy characters from the old string to the new string
    size_t j = 0;
    size_t i = 0;
    for (; i <= len - old_len; i++) {
        if (!strncmp(&str[ i ], old, old_len)) {
            for (size_t k = 0; k < new_len; k++) { new[ j++ ] = with[ k ]; }
            i += old_len - 1;
        } else {
            new[ j++ ] = str[ i ];
        }
    }

    // Copy the remaining characters
    while (i < len) { new[ j++ ] = str[ i++ ]; }

    new[ j ] = '\0';

    return new;
}

#define START_RULE()

bh_create_sized(string);
bh_create_sized(char);
typedef sized_char str;

string trim(string input) {
    if (input == NULL) return NULL;

    size_t length = strlen(input);
    // # count spaces at start
    size_t spaces_start = -1;
    while (input[ ++spaces_start ] == ' ' || input[ spaces_start ] == '\n'
           || input[ spaces_start ] == '\t')
        ;

    // # count spaces at the end
    size_t spaces_end = -1;
    while (input[ length - ++spaces_end - 1 ] == ' ' || input[ length - spaces_end - 1 ] == '\n'
           || input[ length - spaces_end - 1 ] == '\t')
        ;

    if (spaces_start == 0 && spaces_end == 0) return input;
    string new_trim = malloc(sizeof(char) * (length - spaces_start - spaces_end));
    sanity(new_trim);

    size_t j = 0;
    for (size_t i = spaces_start; i < length - spaces_end; i++) { new_trim[ j++ ] = input[ i ]; }

    return new_trim;
}

sized_string *split(string text, char delimeter) {
    sized_string *output = bh_init_string();

    size_t last_ti = 0;
    size_t ti      = 0;
    string _start  = text;
    for (; ti < strlen(text); ti++)
        if (text[ ti ] == delimeter) {
            // # copy the string

            if (ti - last_ti == 0) {
                bh_append_string(output, "");
                continue;
            }

            string new = malloc(sizeof(char) * (ti - last_ti));
            sanity(new);
            for (size_t ni = 0; ni < (ti - last_ti); ni++) { new[ ni ] = _start[ ni ]; }

            bh_append_string(output, trim(new));
            _start  = &text[ ti + 1 ];
            last_ti = ti + 1;
        }

    string new = malloc(sizeof(char) * (ti - last_ti));
    sanity(new);
    for (size_t ni = 0; ni < (ti - last_ti); ni++) { new[ ni ] = _start[ ni ]; }

    bh_append_string(output, new);
    _start  = &text[ ti + 1 ];
    last_ti = ti + 1;
    return output;
}