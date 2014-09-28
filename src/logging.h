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

#include <stdio.h>

#include "cgp.h"
#include "image.h"
#include "archive.h"
#include "fitness.h"
#include "predictors.h"


/* standard console outputing */

#ifdef DEBUG
    #define DEBUGLOG(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
#else
    #define DEBUGLOG(...)
#endif

#define LOG(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
#define SLOWLOG(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }


/**
 * Print current CGP progress
 * @param  cgp_population
 */
void print_cgp_progress(ga_pop_t cgp_population);


/**
 * Print current predictors progress
 * @param  pred_population
 * @param  pred_archive
 */
void print_pred_progress(ga_pop_t pred_population,
    archive_t pred_archive);


/**
 * Print current progress
 * @param  cgp_population
 * @param  pred_population
 * @param  pred_archive
 */
void print_progress(ga_pop_t cgp_population, ga_pop_t pred_population,
    archive_t pred_archive);


/**
 * Print current progress and best found circuit
 * @param cgp_population
 * @param pred_population
 * @param pred_archive
 */
void print_results(ga_pop_t cgp_population, ga_pop_t pred_population,
    archive_t pred_archive);


/**
 * Saves original image to results directory
 * @param dir results directory
 * @param original
 */
void save_original_image(char *dir, img_image_t original);


/**
 * Saves input noisy image to results directory
 * @param dir results directory
 * @param noisy
 */
void save_noisy_image(char *dir, img_image_t noisy);


/**
 * Saves image filtered by best filter to results directory
 * @param dir results directory
 * @param cgp_population
 * @param noisy
 */
void save_filtered_image(char *dir, ga_pop_t cgp_population, img_image_t noisy);
