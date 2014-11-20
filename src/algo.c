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
 * @param  wd (= work_data)
 * @return Program return value
 */
int cgp_main(algo_data_t *wd)
{
    ga_fitness_t cgp_parent_fitness;

    // append first entry to CSV
    _log_cgp_csv(wd->config->algorithm, bw_get(&wd->history, -1),
        wd->cgp_population, wd->cgp_archive, wd->pred_archive, wd->history_file);

    // do the work
    while (!(wd->finished)) {

        #pragma omp critical (PRED_ARCHIVE__CGP_POP)
        {
            cgp_parent_fitness = wd->cgp_population->best_fitness;
            // create children and evaluate new generation
            ga_next_generation(wd->cgp_population);
        }

        // last generation?
        if (wd->cgp_population->generation >= wd->config->max_generations) {
            printf("Generations limit reached (%d), terminating.\n", wd->config->max_generations);
            wd->finished = true;
        }

        // target fitness achieved?
        if (wd->config->target_fitness != 0 && wd->cgp_population->best_fitness >= wd->config->target_fitness) {
            printf("Target fitness reached (" FITNESS_FMT "), terminating.\n", wd->config->target_fitness);
            wd->finished = true;
        }

        // check signals
        int signal = check_signals(wd->cgp_population->generation);
        if (signal > 0) {
            // stop other threads
            wd->finished = true;
        }

        // whether we found better solution
        bool is_better = ga_is_better(wd->cgp_population->problem_type, wd->cgp_population->best_fitness, cgp_parent_fitness);

        // whether we should log now
        bool log_now = wd->config->log_interval && ((wd->cgp_population->generation % wd->config->log_interval) == 0);

        // whether new entry was appended to history log
        bw_history_entry_t last_history_entry;

        // whether we update evolution params now
        bool apply_baldwin = false;
        if (wd->config->algorithm == baldwin) {
            if (is_better) {
                apply_baldwin = true;

            } else if (wd->config->bw_interval) {
                int diff = wd->cgp_population->generation - wd->baldwin_state.last_applied_generation;
                if (diff >= wd->config->bw_interval) {
                    apply_baldwin = true;
                }
            }
        }

        // store predicted and real fitness
        ga_fitness_t predicted_fitness = wd->cgp_population->best_fitness;
        ga_fitness_t real_fitness = 0;

        // update archive if necessary
        if (is_better) {
            DOUBLE_LOG(log_cgp_change, wd->log_file, cgp_parent_fitness, wd->cgp_population->best_fitness);

            if (wd->config->algorithm != simple_cgp) {
                // store and recalculate predictors fitness
                ga_chr_t archived;
                #pragma omp critical (CGP_ARCHIVE__PRED_POP)
                {
                    archived = arc_insert(wd->cgp_archive, wd->cgp_population->best_chromosome);
                    ga_reevaluate_pop(wd->pred_population);
                    #pragma omp critical (PRED_ARCHIVE__CGP_POP)
                    {
                        ga_reevaluate_chr(wd->pred_population, arc_get(wd->pred_archive, 0));
                    }
                }

                predicted_fitness = wd->cgp_population->best_fitness;
                real_fitness = archived->fitness;
                //printf("Inaccuracy %.10g\n", predicted_fitness / real_fitness);

                DOUBLE_LOG(log_cgp_archived, wd->log_file, predicted_fitness, real_fitness);
                logger_fire(&wd->loggers, better_cgp, predicted_fitness, real_fitness);

            } else {
                // in simple CGP, predicted fitness is in fact the real fitness
                logger_fire(&wd->loggers, better_cgp, -1, predicted_fitness);
            }
        }

        if (is_better || apply_baldwin || log_now || wd->finished) {
            // update history log
            bw_history_entry_t *prev_entry = bw_get(&wd->history, -1);

            if (wd->config->algorithm == simple_cgp) {
                bw_calc_history(
                    &last_history_entry,
                    prev_entry,
                    wd->cgp_population->generation,
                    wd->cgp_population->best_chromosome->fitness,
                    -1,
                    -1
                );

            } else {
                bw_calc_history(
                    &last_history_entry,
                    prev_entry,
                    wd->cgp_population->generation,
                    arc_get(wd->cgp_archive, -1)->fitness,
                    arc_get_original_fitness(wd->cgp_archive, -1),
                    arc_get(wd->pred_archive, 0)->fitness
                );
            }

            if (is_better || apply_baldwin) {
                bw_add_history_entry(&wd->history, &last_history_entry);
                DOUBLE_LOG(log_bw_history_entry, wd->log_file, &last_history_entry);

                //bw_dump_history_asciiart(stdout, &wd->history);
            }
        }

        if (apply_baldwin) {
            // Baldwin mode: change evolution params.
            // Everything is done in predictors thread asynchronously-
            // Enter critical section only if it seems that it is not set.
            if (!wd->baldwin_state.apply_now) {
                #pragma omp critical (BALDWIN_STATE)
                {
                    wd->baldwin_state.apply_now = true;
                }
            }
        }

        // append next entry to CSV
        // if apply_baldwin == 1, CSV entry is written after update by pred thread
        if (is_better || log_now || wd->finished) {
            DOUBLE_LOG(log_cgp_progress, wd->log_file, wd->cgp_population, fitness_get_cgp_evals());

            // lock for best circuit in archive so it won't be overwritten
            // in the middle of the dump
            #pragma omp critical (CGP_ARCHIVE__PRED_POP)
            {
                #pragma omp critical (PRED_ARCHIVE__CGP_POP)
                {
                    _log_cgp_csv(wd->config->algorithm,
                        &last_history_entry, wd->cgp_population,
                        wd->cgp_archive, wd->pred_archive, wd->history_file);
                }
            }

            fflush(wd->log_file);
            fflush(wd->history_file);
        }

        if (wd->finished && signal >= 0) {
            #pragma omp critical (PRED_ARCHIVE__CGP_POP)
            {
                DOUBLE_LOG(log_cgp_finished, wd->log_file, wd->cgp_population);
            }
        }

        if (signal > 0) {
            return signal;
        }
    }

    return 0;
}


/**
 * Coevolutionary predictors main loop
 * @param  wd (= work_data)
 */
void pred_main(algo_data_t *wd)
{
    while (!(wd->finished)) {

        #pragma omp critical (CGP_ARCHIVE__PRED_POP)
        {
            ga_next_generation(wd->pred_population);
        }

        // If evolution params should be changed now, do it
        // This insane nesting is a small optimization - if state is false,
        // nothing happens, if true, aquire lock and check again, just to be sure
        // However, apply_now is never set to false outside this function, so it
        // is not really necessary to check again.
        if (wd->baldwin_state.apply_now) {
            #pragma omp critical (BALDWIN_STATE)
            {
                if (wd->baldwin_state.apply_now) {
                    bw_update_t result;

                    // update evolution params
                    bw_update_params(&wd->config->bw_config, &wd->history, &result);

                    if (result.predictor_length_changed) {
                        DOUBLE_LOG(log_predictors_length_change, wd->log_file,
                            result.old_predictor_length, result.new_predictor_length);

                        // recalculate predictors' phenotypes
                        pred_pop_calculate_phenotype(wd->pred_population);
                        #pragma omp critical (PRED_ARCHIVE__CGP_POP)
                        {
                            pred_calculate_phenotype(arc_get(wd->pred_archive, 0)->genome);
                        }

                        // reevaluate predictors & write new row to CSV
                        #pragma omp critical (CGP_ARCHIVE__PRED_POP)
                        {
                            ga_reevaluate_pop(wd->pred_population);
                            #pragma omp critical (PRED_ARCHIVE__CGP_POP)
                            {
                                ga_reevaluate_chr(wd->pred_population, arc_get(wd->pred_archive, 0));
                            }

                            _log_cgp_csv(wd->config->algorithm,
                                bw_get(&wd->history, -1), wd->cgp_population,
                                wd->cgp_archive, wd->pred_archive, wd->history_file);
                        }
                    }

                    // no lock on CGP population here, it should not matter
                    wd->baldwin_state.last_applied_generation = wd->cgp_population->generation;
                    wd->baldwin_state.apply_now = false;
                }
            }
        }

       // log progress
       bool is_better = ga_is_better(wd->pred_population->problem_type, wd->pred_population->best_fitness, arc_get(wd->pred_archive, 0)->fitness);
       bool log_interval = wd->config->log_interval && (wd->pred_population->generation % wd->config->log_interval) == 0;

       // log progress
       if (is_better || log_interval) {
           DOUBLE_LOG_PRED(log_pred_progress, wd->log_file, wd->pred_population, wd->pred_archive);
       }

        // update archive if necessary
        if (is_better) {
            // log
            DOUBLE_LOG_PRED(
                log_pred_change,
                wd->log_file,
                arc_get(wd->pred_archive, 0)->fitness,
                wd->pred_population->best_fitness,
                (pred_genome_t) wd->pred_population->best_chromosome->genome);

            // store and invalidate CGP fitness
            #pragma omp critical (PRED_ARCHIVE__CGP_POP)
            {
                arc_insert(wd->pred_archive, wd->pred_population->best_chromosome);
                ga_reevaluate_pop(wd->cgp_population);
            }
        }
    }
}
