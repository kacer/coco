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


#include "cgp.h"
#include "image.h"
#include "archive.h"
#include "predictors.h"


/**
 * Initializes fitness module - prepares test image
 * @param original
 * @param noisy
 * @param cgp_archive
 * @param pred_archive
 */
void fitness_init(img_image_t original, img_image_t noisy,
    archive_t cgp_archive, archive_t pred_archive);


/**
 * Deinitialize fitness module internals
 */
void fitness_deinit();


/**
 * Returns number of performed CGP evaluations
 */
long fitness_get_cgp_evals();


/**
 * Filters image using given filter. Caller is responsible for freeing
 * the filtered image
 *
 * Works in single thread
 *
 * @param  chr
 * @return fitness value
 */
img_image_t fitness_filter_image(ga_chr_t chr);


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_cgp(ga_chr_t chr);


/**
 * If predictors archive is empty, returns `fitness_eval_cgp` result.
 * If there is at least one predictor in archive
 * returns `fitness_predict_cgp` result using first predictor in archive.
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_or_predict_cgp(ga_chr_t chr);


/**
 * Predictes CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_predict_cgp(ga_chr_t cgp_chr, ga_chr_t pred_chr);


/**
 * Evaluates predictor fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_predictor(ga_chr_t chr);


/**
 * Calculates fitness using the PSNR (peak signal-to-noise ratio) function.
 * The higher the value, the better the filter.
 *
 * @param  original image
 * @param  filtered image
 * @return fitness value (PSNR)
 */
ga_fitness_t fitness_psnr(img_image_t original, img_image_t filtered);


/**
 * Computes real PSNR value from fitness value
 */
static inline double fitness_to_psnr(ga_fitness_t f) {
    return 10 * log10(f);
}
