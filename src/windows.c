/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master Thesis
 * 2014/2015
 *
 * Supervisor: Ing. Michaela Šikulová <isikulova@fit.vutbr.cz>
 *
 * Faculty of Information Technologies
 * Brno University of Technology
 * http://www.fit.vutbr.cz/
 *
 * Started on 28/07/2014.
 *      _       _
 *   __(.)=   =(.)__
 *   \___)     (___/
 */


#include <signal.h>
#include <sys/ioctl.h>

#include "windows.h"
#include "cgp.h"


// SIDWINCH handler
// volatile sig_atomic_t resized = 0;
// void resize_handler(int _)
// {
//     signal(SIDWINCH, sigint_handler);
//     resized = 1;
// }


int cols = -1;
int lines = -1;

static inline int get_cols() {
    return (cols >= 0)? cols : COLS;
}

static inline int get_lines() {
    return (lines >= 0)? lines : LINES;
}


void _init_subwindow(WINDOW **win, WINDOW **brd, char* title, int height, int width, int starty, int startx);
WINDOW *_init_statusline(char* text, int starty, chtype color);
WINDOW *_win_new(int height, int width, int starty, int startx, bool border);
void _win_destroy(WINDOW *local_win);
void _win_print_title(WINDOW *win, char *title);
void on_resize();

WINDOW *border_progress_window;
WINDOW *border_slowlog_window;
WINDOW *border_circuit_window;

WINDOW *title_window;
WINDOW *statusbar_window;
WINDOW *progress_window;
WINDOW *circuit_window;
WINDOW *slowlog_window;


bool _nc_progress_first = true;
bool _nc_slowlog_first = true;


#define COLOR_APP_TITLE 1
#define COLOR_WIN_TITLE 2
#define COLOR_STATUSBAR COLOR_APP_TITLE

#define APP_TITLE "Colearning in Coevolutionary Algorithms, Bc. Michal Wiglasz, xwigla00@stud.fit.vutbr.cz"
#define STATUSBAR_TEXT "Hotkeys: s = store current state, q = quit"

#define CIRCUIT_HEIGHT (cgp_dump_chr_asciiart_height() + 2)
#define CIRCUIT_WIDTH get_cols()
#define CIRCUIT_TOP (get_lines() - CIRCUIT_HEIGHT - 2)
#define CIRCUIT_LEFT 0
#define CIRCUIT_TITLE "Best circuit found"

#define PROGRESS_TOP 2
#define PROGRESS_LEFT 0
#define PROGRESS_HEIGHT (CIRCUIT_TOP - PROGRESS_TOP - 1)
#define PROGRESS_WIDTH (get_cols() / 2)
#define PROGRESS_TITLE "Live progress"

#define SLOWLOG_TOP PROGRESS_TOP
#define SLOWLOG_LEFT (get_cols() - PROGRESS_WIDTH + PROGRESS_LEFT)
#define SLOWLOG_HEIGHT PROGRESS_HEIGHT
#define SLOWLOG_WIDTH PROGRESS_WIDTH
#define SLOWLOG_TITLE "Slow log"


void windows_init()
{
    initscr();
    start_color();
    use_default_colors();
    timeout(0);   // non-blocking getch()
    noecho();     // do not echo chars from input
    curs_set(0);  // hide cursor

    init_pair(COLOR_APP_TITLE, COLOR_RED, COLOR_YELLOW);
    init_pair(COLOR_WIN_TITLE, COLOR_RED, -1);
    refresh();


    // app title

    title_window = _init_statusline(APP_TITLE, 0, COLOR_PAIR(COLOR_APP_TITLE));


    // status bar
    statusbar_window = _init_statusline(STATUSBAR_TEXT, get_lines() - 1, COLOR_PAIR(COLOR_STATUSBAR));


    // subwindows

    _init_subwindow(&progress_window, &border_progress_window, PROGRESS_TITLE,
        PROGRESS_HEIGHT, PROGRESS_WIDTH, PROGRESS_TOP, PROGRESS_LEFT);
    scrollok(progress_window, true);

    _init_subwindow(&slowlog_window, &border_slowlog_window, SLOWLOG_TITLE,
        SLOWLOG_HEIGHT, SLOWLOG_WIDTH, SLOWLOG_TOP, SLOWLOG_LEFT);
    scrollok(slowlog_window, true);

    _init_subwindow(&circuit_window, &border_circuit_window, CIRCUIT_TITLE,
        CIRCUIT_HEIGHT, CIRCUIT_WIDTH, CIRCUIT_TOP, CIRCUIT_LEFT);

    //signal(SIDWINCH, sigint_handler);

    doupdate();
}


void windows_destroy()
{
    delwin(border_progress_window);
    delwin(border_slowlog_window);
    delwin(border_circuit_window);
    delwin(title_window);
    delwin(statusbar_window);
    delwin(progress_window);
    delwin(circuit_window);
    delwin(slowlog_window);
    endwin();
}


windows_event windows_check_events()
{
    int c = getch();
    if (c == ERR) return none;

    switch (c) {
        case 's': return save_state;
        case 'q': return quit;
        case 'p': return pause;
        //case 'r': on_resize(); return unknown;
        default: return unknown;
    }
}


void on_resize()
{
    struct winsize size = {};
    ioctl(0, TIOCGWINSZ, &size);
    if (size.ws_row && size.ws_col) {
        lines = size.ws_row;
        cols = size.ws_col;
        resizeterm(size.ws_row, size.ws_col);
        clearok(stdscr, TRUE);
    }

    touchwin(border_progress_window);
    touchwin(border_slowlog_window);
    touchwin(border_circuit_window);
    touchwin(title_window);
    touchwin(statusbar_window);
    touchwin(progress_window);
    touchwin(circuit_window);
    touchwin(slowlog_window);

    wrefresh(border_progress_window);
    wrefresh(border_slowlog_window);
    wrefresh(border_circuit_window);
    wrefresh(title_window);
    wrefresh(statusbar_window);
    wrefresh(progress_window);
    wrefresh(circuit_window);
    wrefresh(slowlog_window);

    touchwin(stdscr);
    refresh();
}


void _win_print_title(WINDOW *win, char *title)
{
    wattron(win, COLOR_PAIR(COLOR_WIN_TITLE));
    mvwprintw(win, 0, 1, "[");
    wprintw(win, title);
    wprintw(win, "]");
    wattroff(win, COLOR_PAIR(COLOR_WIN_TITLE));
    wrefresh(win);
}


void _init_subwindow(WINDOW **win, WINDOW **brd, char* title, int height, int width, int starty, int startx)
{
    *brd = _win_new(height, width, starty, startx, true);
    *win = _win_new(height - 2, width - 2, starty + 1, startx + 1, false);
    _win_print_title(*brd, title);
}


WINDOW *_init_statusline(char* text, int starty, chtype color)
{
    WINDOW *win = _win_new(1, COLS, starty, 0, false);
    wbkgd(win, color);
    whline(win, 0, COLS);
    mvwprintw(win, 0, 1, " ");
    wprintw(win, text);
    wprintw(win, " ");
    wrefresh(win);
    return win;
}


WINDOW *_win_new(int height, int width, int starty, int startx, bool border)
{
    WINDOW *local_win = newwin(height, width, starty, startx);
    if (border) box(local_win, 0 , 0);
    wrefresh(local_win);
    return local_win;
}


void _win_destroy(WINDOW *local_win)
{
    /* box(local_win, ' ', ' '); : This won't produce the desired
     * result of erasing the window. It will leave it's four corners
     * and so an ugly remnant of window.
     */
    wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
    /* The parameters taken are
     * 1. win: the window on which to operate
     * 2. ls: character to be used for the left side of the window
     * 3. rs: character to be used for the right side of the window
     * 4. ts: character to be used for the top side of the window
     * 5. bs: character to be used for the bottom side of the window
     * 6. tl: character to be used for the top left corner of the window
     * 7. tr: character to be used for the top right corner of the window
     * 8. bl: character to be used for the bottom left corner of the window
     * 9. br: character to be used for the bottom right corner of the window
     */
    wrefresh(local_win);
    delwin(local_win);
}
