#include "belper.h"
#include "file.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ttycom.h>
#include <unistd.h>

typedef unsigned char byte;
typedef byte bool;

#define false 0
#define true  1

typedef unsigned short us;
#define insert_char 126

#define clear()    printf("\e[1;1H\e[2J")
#define goto(x, y) printf("\033[%llu;%lluH", y, x);

int indexOf(string haystack, char needle) {
    for (int i = 0; i < strlen(haystack); i++)
        if (haystack[ i ] == needle) return i;
    return -1;
}

int rgbToAnsi(int r, int g, int b) {
    if (r == g && g == b) {
        if (r < 8) return 16;
        if (r < 248) return 231;

        return (int) (((r - 8) / 247) * 24) + 232;
    }

    return 16 + (36 * (int) (r / 255 * 5)) + (6 * (int) (g / 255 * 5)) + (int) (b / 255 * 5);
}

#define fg(r, g, b) printf("\033[38;2;%i;%i;%im", r, g, b)
#define bg(r, g, b) printf("\033[48;2;%i;%i;%im", r, g, b)
#define reset()     printf("\e[0m")
#define bold        "\e[1m"
#define unbold      "\e[22m"

typedef unsigned long long ull;

typedef struct location {
    ull index;
    ull line;
    ull column;
} location;

typedef struct {
    ull         index;
    sized_char *content;
    location   *location;
} str_file;

#define alc(type, name)                                                                            \
    type *name = malloc(sizeof(type));                                                             \
    sanity(name)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

location *getlocation(str_file *file) {
    alc(location, l);

    l->index  = file->index;
    l->line   = -1;
    l->column = -1;

    sized_string *lines        = split(file->content->array, '\n');
    ull           currentIndex = 0;

    for (ull i = 0; i < lines->size; i++) {
        string line = lines->array[ i ];
        ull    len  = strlen(line);
        if (currentIndex + len + 1 > file->index) {
            l->line   = i;
            l->column = MAX(file->index - currentIndex, 0);
            break;
        }
        currentIndex += len + 1; // +1 for newline character
    }

    l->line   = MAX((long long) l->line, 0);
    l->column = MAX((long long) l->column, 0);

    for (ull i = 0; i < lines->size; i++) free(lines->array[ i ]);
    free(lines); // Free the memory allocated for lines

    return l;
}

string substring(const string str, ull start, ull length) {
    if (length == 0) { return NULL; }

    sized_char *substr = bh_init_char();
    for (ull i = start; i < start + length && i < strlen(str); i++) {
        bh_append_char(substr, str[ i ]);
    }

    // printf("got: %s\n", substr->array);
    string a = substr->array;
    free(substr);
    return a;
}

sized_string *getvisible(ull height, ull width, str_file contents) {
    // logic
    // this function should get the height and width, and the contents of a file
    // get lines that fit on (height - 2) above and below the cursor (cursor should always be in the
    // middle) and trim them to the width of the screen left and write of the cursor (here it should
    // also be in the middle) however, lines shouldn't be added to the top or whitespace characters
    // to the left, meaning there shoulnd't be additional whitespace to the left and the top of the
    // lines.

    sized_string *lines  = bh_init_string();
    location     *cursor = getlocation(&contents);

    sized_string *fileLines = split(contents.content->array, '\n');

    // Calculate the range of lines to display
    ull startLine = cursor->line < (height - 4) ? 0 : cursor->line - (height - 4);
    ull endLine   = MIN(fileLines->size, startLine + height - 4);

    // Iterate over the lines and add them to the visible lines
    for (ull i = startLine; i < endLine; i++) {
        string line = fileLines->array[ i ];
        // Trim the line to fit within the width
        ull trimStart = cursor->column < width ? 0 : cursor->column - width;
        ull trimEnd   = MIN(strlen(line), trimStart + width);

        string trimmedLine
            = trimEnd - trimStart <= width ? line : substring(line, trimStart, trimEnd - trimStart);
        bh_append_string(lines, trimmedLine);
    }

    for (ull i = endLine; i < height - 4 - (endLine - startLine); i++) bh_append_string(lines, "");

    // Clean up
    free(fileLines);
    free(cursor);

    return lines;
}

#define STANDARD_SIZE 2048

sized_char *insert(sized_char *input, ull location, char x) {
    sized_char *new = bh_init_char();
    for (ull i = 0; i < input->size + 1; i++) {
        if (i == location) bh_append_char(new, x);
        else if (i > location)
            bh_append_char(new, input->array[ i - 1 ]);
        else
            bh_append_char(new, input->array[ i ]);
    }

    return new;
}

sized_char *delete(sized_char *input, ull location) {
    sized_char *new = bh_init_char();
    for (ull i = 0; i < input->size + 1; i++) {
        if (i == location) continue;
        else if (i > location)
            bh_append_char(new, input->array[ i ]);
        else
            bh_append_char(new, input->array[ i ]);
    }

    return new;
}

#define updatelocation() file_contents.location = getlocation(&file_contents);

int main(int argc, string *argv) {
    bool   hasfile;
    string filename;

    str_file file_contents = { .content = bh_init_char() };

    if (argc <= 1) {
        // empty buffer
        hasfile  = false;
        filename = "empty.txt";
    } else {
        // literal file
        hasfile  = true;
        filename = argv[ 1 ];

        File *f = readFile(filename);
        if (f->size == 0 && f->content == NULL) hasfile = false;

        // add characters
        for (ull i = 0; i < f->size; i++) bh_append_char(file_contents.content, f->content[ i ]);
    }

    (void) filename;

    updatelocation();

    bool is_insert  = true;
    bool has_output = false;

    char cmd_buffer[ STANDARD_SIZE ];

    for (us i = 0; i < STANDARD_SIZE; i++) cmd_buffer[ i ] = 0;

    int ptr = -1;

    char input;

    string acceptable = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,/"
                        "<>?;':\"[]{}|\\+=_-)(*&^%$#@!~`";

    bool   hasmessage = false;
    string message    = "";

    bool has_last = false;
    char last     = 0;

    bool has_insert = true;

    while (true) {
        struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);

        us width  = w.ws_col;
        us height = w.ws_row + 1;

        // START DRAW

        sized_string *lines = NULL;
        if (has_insert) { // insertion done. redraw text.c
            clear();
            if(lines != NULL) {
                for(ull i = 0; i < lines->size; i++) {
                    free(lines->array[i]);
                }
                free(lines);
            }

            lines = getvisible(height, width, file_contents);
            for (us i = 0; i < lines->size; i++) {
                string cur = lines->array[ i ];
                for (us j = 0; j < strlen(cur); j++) {
                    if (cur[ j ] == '\n') printf(" \n");
                    else
                        printf("%c", cur[ j ]);
                }
                printf("\n");
            }
            fflush(stdout);
            has_insert = false;
        }

        goto((ull) 0, (ull) (height - 2));

        // write insert buffer
        bg(187, 110, 255);
        fg(0, 0, 0);
        for (us i = 0; i < width; i++) printf(" ");
        for (us i = 0; i < width; i++) printf("\r");
        fflush(stdout);
        printf(
            " [STATUS] Ln %llu Col %llu %s %s\n",
            file_contents.location->line + 1,
            file_contents.location->column + 1,
            file_contents.content->size == 0 ? (hasfile ? "(Empty file)" : "(New file)") : "",
            hasmessage ? message : "");
        for (us i = 0; i < width; i++) printf(" ");
        for (us i = 0; i < width; i++) printf("\r");
        printf(" [%s] %s", is_insert ? "🖊️ " : "💻", cmd_buffer);
        reset();

        fflush(stdout);

        goto(file_contents.location->column + 1, MIN(file_contents.location->line + 1, height - 2));

        system("/bin/stty raw");
        input = has_last ? last : getchar();
        if (has_last) last = false;
        system("/bin/stty cooked");

        if (input == '\e' && is_insert) { // if the first value is esc
            char c = getchar();
            if (c != '[') { // skip the [
                has_last = true;
                last     = c;
                continue;
            }

            ull col;
            ull _li;

            ull nextline;
            ull curline;

            c = getchar();
            switch (c) { // the real arrow value
                case 'A':
                    // code for arrow up
                    
                    col = file_contents.location->column;
                    _li = file_contents.location->line;

                    nextline = strlen(lines->array[_li - 1]);
                    
                    if(col > nextline) { // line size is more than future line, should go to the end of that one
                        while(file_contents.location->column != nextline || file_contents.location->line != (_li - 1)) {
                            file_contents.index--;
                            updatelocation();
                        }
                    } else {
                        while(file_contents.location->column != col || file_contents.location->line != (_li - 1)) {
                            file_contents.index--;
                            updatelocation();
                        }
                    }

                    break;
                case 'B':;
                    // code for arrow down

                    col = file_contents.location->column;
                    _li = file_contents.location->line;

                    nextline = strlen(lines->array[_li + 1]);

                    if(col > nextline) { // line size is more than future line, should go to the end of that one
                        while(file_contents.location->column != nextline || file_contents.location->line != (_li + 1)) {
                            file_contents.index++;
                            updatelocation();
                        }
                    } else {
                        while(file_contents.location->column != col || file_contents.location->line != (_li + 1)) {
                            file_contents.index++;
                            updatelocation();
                        }
                    }
                    break;
                case 'C':;
                    // code for arrow right
                    file_contents.index++;
                    // if(file_contents.content->array[file_contents.index] == '\n')
                    // file_contents.index++;
                    break;
                case 'D':;
                    // code for arrow left
                    file_contents.index--;
                    // if(file_contents.content->array[file_contents.index] == '\n')
                    // file_contents.index--;
                    break;
                default:
                    has_last = true;
                    last     = c;
                    continue;
            }

            file_contents.index
                = MIN(MAX((long long) file_contents.index, 0), file_contents.content->size - 1);
            updatelocation();

            continue;
        }

        if (input <= 10) { // Any control character -> change mode
            is_insert = !is_insert;
            continue;
        }

        if (!is_insert) {
            if (input == 127) { // Backspace
                cmd_buffer[ ptr-- ] = 0;
                if (ptr < -1) ptr = -1;
            } else if (input == 13) {
                if (ptr == 0) {
                } else if (!strcmp(cmd_buffer, "exit") || !strcmp(cmd_buffer, "quit"))
                    goto EXIT;
                else if (!strcmp(cmd_buffer, "d") || !strncmp(cmd_buffer, "dismiss", 7)) {
                    hasmessage = false;
                }

                else {
                    hasmessage = true;
                    message    = "Unknown command";
                }

                ptr = -1;
                for (us i = 0; i < STANDARD_SIZE; i++) cmd_buffer[ i ] = 0;
                goto CONTINUE;
            }

            else if (indexOf(acceptable, input) != -1) {
                if (isprint(input)) cmd_buffer[ ++ptr ] = input;
            }
        } else {
            if (isprint(input) || input == '\n' || input == 13) {
                file_contents.content = insert(
                    file_contents.content, file_contents.index, input == 13 ? '\n' : input);
                file_contents.index++;
                updatelocation();

                has_insert = true;
            } else if (input == 127 || input == 8) {
                file_contents.content = delete (file_contents.content, file_contents.index - 1);
                file_contents.index--;
                updatelocation();

                has_insert = true;
            }
        }

        if (ptr >= STANDARD_SIZE || ptr >= width - 7) {
            ptr = -1;
            for (us i = 0; i < STANDARD_SIZE; i++) cmd_buffer[ i ] = 0;
        }

        goto CONTINUE;

    EXIT:
        printf("exit");
        clear();
        exit(0);

    CONTINUE:
        updatelocation();
    }
}