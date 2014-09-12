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
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <ctype.h>
#include <errno.h>

#include "config.h"


#define OPT_ALGORITHM 'a'

#define OPT_ORIGINAL 'i'
#define OPT_NOISY 'n'

#define OPT_VAULT_DIR 'v'
#define OPT_VAULT_INTERVAL 'w'

#define OPT_LOG_INTERVAL 'l'

#define OPT_RESULTS_DIR 'r'

#define OPT_CGP_MUTATE 'm'
#define OPT_CGP_POPSIZE 'p'
#define OPT_CGP_ARCSIZE 's'

#define OPT_PRED_SIZE 'S'
#define OPT_PRED_MUTATE 'M'
#define OPT_PRED_POPSIZE 'P'

#define OPT_HELP 'h'

#define CHECK_FILENAME_LENGTH do { \
    if (strlen(optarg) > FILENAME_LENGTH - 1) { \
        fprintf(stderr, "Option %s is too long (limit: %d chars)\n", \
            long_options[option_index].name, \
            FILENAME_LENGTH - 1); \
        return 1; \
    } \
} while(0);

#define PARSE_INT(dst) do { \
    char *endptr; \
    (dst) = strtol(optarg, &endptr, 10); \
    if (*endptr != '\0' && endptr != optarg && errno != ERANGE) { \
        fprintf(stderr, "Option %s requires valid integer\n", \
            long_options[option_index].name); \
        return 1; \
    } \
} while(0);

#define PARSE_DOUBLE(dst) do { \
    char *endptr; \
    (dst) = strtod(optarg, &endptr); \
    if (*endptr != '\0' && endptr != optarg && errno != ERANGE) { \
        fprintf(stderr, "Option %s requires valid float\n", \
            long_options[option_index].name); \
        return 1; \
    } \
} while(0);


/**
 * Load configuration from command line
 */
int config_load_args(int argc, char **argv, config_t *cfg)
{
    assert(cfg != NULL);

    static struct option long_options[] =
    {
        {"help", no_argument, 0, OPT_HELP},

        /* Algorithm mode */
        {"algorithm", required_argument, 0, OPT_ALGORITHM},

        /* Input images */
        {"original", required_argument, 0, OPT_ORIGINAL},
        {"noisy", required_argument, 0, OPT_NOISY},

        /* Vault */
        {"vault-dir", required_argument, 0, OPT_VAULT_DIR},
        {"vault-interval", required_argument, 0, OPT_VAULT_INTERVAL},

        /* Logging */
        {"log-interval", required_argument, 0, OPT_LOG_INTERVAL},
        {"results-dir", required_argument, 0, OPT_RESULTS_DIR},

        /* CGP */
        {"cgp-mutate", required_argument, 0, OPT_CGP_MUTATE},
        {"cgp-population-size", required_argument, 0, OPT_CGP_POPSIZE},
        {"cgp-archive-size", required_argument, 0, OPT_CGP_ARCSIZE},

        /* Predictors */
        {"pred-size", required_argument, 0, OPT_PRED_SIZE},
        {"pred-mutate", required_argument, 0, OPT_PRED_MUTATE},
        {"pred-population-size", required_argument, 0, OPT_PRED_POPSIZE},

        {0, 0, 0, 0}
    };

    static const char *short_options = "ha:i:n:v:w:l:r:m:p:s:S:M:P";

    while (1) {
        int option_index;
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c == - 1) break;

        switch (c) {
            case OPT_HELP:
                print_help();
                return 1;

            case OPT_ALGORITHM:
                if (strcmp(optarg, "cgp") == 0) {
                    cfg->algorithm = simple_cgp;
                } else if (strcmp(optarg, "predictors") == 0) {
                    cfg->algorithm = predictors;
                } else if (strcmp(optarg, "baldwin") == 0) {
                    cfg->algorithm = baldwin;
                } else {
                    fprintf(stderr, "Invalid algorithm (options: cgp, predictors, baldwin)\n");
                    return 1;
                }
                break;

            case OPT_ORIGINAL:
                CHECK_FILENAME_LENGTH;
                strncpy(cfg->input_image, optarg, FILENAME_LENGTH);
                break;

            case OPT_NOISY:
                CHECK_FILENAME_LENGTH;
                strncpy(cfg->noisy_image, optarg, FILENAME_LENGTH);
                break;

            case OPT_VAULT_DIR:
                CHECK_FILENAME_LENGTH;
                strncpy(cfg->vault_directory, optarg, FILENAME_LENGTH);
                cfg->vault_enabled = true;
                break;

            case OPT_VAULT_INTERVAL:
                PARSE_INT(cfg->vault_interval);
                break;

            case OPT_LOG_INTERVAL:
                PARSE_INT(cfg->log_interval);
                break;

            case OPT_RESULTS_DIR:
                CHECK_FILENAME_LENGTH;
                strncpy(cfg->results_dir, optarg, FILENAME_LENGTH);
                break;

            case OPT_CGP_MUTATE:
                PARSE_INT(cfg->cgp_mutate_genes);
                break;

            case OPT_CGP_POPSIZE:
                PARSE_INT(cfg->cgp_population_size);
                break;

            case OPT_CGP_ARCSIZE:
                PARSE_INT(cfg->cgp_archive_size);
                break;

            case OPT_PRED_SIZE:
                PARSE_DOUBLE(cfg->pred_size);
                if (cfg->pred_size > 1) {
                    cfg->pred_size /= 100.0;
                }
                break;

            case OPT_PRED_MUTATE:
                PARSE_DOUBLE(cfg->pred_mutation_rate);
                if (cfg->pred_mutation_rate > 1) {
                    cfg->pred_mutation_rate /= 100.0;
                }
                break;

            case OPT_PRED_POPSIZE:
                PARSE_INT(cfg->pred_population_size);
                break;

            default:
                return 1;

        }
    }

    return 0;
}
