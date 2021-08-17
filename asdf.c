#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>

#define PAIR_NORMAL 0
#define PAIR_SPECIAL 1
#define PAIR_HIGHLIGHT 2

#define NEWLINE "$\n"
#define TABCHAR "----"

#define COLOR_ON(pair) attron(COLOR_PAIR(pair))
#define COLOR_OFF(pair) attroff(COLOR_PAIR(pair))
#define CTRL(k) ((k) & 0x1f)
#define MIN(a, b) ((a < b) ? a : b)

#define SPECIAL_PRINTW(pair, ...)               \
    do {                                        \
        attron(COLOR_PAIR(PAIR_SPECIAL));       \
        printw(__VA_ARGS__);                    \
        attroff(COLOR_PAIR(PAIR_SPECIAL));      \
        attron(COLOR_PAIR(pair));               \
    } while (0)

typedef struct {
    char *text;
    size_t capacity;
} Buffer;

typedef struct {
    size_t total;
    size_t wrong;
} Result;

// Free the text in a buffer and reset it
void buffer_free(Buffer *buffer)
{
    if (buffer->text) free(buffer->text);
    buffer->capacity = 0;
}

// Read a file into a buffer
void buffer_read(Buffer *buffer, const char *file_name)
{
    FILE *file = fopen(file_name, "r");
    if (!file) {
        fprintf(stderr, "error: could not read file '%s'\n", file_name);
        buffer_free(buffer);
        endwin();
        exit(1);
    }

    // Get the size of the file
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    size_t length = file_size + 1;

    // Buffer allocations if necessary
    if (length > buffer->capacity) {
        buffer->text = (buffer->text)
            ? realloc(buffer->text, length)
            : malloc(length);
        buffer->capacity = length;
    }

    // Read the file into the buffer
    fread(buffer->text, sizeof(char), file_size, file);
    fclose(file);

    buffer->text[file_size] = '\0';
}

// Print a chunk of text with special highlights and characters for
// newlines and tabs
size_t colored_print(const char *text, size_t limit)
{
    clear();
    COLOR_OFF(PAIR_HIGHLIGHT);

    size_t count = 0;
    size_t index = 0;
    for ( ; count < limit; ++index) {
        switch (text[index]) {
        case '\n':
            count += 1 + COLS - count % COLS;
            SPECIAL_PRINTW(PAIR_NORMAL, NEWLINE);
            break;
        case '\t':
            count += strlen(TABCHAR);
            SPECIAL_PRINTW(PAIR_NORMAL, TABCHAR);
            break;
        case '\0':
            count = limit;
            break;
        default:
            count++;
            addch(text[index]);
        }
    }
    COLOR_ON(PAIR_HIGHLIGHT);
    return index;
}

// Train the user on a buffer
bool buffer_train(const Buffer *buffer, Result *result)
{
    const char *page = buffer->text;
    size_t page_size = 0;

    size_t wrong = 0;
    size_t total = 0;

    char input;
    size_t index = 0;
    size_t row = 0;
    size_t col = 0;

    bool running = true;
    bool stopped = false;
    while (running) {
        // Display the current "page" of the buffer to be trained. A
        // page is a portion of the buffer which can be displayed at a
        // time because of terminal size limitations.
        page_size = colored_print(page, LINES * COLS);

        // Go to the origin of the terminal screen
        row = 0;
        col = 0;
        move(row, col);

        // Train the user on the current page
        while (running) {
            input = getch();

            // Basically keep count of the total and wrongly typed
            // characters. Also keep highlighting the characters
            // which have been typed for a better visual appeal.
            if (input == page[index]) {
                index++;
                total++;

                if (index >= page_size) {
                    break;
                } else if (page[index] == '\0') {
                    running = false;
                } else {
                    // Move the cursor down one line if needed
                    if (col + 1 == (size_t) COLS) {
                        col = 0;
                        row++;
                    } else if (page[index - 1] == '\n') {
                        col = 0;
                        row++;
                        SPECIAL_PRINTW(PAIR_HIGHLIGHT, NEWLINE);
                    } else if (page[index - 1] == '\t') {
                        SPECIAL_PRINTW(PAIR_HIGHLIGHT, TABCHAR);
                        col += strlen(TABCHAR);
                    } else {
                        col++;
                        addch(page[index - 1]);
                    }

                    move(row, col);
                }
            } else if (input == CTRL('q')) {
                // CTRL-q stops the application
                running = false;
                stopped = true;
            } else {
                wrong++;
            }
        }

        // Go to the next page of the buffer
        page += page_size;
        index = 0;
    }

    if (result) {
        result->total += total;
        result->wrong += wrong;
    }

    return !stopped;
}

// Words Per Minute.
// The formula is (characters / 5) / minutes
static inline float wpm(size_t chars, clock_t time)
{
    return (3.0 * CLOCKS_PER_SEC * chars) / (250.0 * time);
}

// Accuracy percentage
static inline float accuracy(Result result)
{
    return (float) (result.total - result.wrong) / result.total * 100;
}

int main(int argc, char **argv)
{
    Buffer buffer = {0};
    Result result = {0};

    initscr();
    raw();
    cbreak();
    noecho();
    use_default_colors();
    start_color();
    init_pair(PAIR_NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(PAIR_SPECIAL, COLOR_BLUE, COLOR_BLACK);
    init_pair(PAIR_HIGHLIGHT, COLOR_YELLOW, COLOR_BLACK);

    clock_t start = clock();
    for (int i = 1; i < argc; ++i) {
        buffer_read(&buffer, argv[i]);
        if (!buffer_train(&buffer, &result))
            break;
    }
    clock_t time = clock() - start;

    endwin();
    buffer_free(&buffer);
    printf("WPM: %.2f\n", wpm(result.total, time));
    printf("Accuracy: %.2f\n", accuracy(result));
    return 0;
}
