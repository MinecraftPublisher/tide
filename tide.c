#include "file.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define var __auto_type

#define clear()                                                                                    \
    printf("\e[1;1H\e[2J");                                                                        \
    fflush(stdout)
#define goto(x, y) printf("\033[%u;%uH", y, x)
#define at(x, y, c)                                                                                \
    printf("\033[%u;%uH%s", y, x, c);                                                              \
    fflush(stdout)

#define fg(r, g, b) printf("\033[38;2;%i;%i;%im", r, g, b)
#define bg(r, g, b) printf("\033[48;2;%i;%i;%im", r, g, b)

#define reset() printf("\e[0m")
#define bold    "\e[1m"
#define unbold  "\e[22m"

void memoryFailure(string file, int line) {
    printf(
        "----------------------------\n"
        " Memory allocation failure!\n"
        " At file %s\n"
        " Line %i\n"
        "----------------------------\n",
        file,
        line);
    exit(SIGABRT);
}

#define format(fmts, ...)                                                                          \
    ({                                                                                             \
        string buf    = NULL;                                                                      \
        int    result = asprintf(&buf, fmts __VA_OPT__(, ) __VA_ARGS__);                           \
        if (result < 0) memoryFailure(__FILE__, __LINE__);                                         \
        buf;                                                                                       \
    })

typedef unsigned short us;
struct curpos {
    us x;
    us y;
};

struct terminal_size {
    us width;
    us height;
};
struct terminal_size get_size() {
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    us width  = w.ws_col;
    us height = w.ws_row + 1;

    return (struct terminal_size) { .width = width, .height = height };
}

struct terminal_size size;

typedef struct rgb {
    us r;
    us g;
    us b;
} rgb;
typedef struct spot {
    rgb  bgcolor;
    rgb  fgcolor;
    char value;
} spot;

#define MIN(a, b) ((a) < (b)) ? (a) : (b)

void write_buf(spot* buf, string text, us x, us y, rgb fgcolor, rgb bgcolor) {
    var len      = strlen(text);
    var max      = MIN(len, size.width - x);
    var setcolor = fgcolor.r == fgcolor.g == fgcolor.b == bgcolor.r == bgcolor.g == bgcolor.b;
    for (us i = 0; i < max; i++) {
        buf[ y * size.width + x + i ].value = text[ i ];
        if (setcolor) continue;
        buf[ y * size.width + x + i ].fgcolor = fgcolor;
        buf[ y * size.width + x + i ].bgcolor = bgcolor;
    }

    free(text);
}

void write_from_right(spot* buf, string text, us x, us y, rgb fgcolor, rgb bgcolor) {
    // Calculate start position
    var length = strlen(text);
    var max    = MIN(length, size.width);

    var start = size.width - x - max;

    if (start < 0) start = 0;
    write_buf(buf, text, start, y, fgcolor, bgcolor);
}

#define p_cat(a, b) a##b
#define cat(a, b)   p_cat(a, b)

#define expand(...) /* The use case of this macro is left as an exercise to the reader. */         \
    __VA_ARGS__
#define __demon(code, ID)                                                                          \
    ({                                                                                             \
        __pid_t cat(fork_, ID) = fork();                                                           \
        if (cat(fork_, ID) < 0) {                                                                  \
            perror("Failed to fork!");                                                             \
            exit(1);                                                                               \
        } else if (cat(fork_, ID) == 0) {                                                          \
            code;                                                                                  \
            exit(0);                                                                               \
        };                                                                                         \
        cat(fork_, ID);                                                                            \
    })
#define demon(code)    /* Fork the current process and creates an async process that runs          \
                          in parallel, it also returns the PID of the child process to the         \
                          parent, in case you need to wait until the child dies. */                \
    __demon(code, expand(__COUNTER__))

const string modes[] = { "Edit", "Command" };

void exitme() {
    clear();
    printf("Tide, out.");
    exit(0);
}

char is_grid_equal(spot* grid1, spot* grid2) {
    var max_size = size.width * size.height;

    for (int i = 0; i < max_size; i++) {
        var left  = grid1[ i ];
        var right = grid2[ i ];

        if (left.value == right.value
            && (left.bgcolor.r == right.bgcolor.r && left.bgcolor.g == right.bgcolor.g
                && left.bgcolor.b == right.bgcolor.b)
            && (left.fgcolor.r == right.fgcolor.r && left.fgcolor.g == right.fgcolor.g
                && left.fgcolor.b == right.fgcolor.b))
            continue;
        return 0;
    }

    return 1;
}

#define write(x, y, fr, fg, fb, br, bg, bb, text, ...)                                             \
    write_buf(buffer, format(text, __VA_ARGS__), x, y, (rgb) { fr, fg, fb }, (rgb) { br, bg, bb })
#define write_right(x, y, fr, fg, fb, br, bg, bb, text, ...)                                       \
    write_from_right(                                                                              \
        buffer,                                                                                    \
        format(text __VA_OPT__(, ) __VA_ARGS__),                                                   \
        x,                                                                                         \
        y,                                                                                         \
        (rgb) { fr, fg, fb },                                                                      \
        (rgb) { br, bg, bb })
#define winherit(x, y, text, ...)  write(x, y + 1, 0, 0, 0, 0, 0, 0, text, __VA_ARGS__)
#define wrinherit(x, y, text, ...) write_right(x, y, 0, 0, 0, 0, 0, 0, text, __VA_ARGS__)

#define status_1(text, ...) winherit(1, size.height - 3, text, __VA_ARGS__)
#define status_2(text, ...) winherit(1, size.height - 2, text, __VA_ARGS__)

void output_buffer(spot* buffer) {
    for (int y = 0; y < size.height; y++) {
        goto(0, y);
        for (int x = 0; x < size.width; x++) {
            var spot     = buffer[ y * size.width + x ];
            var setcolor = spot.fgcolor.r == spot.fgcolor.g == spot.fgcolor.b == spot.bgcolor.r
                           == spot.bgcolor.g == spot.bgcolor.b;
            if (!setcolor) {
                fg(spot.fgcolor.r, spot.fgcolor.g, spot.fgcolor.b);
                bg(spot.bgcolor.r, spot.bgcolor.g, spot.bgcolor.b);
            } else {
                reset();
            }

            printf("%c", spot.value == 0 ? ' ' : spot.value);
        }
    }

    reset();
}

void __fill_row(spot* buffer, int row, rgb fg, rgb bg) {
    for (int j = 0; j < size.width; j++) {
        var addr        = &buffer[ size.width * row + j ];
        addr->bgcolor.r = bg.r;
        addr->bgcolor.g = bg.g;
        addr->bgcolor.b = bg.b;
        addr->fgcolor.r = fg.r;
        addr->fgcolor.g = fg.g;
        addr->fgcolor.b = fg.b;
    }
}

#define fill_row(row, br, bg, bb, fr, fg, fb)                                                      \
    __fill_row(buffer, row, (rgb) { fr, fg, fb }, (rgb) { br, bg, bb })

spot* create_buffer(struct terminal_size size) {
    var mem = mmap(
        NULL,
        size.width * size.height * sizeof(spot),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0);
    memset(mem, 0, size.width * size.height * sizeof(spot));
    return mem;
}

void init(spot* buffer) {
    for (int i = 0; i < size.width * size.height; i++) {
        buffer[ i ].fgcolor = (rgb) { 255, 255, 255 };
    }

    for (int i = 1; i <= 2; i++) { fill_row(size.height - i, 255, 100, 200, 0, 0, 0); }
}

int main() {
    signal(SIGKILL, exitme);
    signal(SIGQUIT, exitme);
    signal(SIGINT, exitme);

    var mode = 0;

    size              = get_size();
    spot* buffer      = create_buffer(size);
    spot* last_buffer = create_buffer(size);

    init(buffer);

    // Console reader
    demon({
        system("/bin/stty raw -echo");
        while (1) {
            char c = getchar();
            winherit(0, 0, "%c", c);
            if (c < 10) {
                kill(getppid(), SIGINT);
                exit(SIGINT);
            }
        }
    });

    while (1) {
        var new_size = get_size();
        var just_changed = new_size.height != size.height || new_size.width != size.width;
        if (just_changed) {
            clear();
            munmap(buffer, size.width * size.height * sizeof(spot));
            munmap(last_buffer, size.width * size.height * sizeof(spot));
            size        = new_size;
            buffer      = create_buffer(size);
            last_buffer = create_buffer(size);
            init(buffer);
        }

        status_2("%s", "How are you?");

        status_1("Mode: %s", modes[ mode ]);
        wrinherit(1, size.height - 2, "[%li]", time(NULL));

        if (!is_grid_equal(buffer, last_buffer) || just_changed) {
            // Copy buffer into last_buffer
            memcpy(last_buffer, buffer, size.width * size.height * sizeof(spot));

            // clear();
            output_buffer(buffer);
            goto(0, 0);
            fflush(stdout);
        }
    }
}