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


#ifndef SYMREG
    /* to confuse linter */
    #define SYMREG
#endif


#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cpu.h"
#include "random.h"
#include "fitness.h"
#include "inputdata.h"


input_data_t *fitness_input_data;
archive_t fitness_cgp_archive;
archive_t fitness_pred_archive;
long fitness_cgp_evals;


/**
 * Initializes fitness module
 * @param config
 * @param input
 * @param cgp_archive
 * @param pred_archive
 */
void fitness_init(config_t *config, input_data_t *input,
    archive_t cgp_archive, archive_t pred_archive)
{
    fitness_input_data = input;
    fitness_cgp_archive = cgp_archive;
    fitness_pred_archive = pred_archive;
    fitness_cgp_evals = 0;
    _fitness_init(config, input, cgp_archive, pred_archive);
}


/**
 * Deinitialize fitness module internals
 */
void fitness_deinit()
{
}


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_or_predict_cgp(ga_chr_t chr)
{
    if (fitness_pred_archive && fitness_pred_archive->stored > 0) {
        return fitness_predict_cgp(chr, arc_get(fitness_pred_archive, 0));
    } else {
        return fitness_eval_cgp(chr);
    }
}


/**
 * Evaluates predictor fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_predictor(ga_chr_t pred_chr)
{
    double sum = 0;
    for (int i = 0; i < fitness_cgp_archive->stored; i++) {
        ga_chr_t cgp_chr = arc_get(fitness_cgp_archive, i);
        double predicted = fitness_predict_cgp(cgp_chr, pred_chr);
        sum += fabs(cgp_chr->fitness - predicted);
    }
    return sum / fitness_cgp_archive->stored;
}


/**
 * Evaluates circular predictor fitness, using PRED_CIRCULAR_TRIES to
 * determine best offset
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_circular_predictor(ga_chr_t pred_chr)
{
    pred_genome_t predictor = (pred_genome_t) pred_chr->genome;
    int best_offset = predictor->_circular_offset;
    ga_fitness_t best_fitness = fitness_eval_predictor(pred_chr);

    for (int i = 0; i < PRED_CIRCULAR_TRIES; i++) {
        // generate new phenotype
        int offset = rand_urange(0, pred_get_max_length() - 1);
        predictor->_circular_offset = offset;
        pred_calculate_phenotype(predictor);

        // calculate predictor fitness
        ga_fitness_t fit = fitness_eval_predictor(pred_chr);

        // if it is better, store it
        if (ga_is_better(PRED_PROBLEM_TYPE, fit, best_fitness)) {
            best_offset = offset;
            best_fitness = fit;
        }
    }

    // set predictor to best found phenotype
    if (predictor->_circular_offset != best_offset) {
        predictor->_circular_offset = best_offset;
        pred_calculate_phenotype(predictor);
    }
    return best_fitness;
}

