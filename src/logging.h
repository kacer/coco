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
#include "files.h"
#include "config.h"
#include "archive.h"
#include "fitness.h"
#include "predictors.h"


#define FITNESS_FMT "%.10g"

#define SECTION_CGP "cgp"
#define SECTION_PRED "prd"
#define SECTION_SYS "sys"

/* predictors log to second column */
#define PRED_INDENT "                                                        "


/* standard console outputing */

#include "debug.h"

#define LOG_F(fp, ...) { LOG_THREAD_IDENT((fp)); fprintf((fp), __VA_ARGS__); fprintf((fp), "\n"); }
#define SLOWLOG_F(fp, ...) { LOG_THREAD_IDENT((fp)); fprintf((fp), __VA_ARGS__); fprintf((fp), "\n"); }

#define LOG(...) LOG_F(stdout, __VA_ARGS__)
#define SLOWLOG(...) SLOWLOG_F(stdout, __VA_ARGS__)


/**
 * Creates log directories
 * @param  dir
 * @param  vault_dir If not NULL, stores vault directory here
 * @param  vault_dir_buffer_size
 */
int log_create_dirs(const char *dir, const char *vault_dir, const int vault_dir_buffer_size);


/**
 * Print current CGP progress
 * @param  cgp_population
 */
void log_cgp_progress(FILE *fp, ga_pop_t cgp_population);


/**
 * Print current predictors progress
 * @param  pred_population
 * @param  pred_archive
 */
void log_pred_progress(FILE *fp, ga_pop_t pred_population,
    archive_t pred_archive, bool indent);


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
void log_cgp_change(FILE *fp, ga_fitness_t previous_best, ga_fitness_t new_best);


/**
 * Logs that CGP was moved to archive
 * @param predicted
 * @param real
 */
void log_cgp_archived(FILE *fp, ga_fitness_t predicted, ga_fitness_t real);


/**
 * Logs that predictors best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_pred_change(FILE *fp, ga_fitness_t previous_best, ga_fitness_t new_best, bool indent);


/**
 * Saves original image to results directory
 * @param dir results directory
 * @param original
 */
void save_original_image(const char *dir, img_image_t original);


/**
 * Saves input noisy image to results directory
 * @param dir results directory
 * @param noisy
 */
void save_noisy_image(const char *dir, img_image_t noisy);


/**
 * Saves image filtered by best filter to results directory
 * @param dir results directory
 * @param cgp_population
 * @param noisy
 */
void save_filtered_image(const char *dir, ga_pop_t cgp_population, img_image_t noisy);


/**
 * Saves configuration to results directory
 */
void save_config(const char *dir, config_t *config);


/**
 * Open specified file for writing. Caller is responsible for closing.
 * @param  dir
 * @param  file
 * @return
 */
FILE *open_log_file(const char *dir, const char *file);
