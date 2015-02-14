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
    /* fixes linter */
    #define SYMREG
#endif


#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../cpu.h"
#include "../random.h"
#include "../fitness.h"
#include "fitness.h"
#include "inputdata.h"


static double _epsilon;


/** Public and "friend" API ***************************************************/


/**
 * Initializes fitness module
 * @param config
 * @param input
 * @param cgp_archive
 * @param pred_archive
 */
void _fitness_init(config_t *config, input_data_t *input,
    archive_t cgp_archive, archive_t pred_archive)
{
    _epsilon = config->epsilon;
}


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_cgp(ga_chr_t chr)
{
    unsigned int hits = 0;

    for (int i = 0; i < fitness_input_data->fitness_cases; i++) {
        cgp_value_t *inputs = &fitness_input_data->inputs[INPUT_IDX(i, 0)];
        cgp_value_t target_output = fitness_input_data->outputs[i];
        cgp_value_t cgp_output;
        bool should_restart = cgp_get_output(chr, inputs, &cgp_output);
        if (should_restart) {
            i = 0;
            hits = 0;
            continue;
        }

        if (fabs(target_output - cgp_output) < _epsilon) {
            hits++;
        }
    }
    #pragma omp atomic
        fitness_cgp_evals += fitness_input_data->fitness_cases;

    return (100.0 * hits) / fitness_input_data->fitness_cases;
}


/**
 * Predictes CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_predict_cgp(ga_chr_t cgp_chr, ga_chr_t pred_chr)
{
    pred_genome_t predictor = (pred_genome_t) pred_chr->genome;
    unsigned int hits = 0;

    for (int i = 0; i < predictor->used_pixels; i++) {
        pred_gene_t index = predictor->pixels[i];
        assert(index < fitness_input_data->fitness_cases);

        cgp_value_t *inputs = &fitness_input_data->inputs[INPUT_IDX(index, 0)];
        cgp_value_t target_output = fitness_input_data->outputs[index];
        cgp_value_t cgp_output;

        bool should_restart = cgp_get_output(cgp_chr, inputs, &cgp_output);
        if (should_restart) {
            i = 0;
            hits = 0;
            continue;
        }

        if (fabs(target_output - cgp_output) < _epsilon) {
            hits++;
        }
    }
    #pragma omp atomic
        fitness_cgp_evals += predictor->used_pixels;

    return (100.0 * hits) / predictor->used_pixels;
}




/**
 * Fills simd-friendly predictor arrays with correct image data
 * @param  genome
 */
void fitness_prepare_predictor_for_simd(pred_genome_t predictor)
{
    // SIMD is not supported
    assert(false);
}
