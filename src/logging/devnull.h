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

#include <stdlib.h>

#include "base.h"


/* devnull logger type definition */
typedef logger_t logger_devnull_t;


/**
 * Empty event handlers
 */

static inline void logger_devnull_started(logger_t logger) {}
static inline void logger_devnull_finished(logger_t logger, finish_reason_t reason) {}
static inline void logger_devnull_better_cgp(logger_t logger, ga_fitness_t predicted_fitness, ga_fitness_t real_fitness) {}
static inline void logger_devnull_baldwin_triggered(logger_t logger) {}
static inline void logger_devnull_log_tick(logger_t logger) {}
static inline void logger_devnull_better_pred(logger_t logger) {}
static inline void logger_devnull_pred_length_changed(logger_t logger, unsigned int old_length, unsigned int new_length) {}
static inline void logger_devnull_signal(logger_t logger, int signal) {}


/**
 * Create devnull logger (which does nothing)
 * @param logger
 */
static inline logger_devnull_t logger_devnull_create()
{
    logger_devnull_t logger = (logger_devnull_t) malloc(sizeof(struct logger_base));
    if (logger == NULL) return NULL;

    logger_init_time(logger);
    logger->handler_started = logger_devnull_started;
    logger->handler_finished = logger_devnull_finished;
    logger->handler_better_cgp = logger_devnull_better_cgp;
    logger->handler_baldwin_triggered = logger_devnull_baldwin_triggered;
    logger->handler_log_tick = logger_devnull_log_tick;
    logger->handler_better_pred = logger_devnull_better_pred;
    logger->handler_pred_length_changed = logger_devnull_pred_length_changed;
    logger->handler_signal = logger_devnull_signal;
    return logger;
}


