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

#include <stdlib.h>
#include <assert.h>

#include "text.h"


struct logger_text {
    struct logger_base base;  // must be first!
    FILE *log_file;
};
typedef struct logger_text *logger_text_t;


/**
 * Extracts file pointer from logger "object"
 * @param  logger
 * @return
 */
static inline FILE *_get_fp(logger_t logger) {
    return ((logger_text_t) logger)->log_file;
}


/* formatting */
#define FITNESS_FMT "%.10g"


/* event handlers */
void logger_text_started(logger_t logger);
void logger_text_finished(logger_t logger, finish_reason_t reason);
void logger_text_better_cgp(logger_t logger, ga_fitness_t predicted_fitness, ga_fitness_t real_fitness);
void logger_text_baldwin_triggered(logger_t logger);
void logger_text_log_tick(logger_t logger);
void logger_text_better_pred(logger_t logger);
void logger_text_pred_length_changed(logger_t logger, unsigned int old_length, unsigned int new_length);
void logger_text_signal(logger_t logger, int signal);

/* "destructor" */
void logger_text_destruct(logger_t logger);


/**
 * Create text logger
 * @param logger
 */
logger_t logger_text_create(FILE *target)
{
    assert(target != NULL);

    logger_text_t logger = (logger_text_t) malloc(sizeof(struct logger_text));
    if (logger == NULL) return NULL;

    logger->log_file = target;

    // this is the same as &logger->base
    logger_t base = (logger_t) logger;

    logger_init_time(base);
    base->handler_started = logger_text_started;
    base->handler_finished = logger_text_finished;
    base->handler_better_cgp = logger_text_better_cgp;
    base->handler_baldwin_triggered = logger_text_baldwin_triggered;
    base->handler_log_tick = logger_text_log_tick;
    base->handler_better_pred = logger_text_better_pred;
    base->handler_pred_length_changed = logger_text_pred_length_changed;
    base->handler_signal = logger_text_signal;
    base->destructor = logger_text_destruct;

    return base;
}


/**
 * Frees any resources allocated by text logger
 */
void logger_text_destruct(logger_t logger)
{
    free(logger);
}


void logger_text_started(logger_t logger)
{
    fprintf(_get_fp(logger), "Evolution starts now.\n");
}


void logger_text_finished(logger_t logger, finish_reason_t reason)
{

}


void logger_text_better_cgp(logger_t logger, ga_fitness_t predicted_fitness, ga_fitness_t real_fitness)
{
    fprintf(_get_fp(logger), "Better filter found. "
        "Predicted / real = " FITNESS_FMT " / " FITNESS_FMT "\n",
        predicted_fitness, real_fitness);
}


void logger_text_baldwin_triggered(logger_t logger)
{

}


void logger_text_log_tick(logger_t logger)
{

}


void logger_text_better_pred(logger_t logger)
{

}


void logger_text_pred_length_changed(logger_t logger, unsigned int old_length, unsigned int new_length)
{

}


void logger_text_signal(logger_t logger, int signal)
{



}
