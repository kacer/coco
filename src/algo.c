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
#include "fitness.h"

/*
static inline void _log_cgp_csv(
    algorithm_t algorithm,
    history_entry_t *last_entry,
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
*/

bool _should_apply_baldwin(bool is_better, algo_data_t *wd)
{
    if (wd->config->algorithm == baldwin) {
        int diff = (wd->cgp_population->generation
                    - wd->baldwin_state.last_applied_generation);

        if (is_better || (wd->config->bw_interval && diff >= wd->config->bw_interval)) {
            return true;
        }
    }

    return false;
}


/**
 * CGP main loop
 * @param  wd (= work_data)
 * @return Program return value
 */
int cgp_main(algo_data_t *wd)
{
    /* A. log start */
    logger_fire(&wd->loggers, started, history_last(&wd->history));

    /* B. "Infinite" loop */
    while (!(wd->finished)) {
        ga_fitness_t cgp_parent_fitness;
        ga_fitness_t predicted_fitness;
        ga_fitness_t real_fitness = 0;

        history_entry_t current_history_entry;
        finish_reason_t finish_reason;


        /* advance to next generation *****************************************/


        #pragma omp critical (PRED_ARCHIVE__CGP_POP)
        {
            cgp_parent_fitness = wd->cgp_population->best_fitness;
            // create children and evaluate new generation
            ga_next_generation(wd->cgp_population);
        }


        /* check stop conditions **********************************************/


        int received_signal = check_signals(wd->cgp_population->generation);

        // last generation?
        if (wd->cgp_population->generation >= wd->config->max_generations) {
            finish_reason = generation_limit;
            wd->finished = true;
        }

        // target fitness achieved?
        if (wd->config->target_fitness != 0
            && wd->cgp_population->best_fitness >= wd->config->target_fitness)
        {
            finish_reason = target_fitness;
            wd->finished = true;
        }

        // signal received
        if (received_signal > 0) {
            // stop other threads
            finish_reason = received_signal;
            wd->finished = true;
        }


        /* various checks******************************************************/


        // whether we found better solution
        bool is_better = ga_is_better(wd->cgp_population->problem_type,
            wd->cgp_population->best_fitness, cgp_parent_fitness);

        // whether we should log now
        bool log_tick_now = wd->config->log_interval
            && ((wd->cgp_population->generation % wd->config->log_interval) == 0);

        // whether we update evolution params now
        bool apply_baldwin_now = _should_apply_baldwin(is_better, wd);

        // whether we should append entry to history log
        bool need_history_entry_append = is_better || apply_baldwin_now;

        // whether we need to calculate current state for any reason
        bool need_history_entry_calc =
            need_history_entry_append || log_tick_now || received_signal
            || wd->finished;


        /* update archive, calculate real fitness if necessary ****************/


        if (wd->config->algorithm == simple_cgp) {
            predicted_fitness = -1;
            real_fitness = wd->cgp_population->best_fitness;

        } else {
            /* coevolution */
            predicted_fitness = wd->cgp_population->best_fitness;

            if (is_better) {
                // store and recalculate predictors fitness
                ga_chr_t archived;
                #pragma omp critical (CGP_ARCHIVE__PRED_POP)
                {
                    archived = arc_insert(wd->cgp_archive,
                        wd->cgp_population->best_chromosome);
                    real_fitness = archived->fitness;
                    ga_reevaluate_pop(wd->pred_population);
                    #pragma omp critical (PRED_ARCHIVE__CGP_POP)
                    {
                        ga_reevaluate_chr(wd->pred_population,
                            arc_get(wd->pred_archive, 0));
                    }
                }

            } else if (need_history_entry_calc) {
                real_fitness = fitness_eval_cgp(wd->cgp_population->best_chromosome);
            }
        }


        /* change evolution params in baldwin mode ****************************/


        if (apply_baldwin_now) {
            // everything is done in predictors thread asynchronously
            // no need for critical section here
            wd->baldwin_state.apply_now = true;
        }


        /* calculate and append current history entry *************************/


        if (need_history_entry_calc) {
            ga_fitness_t active_predictor_fitness = -1;
            int pred_length = -1;
            int pred_used_length = -1;

            if (wd->config->algorithm != simple_cgp) {
                #pragma omp critical (PRED_ARCHIVE__CGP_POP)
                {
                    ga_chr_t predictor = arc_get(wd->pred_archive, 0);
                    pred_used_length = ((pred_genome_t) predictor->genome)->used_pixels,
                    active_predictor_fitness = predictor->fitness;
                }

                pred_length = pred_get_length();
            }

            history_calc_entry(
                &current_history_entry,
                history_last(&wd->history),
                wd->cgp_population->generation,
                real_fitness,
                predicted_fitness,
                active_predictor_fitness,
                fitness_get_cgp_evals(),
                pred_length,
                pred_used_length
            );
        }

        if (need_history_entry_append) {
            history_append_entry(&wd->history, &current_history_entry);
        }


        /* fire log events ****************************************************/


        if (is_better) {
            logger_fire(&wd->loggers, better_cgp, &current_history_entry);
        } else if (log_tick_now) {
            logger_fire(&wd->loggers, log_tick, &current_history_entry);
        }

        if (apply_baldwin_now) {
            logger_fire(&wd->loggers, baldwin_triggered, &current_history_entry);
        }

        if (received_signal) {
            logger_fire(&wd->loggers, signal, abs(received_signal), &current_history_entry);
        }

        if (wd->finished) {
            logger_fire(&wd->loggers, finished, finish_reason, &current_history_entry);
        }


        /* return signal code, if terminated by signal ************************/


        if (received_signal > 0) {
            return received_signal;
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

        // if evolution params should be changed now, do it
        // no lock necessary
        if (wd->baldwin_state.apply_now) {
            bw_update_t result;
            int generation = wd->cgp_population->generation;

            // update evolution params
            bw_update_params(&wd->config->bw_config, &wd->history, &result);

            if (result.predictor_length_changed) {
                logger_fire(&wd->loggers, pred_length_changed,
                    generation,
                    result.old_predictor_length,
                    result.new_predictor_length);

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
                        ga_reevaluate_chr(wd->pred_population,
                            arc_get(wd->pred_archive, 0));
                    }
                }
            }

            // no lock on CGP population here, it should not matter
            wd->baldwin_state.last_applied_generation = generation;
            wd->baldwin_state.apply_now = false;
        }

        bool is_better = ga_is_better(wd->pred_population->problem_type,
            wd->pred_population->best_fitness,
            arc_get(wd->pred_archive, 0)->fitness);

        // update archive if necessary
        if (is_better) {

            logger_fire(&wd->loggers,
                better_pred,
                arc_get(wd->pred_archive, 0)->fitness,
                wd->pred_population->best_fitness
            );

            // store and invalidate CGP fitness
            #pragma omp critical (PRED_ARCHIVE__CGP_POP)
            {
                arc_insert(wd->pred_archive, wd->pred_population->best_chromosome);
                ga_reevaluate_pop(wd->cgp_population);
            }
        }
    }
}
