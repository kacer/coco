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


/* standard debug logging */


#ifdef _OPENMP
    #include <omp.h>
    #define LOG_THREAD_IDENT(stream) { fprintf((stream), "thread %02d: ", omp_get_thread_num()); }
#else
    #define LOG_THREAD_IDENT(stream)
#endif


#ifdef DEBUG

    #define DEBUGLOG(...) { \
        LOG_THREAD_IDENT(stderr); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    }

#else

    #define DEBUGLOG(...)

#endif


/* verbose debug logging */

#ifdef VERBOSE

    #define VERBOSELOG DEBUGLOG

#else

    #define VERBOSELOG(...)

#endif
