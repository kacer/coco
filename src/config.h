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
#include <stdbool.h>

#include "files.h"


typedef enum
{
    simple_cgp = 0,
    predictors,
    baldwin,
} algorithm_t;


// multiple const to avoid "unused variable" warnings
static const char * const config_algorithm_names[] = {
    "cgp",
    "predictors",
    "baldwin"
};


typedef struct
{
    int max_generations;
    double target_fitness;
    algorithm_t algorithm;
    unsigned int random_seed;

    char input_image[MAX_FILENAME_LENGTH + 1];
    char noisy_image[MAX_FILENAME_LENGTH + 1];

    int cgp_mutate_genes;
    int cgp_population_size;
    int cgp_archive_size;

    float pred_size;
    float pred_mutation_rate;
    float pred_offspring_elite;
    float pred_offspring_combine;
    int pred_population_size;

    int log_interval;
    char log_dir[MAX_FILENAME_LENGTH + 1];

    bool vault_enabled;
    int vault_interval;

} config_t;


static inline void print_help() {
    puts(
        "Colearning in Coevolutionary Algorithms\n"
        "Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>\n"
        "\n"
        "Master Thesis\n"
        "2014/2015\n"
        "\n"
        "Supervisor: Ing. Michaela Šikulová <isikulova@fit.vutbr.cz>\n"
        "\n"
        "Faculty of Information Technologies\n"
        "Brno University of Technology\n"
        "http://www.fit.vutbr.cz/\n"
        "     _       _\n"
        "  __(.)=   =(.)__\n"
        "  \\___)     (___/\n"
        "\n"
        "To see system configuration just call:\n"
        "    ./coco\n"
        "\n"
        "\n"
        "To run evolution:\n"
        "    ./coco --original original.png --noisy noisy.png [options]\n"
        "\n"
        "Command line options:\n"
        "    --help, -h\n"
        "          Show this help and exit\n"
        "\n"
        "Required:\n"
        "    --original FILE, -i FILE\n"
        "          Original image filename\n"
        "    --noisy FILE, -n FILE\n"
        "          Noisy image filename\n"
        "\n"
        "Optional:\n"
        "    --algorithm ALG, -a ALG\n"
        "          Evolution algorithm selection, one of {cgp|predictors|baldwin},\n"
        "          default is \"predictors\"\n"
        "\n"
        "    --random-seed NUM, -r ALG\n"
        "          PRNG seed value, default is obtained using gettimeofday() call\n"
        "\n"
        "    --max-generations NUM, -g NUM\n"
        "          Stop after given number of CGP generations, default is 50000\n"
        "\n"
        "    --target-psnr NUM, -g NUM\n"
        "          Stop after reaching given PSNR (0 to disable), default is 0\n"
        "          If --target-fitness is specified, only one option is used.\n"
        "\n"
        "    --target-fitness NUM, -g NUM\n"
        "          Stop after reaching given fitness (0 to disable), default is 0\n"
        "          Fitness can be obtained from PSNR as F = 10 ^ (PSNR / 10).\n"
        "          If --target-psnr is specified, only one option is used.\n"
        "\n"
        "    --vault, -v\n"
        "          Enable vault, it is disabled by default\n"
        "\n"
        "    --vault-interval NUM, -w NUM\n"
        "          Vault storing interval (in generations), default is 200\n"
        "\n"
        "    --log-dir DIR, -l DIR\n"
        "          Log (results and vault) directory, default is \"cocolog\"\n"
        "\n"
        "    --log-interval NUM, -k NUM\n"
        "          Logging interval (in generations), default is 0\n"
        "          If zero, only fitness changes are logged\n"
        "\n"
        "    --cgp-mutate NUM, -m NUM\n"
        "          Number of (max) mutated genes in CGP, default is 5\n"
        "\n"
        "    --cgp-population-size NUM, -p NUM\n"
        "          CGP population size, default is 8\n"
        "\n"
        "    --cgp-archive-size NUM, -s NUM\n"
        "          CGP archive size, default is 10\n"
        "\n"
        "    --pred-size NUM, -S NUM\n"
        "          Predictor size (in percent), default is 0.25\n"
        "\n"
        "    --pred-mutate NUM, -M NUM\n"
        "          Predictor mutation rate (in percent), default is 0.05\n"
        "\n"
        "    --pred-population-size NUM, -P NUM\n"
        "          Predictors population size, default is 8\n"
    );
}


/**
 * Load configuration from command line
 */
int config_load_args(int argc, char **argv, config_t *cfg);


/**
 * Load configuration from XML file
 */
int config_load_file(FILE *file, config_t *cfg);


/**
 * Load configuration from XML file
 */
void config_save_file(FILE *file, config_t *cfg);
