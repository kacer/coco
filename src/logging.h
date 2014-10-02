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

#include "debug.h"

#define LOG(...) { LOG_THREAD_IDENT(stdout); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); }
#define SLOWLOG(...) { LOG_THREAD_IDENT(stdout); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); }


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
 * Logs that CGP best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_cgp_change(ga_fitness_t previous_best, ga_fitness_t new_best);


/**
 * Logs that predictors best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_pred_change(ga_fitness_t previous_best, ga_fitness_t new_best);


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
