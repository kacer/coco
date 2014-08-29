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


#pragma once


#include <ncurses.h>


extern WINDOW *border_progress_window;
extern WINDOW *border_slowlog_window;
extern WINDOW *border_circuit_window;

extern WINDOW *progress_window;
extern WINDOW *circuit_window;
extern WINDOW *slowlog_window;


extern bool _nc_progress_first;
extern bool _nc_slowlog_first;


typedef enum {
    none,
    unknown,
    save_state,
    quit,
    pause,
} windows_event;


#define _NC_PROGRESS(...) { \
    if (!_nc_progress_first) wprintw(progress_window, "\n"); \
    else _nc_progress_first = false; \
    wprintw(progress_window, __VA_ARGS__); \
    wrefresh(progress_window); \
}

#define _NC_SLOWLOG(...) { \
    if (!_nc_slowlog_first) wprintw(slowlog_window, "\n"); \
    else _nc_slowlog_first = false; \
    wprintw(slowlog_window, __VA_ARGS__); \
    wrefresh(slowlog_window); \
}


void windows_init();
void windows_destroy();

windows_event windows_check_events();
