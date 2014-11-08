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


#define DOUBLE_LOG(func, file, ...) { \
    func(stdout, __VA_ARGS__); \
    func(file, __VA_ARGS__); \
}


#define DOUBLE_LOG_PRED(func, file, ...) { \
    func(stdout, __VA_ARGS__, false); \
    func(file, __VA_ARGS__, false); \
}


static inline void _log_cgp_csv(
    algorithm_t algorithm,
    bw_history_entry_t *last_entry,
    ga_pop_t cgp_population,
    archive_t cgp_archive,
    archive_t pred_archive,
    FILE *history_file)
{
    if (algorithm == simple_cgp) {
        log_cgp_history(
            history_file,
            last_entry,
            fitness_get_cgp_evals(),
            -1,
            -1,
            cgp_population->best_chromosome->fitness
        );

    } else {
        log_cgp_history(
            history_file,
            last_entry,
            fitness_get_cgp_evals(),
            pred_get_length(),
            ((pred_genome_t) arc_get(pred_archive, 0)->genome)->used_pixels,
            cgp_archive->best_chromosome_ever->fitness
        );
    }
}


/**
 * CGP main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  cgp_archive CGP archive
 * @param  pred_archive Predictors archive
 * @param  config
 * @param  img_noisy Noisy image (to store filtered img to results)
 * @param  baldwin_state Colearning state and sync info
 * @param  best_circuit_file_name_txt File for storing best circuit in readable format
 * @param  best_circuit_file_name_chr File for storing best circuit in CGPViewer format
 * @param  log_file Genral log file
 * @param  history_file History CSV file
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 * @return Program return value
 */
int cgp_main(
    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archives
    archive_t cgp_archive,
    archive_t pred_archive,

    // config
    config_t *config,

    // input
    img_image_t img_noisy,

    // baldwin
    bw_state_t *baldwin_state,

    // log files
    char *best_circuit_file_name_txt,
    char *best_circuit_file_name_chr,
    FILE *log_file,
    FILE *history_file,

    // status
    bool *finished)
{
    ga_fitness_t cgp_parent_fitness;

    // append first entry to CSV
    _log_cgp_csv(config->algorithm, bw_get(&baldwin_state->history, -1),
        cgp_population,cgp_archive, pred_archive, history_file);

    // do the work
    while (!(*finished)) {

        #pragma omp critical (PRED_ARCHIVE__CGP_POP)
        {
            if (config->algorithm != simple_cgp && pred_archive->modified) {
                ga_reevaluate_pop(cgp_population);
                pred_archive->modified = false;
            }
            cgp_parent_fitness = cgp_population->best_fitness;
            ga_next_generation(cgp_population);
        }

        // last generation?
        if (cgp_population->generation >= config->max_generations) {
            printf("Generations limit reached (%d), terminating.\n", config->max_generations);
            *finished = true;
        }

        // target fitness achieved?
        if (config->target_fitness != 0 && cgp_population->best_fitness >= config->target_fitness) {
            printf("Target fitness reached (" FITNESS_FMT "), terminating.\n", config->target_fitness);
            *finished = true;
        }

        // check signals
        int signal = check_signals(cgp_population->generation);
        if (signal > 0) {
            // stop other threads
            *finished = true;
        }

        // whether we found better solution
        bool is_better = ga_is_better(cgp_population->problem_type, cgp_population->best_fitness, cgp_parent_fitness);

        // whether we should log now
        bool log_now = config->log_interval && ((cgp_population->generation % config->log_interval) == 0);

        // whether new entry was appended to history log
        bw_history_entry_t last_history_entry;

        // whether we update evolution params now
        bool apply_baldwin = false;
        if (config->algorithm == baldwin) {
            if (is_better) {
                apply_baldwin = true;

            } else if (config->bw_interval) {
                int diff = cgp_population->generation - baldwin_state->last_applied_generation;
                if (diff >= config->bw_interval) {
                    apply_baldwin = true;
                }
            }
        }

        // store predicted and real fitness
        ga_fitness_t predicted_fitness = cgp_population->best_fitness;
        ga_fitness_t real_fitness = 0;

        // update archive if necessary
        if (is_better) {
            DOUBLE_LOG(log_cgp_change, log_file, cgp_parent_fitness, cgp_population->best_fitness);

            if (config->algorithm != simple_cgp) {
                // store and recalculate predictors fitness
                ga_chr_t archived;
                #pragma omp critical (CGP_ARCHIVE__PRED_POP)
                {
                    archived = arc_insert(cgp_archive, cgp_population->best_chromosome);
                }

                predicted_fitness = cgp_population->best_fitness;
                real_fitness = archived->fitness;
                //printf("Inaccuracy %.10g\n", predicted_fitness / real_fitness);

                DOUBLE_LOG(log_cgp_archived, log_file, predicted_fitness, real_fitness);
            }
        }

        if (is_better || apply_baldwin || log_now || *finished) {
            // update history log
            bw_history_entry_t *prev_entry = bw_get(&baldwin_state->history, -1);

            if (config->algorithm == simple_cgp) {
                bw_calc_history(
                    &last_history_entry,
                    prev_entry,
                    cgp_population->generation,
                    cgp_population->best_chromosome->fitness,
                    -1,
                    -1
                );

            } else {
                bw_calc_history(
                    &last_history_entry,
                    prev_entry,
                    cgp_population->generation,
                    arc_get(cgp_archive, -1)->fitness,
                    arc_get_original_fitness(cgp_archive, -1),
                    arc_get(pred_archive, 0)->fitness
                );
            }

            if (is_better || apply_baldwin) {
                bw_add_history_entry(&baldwin_state->history, &last_history_entry);
                DOUBLE_LOG(log_bw_history_entry, log_file, &last_history_entry);

                //bw_dump_history_asciiart(stdout, &baldwin_state->history);
            }
        }

        if (apply_baldwin) {
            // Baldwin mode: change evolution params.
            // Everything is done in predictors thread asynchronously-
            // Enter critical section only if it seems that it is not set.
            if (!baldwin_state->apply_now) {
                #pragma omp critical (BALDWIN_STATE)
                {
                    baldwin_state->apply_now = true;
                }
            }
        }

        // append next entry to CSV
        // if apply_baldwin == 1, CSV entry is written after update by pred thread
        if (is_better || log_now || *finished) {
            DOUBLE_LOG(log_cgp_progress, log_file, cgp_population, fitness_get_cgp_evals());

            // lock for best circuit in archive so it won't be overwritten
            // in the middle of the dump
            #pragma omp critical (CGP_ARCHIVE__PRED_POP)
            {
                _log_cgp_csv(config->algorithm,
                    &last_history_entry, cgp_population,
                    cgp_archive, pred_archive, history_file);
            }
        }

        if (*finished && signal >= 0) {
            DOUBLE_LOG(log_cgp_finished, log_file, cgp_population);
        }

        if (signal > 0) {
            return signal;
        }
    }

    return 0;
}


/**
 * Coevolutionary predictors main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  cgp_archive CGP archive
 * @param  pred_archive Predictors archive
 * @param  config
 * @param  baldwin_state Colearning state and sync info
 * @param  log_file General log file
 * @param  history_file History CSV file
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 */
void pred_main(
    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archives
    archive_t cgp_archive,
    archive_t pred_archive,

    // config
    config_t *config,

    // baldwin_state
    bw_state_t *baldwin_state,

    // log files
    FILE *log_file,
    FILE *history_file,

    // status
    bool *finished)
{
    ga_fitness_t pred_parent_fitness;


    while (!(*finished)) {

        #pragma omp critical (CGP_ARCHIVE__PRED_POP)
        {
            if (cgp_archive->modified) {
                ga_reevaluate_pop(pred_population);
                ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));
                cgp_archive->modified = false;
            }
            pred_parent_fitness = arc_get(pred_archive, 0)->fitness;
            ga_next_generation(pred_population);
        }

        // If evolution params should be changed now, do it
        // This insane nesting is a small optimization - if state is false,
        // nothing happens, if true, aquire lock and check again, just to be sure
        // However, apply_now is never set to false outside this function, so it
        // is not really necessary to check again.
        /*
        if (config->algorithm == baldwin && baldwin_state->apply_now) {
            #pragma omp critical (BALDWIN_STATE)
            {
                if (baldwin_state->apply_now) {
                    bw_update_t result;

                    // update evolution params
                    bw_update_params(&config->bw_config, &baldwin_state->history, &result);

                    if (result.predictor_length_changed) {
                        DOUBLE_LOG(log_predictors_length_change, log_file,
                            result.old_predictor_length, result.new_predictor_length);

                        // recalculate predictors' phenotypes
                        pred_pop_calculate_phenotype(pred_population);
                        #pragma omp critical (PRED_ARCHIVE__CGP_POP)
                        {
                            pred_calculate_phenotype(arc_get(pred_archive, 0)->genome);
                        }

                        // reevaluate predictors & write new row to CSV
                        #pragma omp critical (CGP_ARCHIVE__PRED_POP)
                        {
                            ga_reevaluate_pop(pred_population);
                            #pragma omp critical (PRED_ARCHIVE__CGP_POP)
                            {
                                ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));
                            }

                            _log_cgp_csv(config->algorithm,
                                bw_get(&baldwin_state->history, -1), cgp_population,
                                cgp_archive, pred_archive, history_file);
                        }
                    }

                    // no lock on CGP population here, it should not matter
                    baldwin_state->last_applied_generation = cgp_population->generation;
                    baldwin_state->apply_now = false;
                }
            }
        }
        */

       // log progress
       bool is_better = ga_is_better(pred_population->problem_type, pred_population->best_fitness, pred_parent_fitness);
       bool log_interval = config->log_interval && (pred_population->generation % config->log_interval) == 0;

       // log progress
       if (is_better || log_interval) {
           DOUBLE_LOG_PRED(log_pred_progress, log_file, pred_population, pred_archive);
       }

        // update archive if necessary
        if (is_better) {
            // log
            DOUBLE_LOG_PRED(
                log_pred_change,
                log_file,
                arc_get(pred_archive, 0)->fitness,
                pred_population->best_fitness,
                (pred_genome_t) pred_population->best_chromosome->genome);

            // store and invalidate CGP fitness
            #pragma omp critical (PRED_ARCHIVE__CGP_POP)
            {
                arc_insert(pred_archive, pred_population->best_chromosome);
            }
        }
    }
}
