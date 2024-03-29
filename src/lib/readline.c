/*
This file is part of fdbox
For license - read license.txt
*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/applet.h"
#include "lib/args.h"
#include "lib/environ.h"
#include "lib/readline.h"

#include "dos/history.h"
#include "dos/prompt.h"

#ifdef __TURBOC__
#include "dos.h"
#include "lib/tc202/stdbool.h"
#include "lib/tc202/stdextra.h"
#endif

#if defined(__WATCOMC__)
#include <conio.h>
#include <i86.h>

// this is needed for clrscr() re-implementation
#include <graph.h>

void clrscr() { _clearscreen(_GCLEARSCREEN); }
#endif

#if defined(_POSIX_C_SOURCE) || defined(__APPLE__)
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#endif

#if defined(__WIN32__)
#include <conio.h>
#include <io.h>
#include <stdbool.h>
#include <windows.h>

int read_char() {
        DWORD cc;
        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

        // console not found
        if (h == NULL) {
                return -1;
        }

        for (;;) {
                INPUT_RECORD irec;
                ReadConsoleInput(h, &irec, 1, &cc);

                if (irec.EventType == KEY_EVENT && irec.Event.KeyEvent.bKeyDown) {
                        if (irec.Event.KeyEvent.uChar.AsciiChar != 0) {
                                return irec.Event.KeyEvent.uChar.AsciiChar;
                        } else {
                                switch (irec.Event.KeyEvent.wVirtualScanCode) {
                                case KEY_ARROW_LEFT & 0x00ff:
                                        return KEY_ARROW_LEFT;
                                case KEY_ARROW_RIGHT & 0x00ff:
                                        return KEY_ARROW_RIGHT;
                                case KEY_ARROW_UP & 0x00ff:
                                        return KEY_ARROW_UP;
                                case KEY_ARROW_DOWN & 0x00ff:
                                        return KEY_ARROW_DOWN;
                                case KEY_HOME & 0x00ff:
                                        return KEY_HOME;
                                case KEY_END & 0x00ff:
                                        return KEY_END;
                                case KEY_PGDOWN & 0x00ff:
                                        return KEY_PGDOWN;
                                case KEY_PGUP & 0x00ff:
                                        return KEY_PGUP;
                                case VK_INSERT:
                                case 82:
                                        return KEY_INS;
                                }
                        }
                        /*
                                        //&& ! ((KEY_EVENT_RECORD&)irec.Event).wRepeatCount )
                                                printf("*** Pressed %d,%c  unicode=%x, scancode=%x,
                           keycode=%x, flags=%x\n", irec.Event.KeyEvent.uChar.AsciiChar,
                                                       irec.Event.KeyEvent.uChar.AsciiChar,
                                                       irec.Event.KeyEvent.uChar.UnicodeChar,
                                                       irec.Event.KeyEvent.wVirtualScanCode,
                                                        irec.Event.KeyEvent.wVirtualKeyCode,
                                                        irec.Event.KeyEvent.dwControlKeyState
                                                );
                                                return irec.Event.KeyEvent.uChar.AsciiChar;
                                                */
                }
        }
        return -1;
}

#elif defined(_POSIX_C_SOURCE) || defined(__APPLE__)
int read_char() {
        int i = getchar();
        switch (i) {
        case '\033':
                i = getchar();
                if (i == '[') {
                        i = getchar();
                        switch (i) {
                        case 'A':
                                return KEY_ARROW_UP;
                        case 'B':
                                return KEY_ARROW_DOWN;
                        case 'C':
                                return KEY_ARROW_RIGHT;
                        case 'D':
                                return KEY_ARROW_LEFT;
                        case 'H':
                                return KEY_HOME;
                        case 'F':
                                return KEY_END;
                        case '2':
                                i = getchar();
                                if (i == 126) {
                                        return KEY_INS;
                                } else {
                                        return 0;
                                }
                        case '3':
                                return KEY_DEL;
                        case '5':
                                return KEY_PGUP;
                        case '6':
                                return KEY_PGDOWN;
                        default:
                                printf("read escape code: %d\n", i);
                        }
                }
                return 0;
        case '\177':
                return '\b';
        default:
                return i;
        }
}
#define get_char_impl get_char_posix

#elif defined(__TURBOC__) || (defined(__WATCOMC__))
int read_char() {
        int c = getch();

        /* extended ASCII FTW */
        if (c == 0) {
                c = getch();
                switch (c) {
                case KEY_ARROW_LEFT & 0x00ff:
                        return KEY_ARROW_LEFT;
                case KEY_ARROW_RIGHT & 0x00ff:
                        return KEY_ARROW_RIGHT;
                case KEY_ARROW_UP & 0x00ff:
                        return KEY_ARROW_UP;
                case KEY_ARROW_DOWN & 0x00ff:
                        return KEY_ARROW_DOWN;
                case KEY_HOME & 0x00ff:
                        return KEY_HOME;
                case KEY_END & 0x00ff:
                        return KEY_END;
                case KEY_PGDOWN & 0x00ff:
                        return KEY_PGDOWN;
                case KEY_PGUP & 0x00ff:
                        return KEY_PGUP;
                case 82:
                        return KEY_INS;
                case 83:
                        return KEY_DEL;
                default:
                        return 0;
                }
        }
        return c;
}
#else
#error Undefined platform - we cannot read a line on a terminal
TODO - it seems that this platform is not supported yet -
    you need to define a function "read_char()" that reads a char without enter beeing pressed.
#endif

struct str_list history;

int read_string(char *line, size_t max_size) {
        int l;
        struct readline_session session;
        readline_session_init(&session);
        set_cursor_insert();
        session.line = line;
        session.max_size = max_size;
        l = readline(&session);
        readline_session_deinit(&session);
        return l;
}

#if defined(WIN32)
// https://docs.microsoft.com/en-us/windows/console/clearing-the-screen
void cls_win32(HANDLE hConsole) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        SMALL_RECT scrollRect;
        COORD scrollTarget;
        CHAR_INFO fill;

        // Get the number of character cells in the current buffer.
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
                return;
        }

        // Scroll the rectangle of the entire buffer.
        scrollRect.Left = 0;
        scrollRect.Top = 0;
        scrollRect.Right = csbi.dwSize.X;
        scrollRect.Bottom = csbi.dwSize.Y;

        // Scroll it upwards off the top of the buffer with a magnitude of the entire height.
        scrollTarget.X = 0;
        scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

        // Fill with empty spaces with the buffer's default text attribute.
        fill.Char.UnicodeChar = TEXT(' ');
        fill.Attributes = csbi.wAttributes;

        // Do the scroll
        ScrollConsoleScreenBuffer(hConsole, &scrollRect, NULL, scrollTarget, &fill);

        // Move the cursor to the top left corner too.
        csbi.dwCursorPosition.X = 0;
        csbi.dwCursorPosition.Y = 0;

        SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
}
#endif

void clear_screen() {
#if defined(_POSIX_C_SOURCE) || defined(__APPLE__)
        printf("\e[1;1H\e[2J");
#elif defined(WIN32)
        HANDLE hStdout;
        hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        cls_win32(hStdout);
#elif defined(__MSDOS__) || defined(__WATCOMC__)
        clrscr();
#else
        please implement clean screen
#endif
}

void set_cursor_insert() {
#if defined(_POSIX_C_SOURCE)
        printf("\e[1 q");
#elif defined(__APPLE__)
#elif defined(WIN32)
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_CURSOR_INFO cursorInfo;

        GetConsoleCursorInfo(out, &cursorInfo);
        cursorInfo.bVisible = true;
        cursorInfo.dwSize = 100;
        SetConsoleCursorInfo(out, &cursorInfo);
#elif defined(__MSDOS__) || defined(__WATCOMC__)
        union REGS xr;
        xr.h.ah = 1;
        xr.h.ch = 6;
        xr.h.cl = 7;
        int86(0x10, &xr, &xr);
#else
        please implement set cursor_block
#endif
}

void set_cursor_override() {
#if defined(_POSIX_C_SOURCE)
        printf("\e[3 q");
#elif defined(__APPLE__)
        // unsupported on apple, ok by me
#elif defined(WIN32)
        // https://stackoverflow.com/a/30126700
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_CURSOR_INFO cursorInfo;

        GetConsoleCursorInfo(out, &cursorInfo);
        cursorInfo.bVisible = true;
        cursorInfo.dwSize = 10;
        SetConsoleCursorInfo(out, &cursorInfo);
#elif defined(__MSDOS__) || defined(__WATCOMC__)
        /*
        https://jbwyatt.com/253/emu/8086_bios_and_dos_interrupts.html
        http://computer-programming-forum.com/29-pascal/7b38c5f9b15dfae6.htm
        http://computer-programming-forum.com/47-c-language/43c194b3ed78f2e2.htm
        */
        union REGS xr;
        xr.h.ah = 1;
        xr.h.ch = 0;
        xr.h.cl = 7;
        int86(0x10, &xr, &xr);
#else
        please implement set cursor_block
#endif
}

/* https://stackoverflow.com/a/1798833 */
#if defined(_POSIX_C_SOURCE) || defined(__APPLE__)
static struct termios oldt;
#endif

void readline_init() {
#if defined(_POSIX_C_SOURCE) || defined(__APPLE__)
        static struct termios newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
        str_list_init(&history, 10);
}

void readline_deinit() {
#ifdef _POSIX_C_SOURCE
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
}

void move_cursor_back(size_t n) {
        size_t i;
        for (i = 0; i < n; i++) {
                putchar('\b');
        }
}

void readline_session_init(struct readline_session *session) {
        session->line = NULL;
        session->max_size = 0;
        session->current_size = 0;
        session->current_history = 0;
        session->index = 0;
        session->override = false;
        session->free_memory = false;
}

void readline_session_allocate(struct readline_session *session, size_t max_size) {
        readline_session_init(session);
        session->line = malloc(max_size);
        session->max_size = max_size;
        session->free_memory = true;
}

void readline_session_deinit(struct readline_session *session) {
        if (session->free_memory) {
                free(session->line);
        }
        readline_session_init(session);
}

static void save_history(struct readline_session *session) {
        const char *last = str_list_get(&history, session->current_history);
        bool should_save = session->current_size != 0 && session->line[0] != ' ';
        if (last != NULL) {
                should_save &= strcmp(last, session->line) != 0;
        }
        if (should_save) {
                str_list_push(&history, session->line);
        }
}

int readline(struct readline_session *session) {
        session->line[0] = 0;
        while (session->index < session->max_size) {
                int c = read_char();
                switch (c) {
                case '\r':
                case '\n': {
                        save_history(session);
                        putchar('\n');
                        return session->current_size;
                }
                case 1: /* control +a */
                case KEY_HOME:
                        readline_move_home(session);
                        break;
                case 5: /* control +a */
                case KEY_END:
                        readline_move_end(session);
                        break;
                case KEY_ARROW_LEFT:
                        readline_move_left(session);
                        break;
                case KEY_ARROW_RIGHT:
                        readline_move_right(session);
                        break;
                case KEY_ARROW_UP: {
                        const char *last;
                        save_history(session);
                        last = str_list_get(&history, session->current_history + 0);
                        if (last != NULL) {
                                readline_set(session, last);
                        }
                        break;
                }
                case KEY_ARROW_DOWN:
                        if (session->current_history != 0) {
                                const char *last =
                                    str_list_get(&history, session->current_history - 1);
                                if (last != NULL) {
                                        readline_set(session, last);
                                }
                        }
                        break;
                case KEY_BACKSPACE:
                        session->index = readline_delete_left(session);
                        break;
                case KEY_DEL:
                        session->index = readline_delete_right(session);
                        break;
                case KEY_INS:
                        session->override = !session->override;
                        if (session->override) {
                                set_cursor_override();
                        } else {
                                set_cursor_insert();
                        }
                        break;
                case 4:
                        session->current_size = 0;
                        session->line[0] = 0;
                        return -1;
                case 12:
                        session->current_size = 0;
                        session->line[0] = 0;
                        clear_screen();
                        return 0;
                default:
                        if (session->override) {
                                session->current_size =
                                    readline_replace(session, session->index, c);
                                putchar(c);
                                if (session->index == session->current_size) {
                                        session->current_size++;
                                }
                                session->index++;
                        } else {
                                session->current_size = readline_insert(session, session->index, c);
                                session->index++;
                        }
                }
        }
        return session->current_size;
}

size_t readline_delete_left(struct readline_session *session) {
        size_t i;
        str_del_char(session->line, session->index - 1);
        move_cursor_back(session->index);
        printf("%s ", session->line);
        session->current_size = strlen(session->line);
        move_cursor_back(session->current_size + 1);
        session->index--;
        if (session->index > session->current_size) {
                session->index = session->current_size;
        }
        for (i = 0; i < session->index; i++) {
                putchar(session->line[i]);
        }
        fflush(stdout);
        return session->index;
}

size_t readline_delete_right(struct readline_session *session) {
        size_t i;
        str_del_char(session->line, session->index);
        move_cursor_back(session->index);
        printf("%s ", session->line);
        session->current_size = strlen(session->line);
        for (i = 0; i < session->index; i++) {
                putchar(session->line[i]);
        }
        fflush(stdout);
        return session->index;
}

size_t readline_replace(struct readline_session *session, size_t index, char c) {
        session->line[index] = c;
        if (session->index == session->current_size) {
                session->line[index + 1] = '\0';
        }
        putchar(c);
        putchar('\b');
        fflush(stdout);
        return session->current_size;
}

size_t readline_insert(struct readline_session *session, size_t index, char c) {
        str_ins_char(session->line, session->max_size, c, index);
        printf("%s", session->line + index);
        session->current_size = strlen(session->line);
        move_cursor_back(session->current_size - index - 1);

        index++;
        if (session->index > session->current_size) {
                session->current_size = index;
        }
        session->line[session->current_size] = 0;
        return session->current_size;
}

size_t readline_set(struct readline_session *session, const char *new_text) {
        size_t l;
        move_cursor_back(session->index);
        for (l = 0; l < session->current_size; l++) {
                putchar(' ');
        }
        move_cursor_back(session->current_size);
        l = strlen(new_text) + 1;
        memcpy(session->line, new_text, l);
        session->current_size = l;
        session->current_history++;
        session->index = l - 1;
        printf("%s", session->line);
        fflush(stdout);
        return l;
}

void readline_move_home(struct readline_session *session) {
        move_cursor_back(session->index);
        session->index = 0;
        fflush(stdout);
}

void readline_move_end(struct readline_session *session) {
        while (session->line[session->index] != '\0') {
                putchar(session->line[session->index]);
                session->index++;
        }
        fflush(stdout);
}

void readline_move_left(struct readline_session *session) {
        if (session->index > 0) {
                session->index--;
                putchar('\b');
        }
}

void readline_move_right(struct readline_session *session) {
        char next = session->line[session->index];
        if (session->index < session->max_size && next != '\0') {
                putchar(session->line[session->index]);
                session->index++;
        }
}

const char *readline_get_history(size_t i) { return str_list_get(&history, i); }
