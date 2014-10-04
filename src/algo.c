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


#include "algo.h"
#include "logging.h"


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
    FILE *log_file)
{
    while (cgp_population->generation < config->max_generations) {

        VERBOSELOG("One iteration of CGP.");

        // create children and evaluate new generation
        ga_next_generation(cgp_population);

        // log progress
        if ((cgp_population->generation % config->log_interval) == 0) {
            log_cgp_progress(stdout, cgp_population);
            log_cgp_progress(log_file, cgp_population);
        }

        int signal = check_signals(cgp_population->generation);
        bool store_now = (cgp_population->generation % config->vault_interval) == 0;

        // store to vault
        if (signal || store_now) {
            print_results(cgp_population, NULL, NULL);
            save_filtered_image(config->log_dir, cgp_population, img_noisy);
            if (config->vault_enabled) {
                vault_store(vault, cgp_population);
            }
            SLOWLOG("Current output image and state stored.");
        }

        if (signal > 0) return signal;
    }

    return 0;
}



/**
 * Coevolutionary CGP main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  cgp_archive CGP archive
 * @param  pred_archive Predictors archive
 * @param  cgp_current_best Pointer to shared variable holding best CGP fitness found
 * @param  pred_current_best Pointer to shared variable holding best predictor fitness found
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
    bool *finished)
{
    while (!(*finished)) {
        VERBOSELOG("One iteration of CGP.");

        // create children and evaluate new generation
        ga_create_children(cgp_population);

        #pragma omp critical (PRED_ARCHIVE)
        {
            ga_evaluate_pop(cgp_population);
        }

        // log progress
        if ((cgp_population->generation % config->log_interval) == 0) {
            log_cgp_progress(stdout, cgp_population);
            log_cgp_progress(progress_log_file, cgp_population);
        }

        if (ga_is_better(cgp_population->problem_type, cgp_population->best_fitness, *cgp_current_best)) {
            // log
            if ((cgp_population->generation % config->log_interval) != 0) {
                log_cgp_progress(stdout, cgp_population);
                log_cgp_progress(progress_log_file, cgp_population);
            }
            log_cgp_change(stdout, *cgp_current_best, cgp_population->best_fitness);
            log_cgp_change(progress_log_file, *cgp_current_best, cgp_population->best_fitness);

            // store and recalculate predictors fitness
            ga_chr_t archived;
            #pragma omp critical (CGP_ARCHIVE)
            {
                archived = arc_insert(cgp_archive, cgp_population->best_chromosome);
                ga_invalidate_fitness(pred_population);
                ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));
            }

            log_cgp_archived(stdout, cgp_population->best_fitness, archived->fitness);
            log_cgp_archived(progress_log_file, cgp_population->best_fitness, archived->fitness);
            *cgp_current_best = cgp_population->best_fitness;

            // drop fitness values
            ga_invalidate_fitness(pred_population);
            ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));

        }

        int signal = check_signals(cgp_population->generation);
        bool store_now = (cgp_population->generation % config->vault_interval) == 0;

        // last generation?
        if (cgp_population->generation >= config->max_generations) {
            SLOWLOG("Generations limit reached (%d), terminating.", config->max_generations);
            *finished = true;
        }

        // store to vault
        if (*finished || signal || store_now) {
            // dump current results to console
            print_results(cgp_population, pred_population, pred_archive);

            // save image
            save_filtered_image(config->log_dir, cgp_population, img_noisy);

            // flush log file
            fflush(progress_log_file);

            // store to vault
            if (config->vault_enabled) {
                // freeze predictors archive during save
                #pragma omp critical (PRED_ARCHIVE)
                {
                    vault_store(vault, cgp_population);
                }
            }
            SLOWLOG("Current output image and state stored.");
        }

        if (signal > 0) {
            // stop other threads
            *finished = true;
            return signal;
        }
    }

    return 0;
}


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
    bool *finished)
{
    while (!(*finished)) {
        VERBOSELOG("One iteration of predictors.");

        ga_create_children(pred_population);
        #pragma omp critical (CGP_ARCHIVE)
        {
            ga_evaluate_pop(pred_population);
        }

        // log progress
        if ((pred_population->generation % config->log_interval) == 0) {
            log_pred_progress(stdout, pred_population, pred_archive, true);
            log_pred_progress(progress_log_file, pred_population, pred_archive, false);
        }

        // copy to archive
        if (ga_is_better(pred_population->problem_type, pred_population->best_fitness, arc_get(pred_archive, 0)->fitness)) {
            // log
            if ((pred_population->generation % config->log_interval) != 0) {
                log_pred_progress(stdout, pred_population, pred_archive, true);
                log_pred_progress(progress_log_file, pred_population, pred_archive, false);
            }
            log_pred_change(stdout, arc_get(pred_archive, 0)->fitness, pred_population->best_fitness, true);
            log_pred_change(progress_log_file, arc_get(pred_archive, 0)->fitness, pred_population->best_fitness, false);

            // store and invalidate CGP fitness
            #pragma omp critical (PRED_ARCHIVE)
            {
                arc_insert(pred_archive, pred_population->best_chromosome);
                ga_invalidate_fitness(cgp_population);
            }
        }
    }
}
