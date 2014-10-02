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


typedef enum
{
    simple_cgp = 0,
    predictors,
    baldwin,
} algorithm_t;


static const char *config_algorithm_names[] = {
    "cgp",
    "predictors",
    "baldwin"
};


#define FILENAME_LENGTH 101


typedef struct
{
    int max_generations;
    algorithm_t algorithm;

    char input_image[FILENAME_LENGTH];
    char noisy_image[FILENAME_LENGTH];

    int cgp_mutate_genes;
    int cgp_population_size;
    int cgp_archive_size;

    float pred_size;
    float pred_mutation_rate;
    float pred_offspring_elite;
    float pred_offspring_combine;
    int pred_population_size;

    int log_interval;
    char results_dir[FILENAME_LENGTH];

    bool vault_enabled;
    char vault_directory[FILENAME_LENGTH];
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
        "    --max-generations NUM, -g NUM\n"
        "          Stop after given number of CGP generations, default is 50000\n"
        "\n"
        "    --vault-dir DIR, -v DIR\n"
        "          Vault directory, disabled by default\n"
        "\n"
        "    --vault-interval NUM, -w NUM\n"
        "          Vault storing interval (in generations), default is 200\n"
        "\n"
        "    --log-interval NUM, -l NUM\n"
        "          Logging interval (in generations), default is 20\n"
        "\n"
        "    --results-dir DIR, -r DIR\n"
        "          Results directory, default is \"results\"\n"
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
