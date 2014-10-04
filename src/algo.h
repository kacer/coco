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
#include "vault.h"
#include "image.h"
#include "config.h"
#include "archive.h"
#include "predictors.h"


/**
 * Checks for SIGXCPU and SIGINT signals
 *
 * Defined in main.c
 *
 * @return Received signal code
 */
extern int check_signals(int current_generation);


/**
 * Simple CGP (no coevolution) main loop
 * @param  cgp_population
 * @param  config
 * @param  vault
 * @param  img_noisy Noisy image (to store filtered img to results)
 * @return Program return value
 */
int simple_cgp_main(
    ga_pop_t cgp_population,
    config_t *config,
    vault_storage_t *vault,
    img_image_t img_noisy,
    FILE *log_file
);


/**
 * Coevolutionary or simple CGP main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  cgp_archive CGP archive
 * @param  pred_archive Predictors archive
 * @param  cgp_current_best Pointer to shared variable holding best CGP fitness found
 * @param  config
 * @param  vault
 * @param  img_noisy Noisy image (to store filtered img to results)
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 * @return Program return value
 */
int coev_cgp_main(
    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archives
    archive_t cgp_archive,
    archive_t pred_archive,

    // best found fitness value
    ga_fitness_t *cgp_current_best,

    // config
    config_t *config,
    vault_storage_t *vault,

    // input
    img_image_t img_noisy,

    // log files
    FILE *best_circuit_file,
    FILE *progress_log_file,

    // status
    bool *finished
);


/**
 * Coevolutionary predictors main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  pred_archive Predictors archive
 * @param  config
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 */
void coev_pred_main(
    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archive
    archive_t pred_archive,

    // config
    config_t *config,

    // log
    FILE *progress_log_file,

    // status
    bool *finished
);
