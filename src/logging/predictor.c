/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master's Thesis
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
#include <string.h>

#include "predictor.h"
#include "../utils.h"


struct logger_predictor {
    struct logger_base base;  // must be first!
    FILE *log_file;
};
typedef struct logger_predictor *logger_predictor_t;


/**
 * Extracts file pointer from logger "object"
 * @param  logger
 * @return
 */
static inline FILE *_get_fp(logger_t logger) {
    return ((logger_predictor_t) logger)->log_file;
}

#define _USERTIME_STR \
    char _usertime_str[100]; \
    logger_snprintf_usertime(logger, _usertime_str, 100);


/* event handlers */
static void handle_better_pred(logger_t logger, int cgp_generation, ga_fitness_t old_fitness, ga_fitness_t new_fitness, ga_chr_t active_predictor);
static void handle_pred_length_change_applied(logger_t logger, int cgp_generation, unsigned int old_length, unsigned int new_length,
    unsigned int old_used_length, unsigned int new_used_length,
    ga_chr_t active_predictor);

/* "destructor" */
static void logger_predictor_destruct(logger_t logger);


/**
 * Create predictor logger
 * @param logger
 */
logger_t logger_predictor_create(config_t *config, FILE *target)
{
    assert(target != NULL);

    logger_predictor_t logger = (logger_predictor_t) malloc(sizeof(struct logger_predictor));
    if (logger == NULL) return NULL;

    logger->log_file = target;

    // this is the same as &logger->base
    logger_t base = (logger_t) logger;

    logger_init_base(base, config);
    base->handler_better_pred = handle_better_pred;
    base->handler_pred_length_change_applied = handle_pred_length_change_applied;
    base->destructor = logger_predictor_destruct;

    return base;
}


/**
 * Frees any resources allocated by predictor logger
 */
static void logger_predictor_destruct(logger_t logger)
{
    free(logger);
}


static void _log_predictor(logger_t logger, int cgp_generation, ga_chr_t active_predictor)
{
    pred_genome_t genome = (pred_genome_t) active_predictor->genome;

    fprintf(_get_fp(logger), "Generation %d: Predictor phenotype length %d [ ",
        cgp_generation, genome->used_pixels);

    for (int i = 0; i < genome->used_pixels; i++) {
        pred_gene_t index = genome->pixels[i];
        fprintf(_get_fp(logger), "%d ", index);
    }

    fprintf(_get_fp(logger), "]\n");
}


static void handle_better_pred(logger_t logger, int cgp_generation,
    ga_fitness_t old_fitness, ga_fitness_t new_fitness,
    ga_chr_t active_predictor)
{
    _log_predictor(logger, cgp_generation, active_predictor);
}


static void handle_pred_length_change_applied(logger_t logger, int cgp_generation,
    unsigned int old_length, unsigned int new_length,
    unsigned int old_used_length, unsigned int new_used_length,
    ga_chr_t active_predictor)
{
    _log_predictor(logger, cgp_generation, active_predictor);
}
