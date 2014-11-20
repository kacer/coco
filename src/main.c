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


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <signal.h>

#ifdef _OPENMP
  #include <omp.h>
#endif

#include "cpu.h"
#include "algo.h"
#include "files.h"
#include "image.h"
#include "random.h"
#include "config.h"
#include "cgp/cgp.h"
#include "fitness.h"
#include "archive.h"
#include "logging.h"
#include "predictors.h"

#include <limits.h>

// signal handlers
volatile sig_atomic_t interrupted = 0;
volatile sig_atomic_t cpu_limit_reached = 0;

// if SIGINT is received twice within this gap, program exits
const int SIGINT_GENERATIONS_GAP = 1000;

// special `check_signals` return value indicating first catch of SIGINT
const int SIGINT_FIRST = -1;


/**
 * Handles SIGINT. Sets `interrupted` flag.
 */
void sigint_handler(int _)
{
    signal(SIGINT, sigint_handler);
    interrupted = 1;
}


/**
 * Handles SIGXCPU. Sets `cpu_limit_reached` flag.
 */
void sigxcpu_handler(int _)
{
    signal(SIGXCPU, sigxcpu_handler);
    cpu_limit_reached = 1;
}


/**
 * Checks for SIGXCPU and SIGINT signals
 * @return Received signal code
 */
int check_signals(int current_generation)
{
    // when SIGINT was received last time
    static long interrupted_generation = -1;

    // SIGXCPU
    if (cpu_limit_reached) {
        fprintf(stderr, "SIGXCPU received!\n");
        return SIGXCPU;
    }

    // SIGINT
    if (interrupted) {
        if (interrupted_generation >= 0
            &&  interrupted_generation > current_generation - SIGINT_GENERATIONS_GAP) {
            fprintf(stderr, "SIGINT received and terminating!\n");
            return SIGINT;
        }

        fprintf(stderr, "SIGINT received!\n");
        interrupted = 0;
        interrupted_generation = current_generation;
        return SIGINT_FIRST;
    }

    return 0;
}


// configuration
static config_t config = {
    .max_generations = 50000,
    .target_fitness = 0,
    .algorithm = predictors,

    .cgp_mutate_genes = 5,
    .cgp_population_size = 8,
    .cgp_archive_size = 10,

    .pred_size = 0.25,
    .pred_initial_size = 0,
    .pred_mutation_rate = 0.05,
    .pred_population_size = 10,
    .pred_offspring_elite = 0.25,
    .pred_offspring_combine = 0.5,
    .pred_genome_type = permuted,

    .bw_interval = 0,
    .bw_config = {
        .algorithm = bwalg_last,
        .use_absolute_increments = false,
        .inaccuracy_tolerance = 1.2,
        .inaccuracy_coef = 2.0,
        .zero_epsilon = 0.001,
        .slow_threshold = 0.1,

        .zero_coef = 0.93,
        .decrease_coef = 0.97,
        .increase_slow_coef = 1.03,
        .increase_fast_coef = 1.00,

    },

    .bw_zero_increment_percent = -0.07,
    .bw_decrease_increment_percent = -0.03,
    .bw_increase_slow_increment_percent = 0.03,
    .bw_increase_fast_increment_percent = 0,

    .log_interval = 0,
    .log_dir = "cocolog",
};


// predictor evolution settings and current state
static pred_metadata_t pred_metadata;

// algorithm working data
// everything else is statically initialized to NULL
static algo_data_t work_data = {
    .config = &config,
    .baldwin_state = {
        .apply_now = false,
        .last_applied_generation = 0,
    },
    .finished = false,
};


/******************************************************************************/


int main(int argc, char *argv[])
{
    // cannot be set in initializer
    config.random_seed = rand_seed_from_time();

    // baldwin
    bw_init_history(&work_data.history);

    // loggers list
    logger_init_list(&work_data.loggers);

    logger_add(&work_data.loggers,
        logger_text_create(fopen("cocolog/test.log", "w")));

    // images
    img_image_t img_original;
    img_image_t img_noisy;

    // application exit code
    int retval = 0;

    /*
        Log system configuration
     */

    #ifdef _OPENMP
        printf("OpenMP is enabled. CPUs: %d. Max threads: %d.\n",
            omp_get_num_procs(), omp_get_max_threads());

            /*
                Nested parallelism is essential and must be explicitly
                enabled. Otherwise the population evaluation will be
                serialised.
             */
            omp_set_nested(true);

    #else

        printf("OpenMP is disabled, coevolution is not available.\n");

    #endif

    #ifdef AVX2
        if (can_use_intel_core_4th_gen_features()) {
            printf("AVX2 is enabled.\n");
        } else {
            printf("AVX2 is enabled, but not supported by CPU.\n");
        }
    #else
        printf("AVX2 is disabled. Recompile with -DAVX2 defined to enable.\n");
    #endif

    #ifdef SSE2
        if (can_use_sse2()) {
            printf("SSE2 is enabled.\n");
        } else {
            printf("SSE2 is enabled, but not supported by CPU.\n");
        }
    #else
        printf("SSE2 is disabled. Recompile with -DSSE2 defined to enable.\n");
    #endif

    /*
        Load configuration and images and init log directory and files
     */

    bool config_ok = true;
    config_retval_t cfg_status = config_load_args(argc, argv, &config);

    if (cfg_status == cfg_err) {
        fprintf(stderr, "Failed to load configuration.\n");
        config_ok = false;

    } else if (cfg_status == cfg_help) {
        return 1;
    }

    if ((img_original = img_load(config.input_image)) == NULL) {
        fprintf(stderr, "Failed to load original image or no filename given.\n");
        config_ok = false;
    }

    if ((img_noisy = img_load(config.noisy_image)) == NULL) {
        fprintf(stderr, "Failed to load noisy image or no filename given.\n");
        config_ok = false;
    }

    int log_create_dirs_retval = log_create_dirs(config.log_dir);
    if (log_create_dirs_retval != 0) {
        fprintf(stderr, "Error initializing results directory: %s\n", strerror(log_create_dirs_retval));
        config_ok = false;
    }

    if ((work_data.log_file = open_log_file(config.log_dir, "progress.log", true)) == NULL) {
        fprintf(stderr, "Failed to open 'progress.log' in results dir for writing.\n");
        config_ok = false;
    }

    if ((work_data.history_file = init_cgp_history_file(config.log_dir, "cgp_history.csv")) == NULL) {
        fprintf(stderr, "Failed to open 'cgp_history.csv' in results dir for writing.\n");
        config_ok = false;
    }

    if (!config_ok) {
        fprintf(stderr, "Run %s --help or %s -h to see available options.\n", argv[0], argv[0]);
        return 1;
    }


    /*
        Initialize data structures etc.
     */

    // random number generator
    rand_init_seed(config.random_seed);

    // cgp evolution
    cgp_init(config.cgp_mutate_genes, fitness_eval_or_predict_cgp);

    // predictors population and both archives
    if (config.algorithm != simple_cgp) {

        // calculate absolute predictors sizes
        int img_size = img_original->width * img_original->height;
        int pred_min_size = config.pred_min_size * img_size;
        int pred_max_size = config.pred_size * img_size;
        int pred_initial_size;

        // allow to set different initial size only for baldwin or circular genotype
        bool is_circular = (config.pred_genome_type == circular);
        if (config.pred_initial_size && (config.algorithm == baldwin || is_circular)) {
            pred_initial_size = config.pred_initial_size * img_size;
        } else {
            pred_initial_size = pred_max_size;
        }

        if (config.algorithm == baldwin) {

            // baldwin thresholds
            config.bw_config.min_length = pred_min_size;
            config.bw_config.max_length = pred_max_size;

            // baldwin absolute increments
            if (config.bw_config.use_absolute_increments) {
                config.bw_config.absolute_increment_base = pred_max_size;
                config.bw_config.zero_increment = pred_max_size * config.bw_zero_increment_percent;
                config.bw_config.decrease_increment = pred_max_size * config.bw_decrease_increment_percent;
                config.bw_config.increase_slow_increment = pred_max_size * config.bw_increase_slow_increment_percent;
                config.bw_config.increase_fast_increment = pred_max_size * config.bw_increase_fast_increment_percent;

                printf("Absolute increments are:\n"
                    "Base: %d pixels\n"
                    "Zero: %d pixels\n"
                    "Decrease: %d pixels\n"
                    "Slow increase: %d pixels\n"
                    "Fast increase: %d pixels\n",
                    config.bw_config.absolute_increment_base,
                    config.bw_config.zero_increment,
                    config.bw_config.decrease_increment,
                    config.bw_config.increase_slow_increment,
                    config.bw_config.increase_fast_increment
                );
            }
        }

        pred_metadata.genome_type = config.pred_genome_type;
        pred_metadata.max_gene_value = img_size - 1;
        pred_metadata.genotype_length = pred_max_size;
        pred_metadata.genotype_used_length = pred_initial_size;
        pred_metadata.mutation_rate = config.pred_mutation_rate;
        pred_metadata.offspring_elite = config.pred_offspring_elite;
        pred_metadata.offspring_combine = config.pred_offspring_combine;

        // predictors evolution
        pred_init(&pred_metadata);

        // cgp archive
        arc_func_vect_t arc_cgp_methods = {
            .alloc_genome = cgp_alloc_genome,
            .free_genome = cgp_free_genome,
            .copy_genome = cgp_copy_genome,
            .fitness = fitness_eval_cgp,
        };
        work_data.cgp_archive = arc_create(config.cgp_archive_size, arc_cgp_methods, CGP_PROBLEM_TYPE);
        if (work_data.cgp_archive == NULL) {
            fprintf(stderr, "Failed to initialize CGP archive.\n");
            return 1;
        }

        // predictor archive
        arc_func_vect_t arc_pred_methods = {
            .alloc_genome = pred_alloc_genome,
            .free_genome = pred_free_genome,
            .copy_genome = pred_copy_genome,
            .fitness = NULL,
        };
        work_data.pred_archive = arc_create(1, arc_pred_methods, PRED_PROBLEM_TYPE);
        if (work_data.pred_archive == NULL) {
            fprintf(stderr, "Failed to initialize predictors archive.\n");
            return 1;
        }
    }

    // fitness function
    fitness_init(img_original, img_noisy, work_data.cgp_archive, work_data.pred_archive);

    /*
        Populations initialization
     */

    work_data.cgp_population = cgp_init_pop(config.cgp_population_size);
    if (work_data.cgp_population == NULL) {
        fprintf(stderr, "Failed to initialize CGP population.\n");
    }

    if (config.algorithm != simple_cgp) {
        work_data.pred_population = pred_init_pop(config.pred_population_size);
        if (work_data.pred_population == NULL) {
            fprintf(stderr, "Failed to initialize predictors population.\n");
        }
    }


    /*
        If no loggers are set, use the devnull one
     */
    if (work_data.loggers.count == 0) {
        logger_add(&work_data.loggers, logger_devnull_create());
    }


    /*
        Log initial info
     */

    #ifndef _OPENMP
        if (config.algorithm != simple_cgp) {
            fprintf(stderr, "Only simple CGP is available.\n");
            fprintf(stderr, "Please recompile program with OpenMP (-fopenmp for gcc) to run coevolution.\n");
            return 1;
        }
    #endif

    printf("Configuration:\n");
    config_save_file(stdout, &config);

    printf("Algorithm: %s\n", config_algorithm_names[config.algorithm]);
    printf("Stop at generation: %d\n", config.max_generations);
    printf("Initial \"PSNR\" value:         %.10g\n",
        img_psnr(img_original, img_noisy));

    ga_evaluate_pop(work_data.cgp_population);

    if (config.algorithm != simple_cgp) {
        arc_insert(work_data.cgp_archive, work_data.cgp_population->best_chromosome);
        ga_evaluate_pop(work_data.pred_population);
        arc_insert(work_data.pred_archive, work_data.pred_population->best_chromosome);
    }

    save_original_image(config.log_dir, img_original);
    save_noisy_image(config.log_dir, img_noisy);
    save_config(config.log_dir, &config);


    /*
        Evolution itself
     */

    log_init_time();

    // install signal handlers
    signal(SIGINT, sigint_handler);
    signal(SIGXCPU, sigxcpu_handler);

    switch (config.algorithm) {

        case simple_cgp:
            retval = cgp_main(&work_data);
            break;

        case predictors:
        case baldwin:
            #pragma omp parallel sections num_threads(2)
            {
                #pragma omp section
                {
                    retval = cgp_main(&work_data);
                }
                #pragma omp section
                {
                    pred_main(&work_data);
                }
            }
            break;

        default:
            fprintf(stderr, "Algorithm '%s' is not supported.\n",
                config_algorithm_names[config.algorithm]);
            return -1;
    }


    /*
        Dump summary and clean-up
     */

    ga_fitness_t best_fitness;
    ga_chr_t best_chromosome;
    if (config.algorithm == simple_cgp) {
        best_chromosome = work_data.cgp_population->best_chromosome;
    } else {
        best_chromosome = work_data.cgp_archive->best_chromosome_ever;
    }
    best_fitness = best_chromosome->fitness;

    // dump best circuit and image
    char buffer[MAX_FILENAME_LENGTH + 1];

    snprintf(buffer, MAX_FILENAME_LENGTH + 1, "%s/best_circuit.txt", config.log_dir);
    FILE *circuit_file_txt = fopen(buffer, "w");
    if (circuit_file_txt) {
        log_cgp_circuit(circuit_file_txt, work_data.cgp_population->generation, best_chromosome);
        fclose(circuit_file_txt);
    } else {
        fprintf(stderr, "Failed to open %s!\n", buffer);
    }

    snprintf(buffer, MAX_FILENAME_LENGTH + 1, "%s/best_circuit.chr", config.log_dir);
    FILE *circuit_file_chr = fopen(buffer, "w");
    if (circuit_file_chr) {
        cgp_dump_chr_compat(best_chromosome, circuit_file_chr);
        fclose(circuit_file_chr);
    } else {
        fprintf(stderr, "Failed to open %s!\n", buffer);
    }

    // save best filtered image
    save_best_image(config.log_dir, best_chromosome, img_noisy);

    // dump evolution summary
    log_final_summary(stdout, work_data.cgp_population->generation,
        best_fitness, fitness_get_cgp_evals());
    printf("\n");
    log_time(stdout);
    printf("\n");
    config_save_file(stdout, &config);

    FILE *summary_file;
    if ((summary_file = open_log_file(config.log_dir, "summary.log", false)) == NULL) {
        fprintf(stderr, "Failed to open 'summary.log' in results dir for writing.\n");

    } else {
        log_final_summary(summary_file, work_data.cgp_population->generation,
            best_fitness, fitness_get_cgp_evals());
        fprintf(summary_file, "\n");
        log_time(summary_file);
    }

    ga_destroy_pop(work_data.cgp_population);

    if (config.algorithm != simple_cgp) {
        ga_destroy_pop(work_data.pred_population);
        arc_destroy(work_data.cgp_archive);
        arc_destroy(work_data.pred_archive);
    }
    cgp_deinit();
    fitness_deinit();

    img_destroy(img_original);
    img_destroy(img_noisy);

    logger_destroy_list(&work_data.loggers);

    fclose(work_data.log_file);
    fclose(work_data.history_file);

    return retval;
}


