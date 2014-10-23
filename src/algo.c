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
#include "baldwin.h"
#include "logging.h"


#define DOUBLE_LOG(func, file, ...) { \
    func(stdout, __VA_ARGS__); \
    func(file, __VA_ARGS__); \
}


#define DOUBLE_LOG_PRED(func, file, ...) { \
    func(stdout, __VA_ARGS__, true); \
    func(file, __VA_ARGS__, false); \
}


static inline void _log_cgp(ga_pop_t cgp_population,
    char *best_circuit_file_name_txt,
    char *best_circuit_file_name_chr,
    FILE *log_file)
{
    DOUBLE_LOG(log_cgp_progress, log_file, cgp_population, fitness_get_cgp_evals());

    FILE *circuit_file_txt = fopen(best_circuit_file_name_txt, "w");
    if (circuit_file_txt) {
        log_cgp_circuit(circuit_file_txt, cgp_population);
        fclose(circuit_file_txt);
    } else {
        fprintf(stderr, "Failed to open %s!\n", best_circuit_file_name_txt);
    }

    FILE *circuit_file_chr = fopen(best_circuit_file_name_chr, "w");
    if (circuit_file_chr) {
        cgp_dump_chr_compat(cgp_population->best_chromosome, circuit_file_chr);
        fclose(circuit_file_chr);
    } else {
        fprintf(stderr, "Failed to open %s!\n", best_circuit_file_name_chr);
    }
}



/**
 * CGP main loop
 * @param  mode
 * @param  cgp_population
 * @param  pred_population
 * @param  cgp_archive CGP archive
 * @param  pred_archive Predictors archive
 * @param  config
 * @param  vault
 * @param  img_noisy Noisy image (to store filtered img to results)
 * @param  best_circuit_file_name_txt File for storing best circuit in readable format
 * @param  best_circuit_file_name_chr File for storing best circuit in CGPViewer format
 * @param  log_file Genral log file
 * @param  history_file History CSV file
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 * @return Program return value
 */
int cgp_main(
    // mode
    algorithm_t mode,

    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archives
    archive_t cgp_archive,
    archive_t pred_archive,

    // config
    config_t *config,
    vault_storage_t *vault,

    // input
    img_image_t img_noisy,

    // log files
    char *best_circuit_file_name_txt,
    char *best_circuit_file_name_chr,
    FILE *log_file,
    FILE *history_file,

    // status
    bool *finished)
{
    ga_fitness_t cgp_parent_fitness;
    bw_history_t bw_history;
    bw_init_history(&bw_history);

    int baldwin_last_generation = cgp_population->generation;

    while (!(*finished)) {

        // create children and evaluate new generation
        ga_create_children(cgp_population);

        #pragma omp critical (PRED_ARCHIVE)
        {
            cgp_parent_fitness = cgp_population->best_fitness;
            ga_evaluate_pop(cgp_population);
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

        // whether we are storing to vault now
        bool store_now = (cgp_population->generation % config->vault_interval) == 0;

        // whether we found better solution
        bool is_better = ga_is_better(cgp_population->problem_type, cgp_population->best_fitness, cgp_parent_fitness);

        // whether we should log now
        bool log_now = config->log_interval && (store_now || (cgp_population->generation % config->log_interval) == 0);

        // whether we update evolution params now
        bool apply_baldwin = false;
        if (mode == baldwin) {
            if (is_better) {
                apply_baldwin = true;

            } else if (config->bw_interval) {
                int diff = cgp_population->generation - baldwin_last_generation;
                if (diff >= config->bw_interval) {
                    apply_baldwin = true;
                }
            }
        }

        if (is_better || log_now || signal || *finished) {
            _log_cgp(cgp_population, best_circuit_file_name_txt, best_circuit_file_name_chr, log_file);
        }

        // update archive if necessary
        if (is_better) {
            // save image
            save_filtered_image(config->log_dir, cgp_population, img_noisy);
            DOUBLE_LOG(log_cgp_change, log_file, cgp_parent_fitness, cgp_population->best_fitness);

            if (mode != simple_cgp) {
                // store and recalculate predictors fitness
                ga_chr_t archived;
                #pragma omp critical (CGP_ARCHIVE)
                {
                    archived = arc_insert(cgp_archive, cgp_population->best_chromosome);
                    ga_invalidate_fitness(pred_population);
                    ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));
                }
                DOUBLE_LOG(log_cgp_archived, log_file, cgp_population->best_fitness, archived->fitness);

                // drop fitness values
                ga_invalidate_fitness(pred_population);
                ga_reevaluate_pop(pred_population);
                ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));
            }
        }

        if (apply_baldwin) {
            // baldwin mode: change evolution params
            // we need to lock out predictors thread
            #pragma omp critical (PRED_LOOP)
            {
                bw_update_t result;

                // update evolution params
                bw_update_params(&bw_history, &result);
                if (result.predictor_length_changed) {
                    DOUBLE_LOG(log_predictors_length_change, log_file,
                        result.old_predictor_length, result.new_predictor_length);

                    // recalculate predictors' phenotypes
                    pred_pop_calculate_phenotype(pred_population);
                    pred_calculate_phenotype(arc_get(pred_archive, 0)->genome);

                    // reevaluate predictors
                    ga_evaluate_pop(pred_population);
                    ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));

                    // update predictors archive
                    if (ga_is_better(
                        pred_population->problem_type,
                        pred_population->best_fitness,
                        arc_get(pred_archive, 0)->fitness))
                    {
                        arc_insert(pred_archive, pred_population->best_chromosome);
                    }

                    // reevaluate CGP
                    ga_reevaluate_pop(cgp_population);
                }
            }

            baldwin_last_generation = cgp_population->generation;
        }

        if (is_better || apply_baldwin) {
            // update history log
            bw_history_entry_t *entry = bw_add_history(
                &bw_history,
                cgp_population->generation,
                fitness_eval_cgp(cgp_population->best_chromosome)
            );
            log_cgp_history(
                history_file,
                entry,
                fitness_get_cgp_evals(),
                mode == simple_cgp? -1 : pred_get_length()
            );
            bw_dump_history_asciiart(stdout, &bw_history);
        }

        // store to vault
        if (*finished || signal || store_now) {

            // PRED thread does not handle signals, so we must log for it
            if (mode != simple_cgp) {
                if (config->log_interval && (pred_population->generation % config->log_interval) != 0) {
                     DOUBLE_LOG_PRED(log_pred_progress, log_file, pred_population, pred_archive);
                }
            }

            // log that we are storing to vault
            log_vault(stdout, cgp_population);

            // flush log file
            fflush(log_file);

            // store to vault
            if (config->vault_enabled) {
                // freeze predictors during save
                #pragma omp critical (PRED_LOOP)
                {
                    vault_store(vault, cgp_population);
                }
            }
        }

        if (*finished || signal > 0) {
            DOUBLE_LOG(log_cgp_finished, log_file, cgp_population);
            save_best_image(config->log_dir, cgp_population, img_noisy);
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
 * @param  mode
 * @param  cgp_population
 * @param  pred_population
 * @param  pred_archive Predictors archive
 * @param  config
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 */
void pred_main(
    // mode
    algorithm_t mode,

    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archive
    archive_t pred_archive,

    // config
    config_t *config,

    // log
    FILE *log_file,

    // status
    bool *finished)
{
    while (!(*finished)) {

        #pragma omp critical (PRED_LOOP)
        {
            // next generation
            ga_create_children(pred_population);
            #pragma omp critical (CGP_ARCHIVE)
            {
                ga_evaluate_pop(pred_population);
            }

            // log progress
            bool is_better = ga_is_better(pred_population->problem_type, pred_population->best_fitness, arc_get(pred_archive, 0)->fitness);
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
                #pragma omp critical (PRED_ARCHIVE)
                {
                    arc_insert(pred_archive, pred_population->best_chromosome);
                    ga_reevaluate_pop(cgp_population);
                }
            }
        }
    }
}