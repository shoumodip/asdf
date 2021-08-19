#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>

/*
 * Color pairs used in the typing application
 */
typedef enum {
    COLOR_NORMAL,
    COLOR_SPECIAL,
    COLOR_HIGHLIGHT,
} Color;

/*
 * Special characters used for legibility puposes
 */
#define NEWLINE "$\n"
#define TABCHAR "----"

/*
 * Turn a special color on
 */
#define COLOR_ON(color) attron(COLOR_PAIR(color))

/*
 * Turn a special color off
 */
#define COLOR_OFF(color) attroff(COLOR_PAIR(color))

/*
 * Mask for comparing against CTRL-pressed characters
 */
#define CTRL(k) ((k) & 0x1f)

/*
 * A growable character array for storing the contents of files to
 * practise
 */
typedef struct {
    char *text;
    size_t capacity;
} Buffer;

/*
 * The results of the typing practise
 */
typedef struct {
    size_t total;
    size_t wrong;
    clock_t time;
} Result;

/*
 * Free the text in a buffer and reset it
 * @param buffer *Buffer The buffer to free
 */
void buffer_free(Buffer *buffer)
{
    if (buffer->text) free(buffer->text);
    buffer->capacity = 0;
}

/*
 * Read a file into a buffer
 * @param buffer *Buffer The buffer to read the file into
 * @param file_name *char The file to read into the buffer
 */
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

/*
 * Print a string in the COLOR_SPECIAL color
 * @param text *char The text to print in the special color
 * @param color Color The color to switch to after the text
 */
void print_special(const char *text, Color color)
{
    COLOR_ON(COLOR_SPECIAL);
    printw(text);
    COLOR_OFF(COLOR_SPECIAL);
    COLOR_ON(color);
}

/*
 * Print a character with special characters and colors
 * @param ch char The character to print
 * @param index *size_t The index of the cursor
 * @param color Color The color to use as the default
 */
void print_char(char ch, size_t *index, Color color)
{
    switch (ch) {
    case '\n':
        if (index) *index += 1 + COLS - *index % COLS;
        print_special(NEWLINE, color);
        return;
    case '\t':
        if (index) *index += strlen(TABCHAR);
        print_special(TABCHAR, color);
        return;
    default:
        if (index) *index += 1;
        addch(ch);
        return;
    }
}

/*
 * Print a buffer
 * @param buffer *Buffer The buffer to print
 * @param limit *size_t The ending index of the buffer in the screen
 * @return size_t the starting index of the buffer in the screen
 */
size_t buffer_print(const Buffer *buffer, size_t *limit)
{
    clear();
    COLOR_OFF(COLOR_HIGHLIGHT);
    COLOR_ON(COLOR_NORMAL);

    size_t start = *limit;
    size_t grid_size = LINES * COLS;
    size_t text_size = strlen(buffer->text);
    for (size_t i = 0; i < grid_size && *limit < text_size; *limit += 1)
        print_char(buffer->text[*limit], &i, COLOR_NORMAL);

    COLOR_ON(COLOR_HIGHLIGHT);
    move(0, 0);

    return start;
}

/*
 * Practise on a buffer
 * @param buffer *Buffer The buffer to practise on
 * @param result *Result The result to update
 * @return bool If the user pressed CTRL-q
 */
bool buffer_practise(const Buffer *buffer, Result *result)
{
    char input;
    size_t limit = 0;

    clock_t start = 0;
    while (limit < strlen(buffer->text)) {
        for (size_t i = buffer_print(buffer, &limit); i < limit; ) {
            input = getch();
            if (i == 0) start = clock(); // Start the clock on first key press

            if (input == CTRL('q')) {
                result->time += clock() - start;
                return false;
            } else if (input == buffer->text[i]) {
                print_char(buffer->text[i++], NULL, COLOR_HIGHLIGHT);
                result->total++;
            } else {
                result->wrong++;
            }
        }
    }

    result->time += clock() - start;
    return true;
}

/*
 * Words Per Minute calculation: (characters / 5) / minutes
 * @param result Result The result of the typing practise
 * @return float The words per minute of the user
 */
static inline float wpm(Result result)
{
    return (3.0 * CLOCKS_PER_SEC * result.total) / (250.0 * result.time);
}

/*
 * Accuracy percentage calculation: ((total - wrong) * 100) / total
 * @param result Result The result of the typing practise
 * @return float The accuracy percentage of the user
 */
static inline float accuracy(Result result)
{
    return (float) ((result.total - result.wrong) * 100.0) / result.total;
}

/*
 * Initialize the ncurses window of the typing application
 */
static inline void ncurses_init(void)
{
    initscr();
    raw();
    noecho();
    use_default_colors();
    start_color();
    init_pair(COLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_SPECIAL, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_HIGHLIGHT, COLOR_YELLOW, COLOR_BLACK);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: asdf [FILE]\n");
        return 1;
    }

    Buffer buffer = {0};
    Result result = {0};

    ncurses_init();
    for (int i = 1; i < argc; ++i) {
        buffer_read(&buffer, argv[i]);
        if (!buffer_practise(&buffer, &result)) break;
    }
    endwin();

    buffer_free(&buffer);

    printf("WPM: %.2f\n", wpm(result));
    printf("Accuracy: %.2f\n", accuracy(result));

    return 0;
}
