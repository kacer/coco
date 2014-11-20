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


#include <sys/time.h>

#include "../ga.h"


/*
    Important events:

    algo started
    algo terminated

    better cgp found
    baldwin triggered
    log interval

    better predictor found
    predictor length changed

    sigint received
    sigxcpu received
    target generation
    target fitness

 */


typedef enum {
    generation_limit,
    target_fitness,
    signal_received,
} finish_reason_t;


/* solves cyclic type dependency */
struct logger_base;
typedef struct logger_base *logger_t;

/* event handlers */
typedef void (*handler_started_t)(logger_t logger);
typedef void (*handler_finished_t)(logger_t logger, finish_reason_t reason);
typedef void (*handler_better_cgp_t)(logger_t logger, ga_fitness_t predicted_fitness, ga_fitness_t real_fitness);
typedef void (*handler_baldwin_triggered_t)(logger_t logger);
typedef void (*handler_log_tick_t)(logger_t logger);
typedef void (*handler_better_pred_t)(logger_t logger);
typedef void (*handler_pred_length_changed_t)(logger_t logger, unsigned int old_length, unsigned int new_length);
typedef void (*handler_signal_t)(logger_t logger, int signal);

/* "destructor" */
typedef void (*logger_destructor_t)(logger_t logger);


struct logger_base {
    /* time */
    struct timeval usertime_start;
    struct timeval wallclock_start;

    /* event handlers */
    handler_started_t handler_started;
    handler_finished_t handler_finished;
    handler_better_cgp_t handler_better_cgp;
    handler_baldwin_triggered_t handler_baldwin_triggered;
    handler_log_tick_t handler_log_tick;
    handler_better_pred_t handler_better_pred;
    handler_pred_length_changed_t handler_pred_length_changed;
    handler_signal_t handler_signal;

    /* "destructor" */
    logger_destructor_t destructor;
};


/**
 * Base logger initializer
 */
void logger_init_time(logger_t logger);


/**
 * Destroy logger
 */
static inline void logger_destroy(logger_t logger)
{
    logger->destructor(logger);
}


/**
 * Returns elapsed usertime
 */
struct timeval logger_get_usertime(logger_t logger);


/**
 * Returns elapsed wall clock
 */
struct timeval logger_get_wallclock(logger_t logger);
