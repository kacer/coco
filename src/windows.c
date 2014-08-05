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

#include "windows.h"
#include "cgp.h"


// SIDWINCH handler
// volatile sig_atomic_t resized = 0;
// void resize_handler(int _)
// {
//     signal(SIDWINCH, sigint_handler);
//     resized = 1;
// }


void _init_subwindow(WINDOW **win, WINDOW **brd, char* title, int height, int width, int starty, int startx);
WINDOW *_win_new(int height, int width, int starty, int startx, bool border);
void _win_destroy(WINDOW *local_win);
void _win_print_title(WINDOW *win, char *title);

WINDOW *border_progress_window;
WINDOW *border_slowlog_window;
WINDOW *border_circuit_window;

WINDOW *title_window;
WINDOW *progress_window;
WINDOW *circuit_window;
WINDOW *slowlog_window;


bool _nc_progress_first = true;
bool _nc_slowlog_first = true;


#define COLOR_APP_TITLE 1
#define COLOR_WIN_TITLE 2

#define APP_TITLE "Colearning in Coevolutionary Algorithms, Bc. Michal Wiglasz, xwigla00@stud.fit.vutbr.cz"

#define CIRCUIT_HEIGHT (cgp_dump_chr_asciiart_height() + 2)
#define CIRCUIT_WIDTH COLS
#define CIRCUIT_TOP (LINES - CIRCUIT_HEIGHT)
#define CIRCUIT_LEFT 0
#define CIRCUIT_TITLE "Best circuit found"

#define PROGRESS_TOP 2
#define PROGRESS_LEFT 0
#define PROGRESS_HEIGHT (CIRCUIT_TOP - PROGRESS_TOP - 1)
#define PROGRESS_WIDTH (COLS / 2)
#define PROGRESS_TITLE "Live progress"

#define SLOWLOG_TOP PROGRESS_TOP
#define SLOWLOG_LEFT (COLS - PROGRESS_WIDTH + PROGRESS_LEFT)
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

    title_window = _win_new(1, COLS, 0, 0, false);
    wbkgd(title_window, COLOR_PAIR(COLOR_APP_TITLE));
    whline(title_window, 0, COLS);
    mvwprintw(title_window, 0, 1, " ");
    wprintw(title_window, APP_TITLE);
    wprintw(title_window, " ");
    wrefresh(title_window);


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
        default: return none;
    }
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
