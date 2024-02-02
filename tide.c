#include "belper.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ttycom.h>

typedef unsigned char byte;
typedef byte bool;

#define false 0
#define true  1

typedef unsigned short us;
#define insert_char 126

#define clear()    printf("\e[1;1H\e[2J")
#define goto(x, y) printf("\033[%d;%dH", y, x);

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

int main(int argc, string *argv) {
    bool   hasfile;
    string filename = "empty.txt";

    if (argc <= 1) {
        // empty file
        hasfile = false;
    } else {
        // literal file
        hasfile  = true;
        filename = argv[ 1 ];
    }

    File               file_contents = { .size = 1, .content = malloc(sizeof(char)) };
    unsigned long long file_location = 0;

    if (hasfile) {
        File *f = readFile(filename);
        if (f->size == 0 && f->content == NULL) hasfile = false;
    }

    bool is_insert  = false;
    bool has_output = false;

#define STANDARD_SIZE 2048

    char cmd_buffer[ STANDARD_SIZE ];
    char cmd_output[ STANDARD_SIZE ];

    for (us i = 0; i < STANDARD_SIZE; i++) cmd_buffer[ i ] = 0;
    for (us i = 0; i < STANDARD_SIZE; i++) cmd_output[ i ] = 0;

    int ptr = -1;

    char input;

    string acceptable = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,/"
                        "<>?;':\"[]{}|\\+=_-)(*&^%$#@!~`";

    while (true) {
        struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);

        us width  = w.ws_col;
        us height = w.ws_row;

        // START DRAW

        // clear screen
        clear();

        // TODO: write text
        for (us i = 0; i < height - 1; i++) { printf("\n"); }

        // write insert buffer
        bg(187, 110, 255);
        for (us i = 0; i < width; i++) printf(" ");
        for (us i = 0; i < width; i++) printf("\r");
        printf("\n [%s] %s", is_insert ? "🖊️ " : "💻", cmd_buffer);
        reset();

        fflush(stdout);

        goto(0, 0);

        system("/bin/stty raw");
        input = getchar();
        system("/bin/stty cooked");

        if (input == 27 || input == 3
            || input == 4) { // Shift+Tab | Esc | Ctrl+C | Ctrl+D -> change mode
            is_insert = !is_insert;
            continue;
        }

        if (!is_insert) {
            if (input == 127) { // Backspace
                cmd_buffer[ ptr-- ] = 0;
                if (ptr < -1) ptr = -1;
            } else if (input == 13) {
                if (!strncmp(cmd_buffer, "exit", 5)) goto EXIT;
                else {}

                ptr = -1;
                for (us i = 0; i < STANDARD_SIZE; i++) cmd_buffer[ i ] = 0;
                goto CONTINUE;
            }

            else if (indexOf(acceptable, input) != -1) {
                cmd_buffer[ ++ptr ] = input;
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

    CONTINUE:;
    }
}