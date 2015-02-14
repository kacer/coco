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


#pragma once


#include "config.h"
#include "cgp/cgp.h"
#include "archive.h"
#include "inputdata.h"
#include "predictors.h"


static const int PRED_CIRCULAR_TRIES = 3;


extern input_data_t *fitness_input_data;
extern archive_t fitness_cgp_archive;
extern archive_t fitness_pred_archive;
extern long fitness_cgp_evals;


/**
 * Problem-specific functions
 */
typedef void (*fitness_init_func_t)(
    config_t *config,
    input_data_t *input,
    archive_t cgp_archive,
    archive_t pred_archive);

typedef ga_fitness_t (*fitness_cgp_eval_func_t)(
    ga_chr_t chr);

typedef ga_fitness_t (*fitness_cgp_predict_func_t)(
    ga_chr_t cgp_chr,
    ga_chr_t pred_chr);


/**
 * Private functions, defined in ifilter/fitness.c or symreg/fitness.c
 */
void _fitness_init(config_t *config, input_data_t *input, archive_t cgp_archive, archive_t pred_archive);
ga_fitness_t _fitness_predict_cgp_by_genome(ga_chr_t cgp_chr, pred_genome_t predictor);


#ifdef SYMREG
    #include "symreg/fitness.h"
#else
    #include "ifilter/fitness.h"
#endif


/**
 * Initializes fitness module
 * @param config
 * @param input
 * @param cgp_archive
 * @param pred_archive
 */
void fitness_init(config_t *config, input_data_t *input,
    archive_t cgp_archive, archive_t pred_archive);


/**
 * Deinitialize fitness module internals
 */
void fitness_deinit();


/**
 * Returns number of performed CGP evaluations
 */
static inline long fitness_get_cgp_evals()
{
    return fitness_cgp_evals;
}


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_cgp(ga_chr_t chr);


/**
 * Predictes CGP circuit fitness
 *
 * @param  cgp_chr
 * @param  pred_chr
 * @return fitness value
 */
ga_fitness_t fitness_predict_cgp(ga_chr_t cgp_chr, ga_chr_t pred_chr);


/**
 * If predictors archive is empty, returns `fitness_eval_cgp` result.
 * If there is at least one predictor in archive
 * returns predicted fitness using first predictor in archive.
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_or_predict_cgp(ga_chr_t chr);


/**
 * Evaluates predictor fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_predictor(ga_chr_t chr);


/**
 * Evaluates circular predictor fitness, using PRED_CIRCULAR_TRIES to
 * determine best offset
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_circular_predictor(ga_chr_t pred_chr);


/**
 * Computes real PSNR value from fitness value
 */
static inline double fitness_to_psnr(ga_fitness_t f) {
    return 10 * log10(f);
}


/**
 * Fills simd-friendly predictor arrays with correct image data
 * @param  genome
 */
void fitness_prepare_predictor_for_simd(pred_genome_t predictor);
