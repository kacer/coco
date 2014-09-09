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

#include "cgp.h"
#include "image.h"
#include "random.h"
#include "fitness.h"
#include "vault.h"

#ifdef NCURSES
    #include "windows.h"
#endif


const char* HELP_MESSAGE =
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
    "Usage:"
    "    ./coco -i original.png -n noisy.png [-v vault] [-g]\n"
    "Command line options:\n"
    "    -h      Show this help and exit\n"
    "    -i      Original image filename\n"
    "    -n      Noisy image filename\n"
    "    -v      Vault directory (default is \"vault\"\n"
#ifdef NCURSES
    "    -g      Use graphic interface\n"
#endif
;


#define CGP_MUTATION_RATE 5   /* number of max. changed genes */
#define CGP_POP_SIZE 8
#define CGP_GENERATIONS 50000

#define PRINT_INTERVAL 20
#define VAULT_INTERVAL 200


// whether to use ncurses mode or not
bool using_ncurses = false;

// outputting
void print_progress(ga_pop_t population);
void print_results(ga_pop_t population);


// SIGINT handler
volatile sig_atomic_t interrupted = 0;
void sigint_handler(int _)
{
    signal(SIGINT, sigint_handler);
    interrupted = 1;
}


// SIGXCPU handler
volatile sig_atomic_t cpu_limit_reached = 0;
void sigxcpu_handler(int _)
{
    signal(SIGXCPU, sigxcpu_handler);
    cpu_limit_reached = 1;
}


void save_image(ga_pop_t population, img_image noisy)
{
    char filename[25 + 8 + 1];
    snprintf(filename, 25 + 8 + 1, "results/img_filtered_%08d.bmp", population->generation);
    img_image filtered = fitness_filter_image(population->chromosomes[population->best_chr_index]);
    img_save_bmp(filtered, filename);
    img_destroy(filtered);
}


/* standard console outputing */

#define _SLOWLOG(...) { printf(__VA_ARGS__); printf("\n"); }


void _print_progress(ga_pop_t population)
{
    printf("Generation %4d: best fitness %.20g\n",
        population->generation, population->best_fitness);
}


void _print_results(ga_pop_t population)
{
    _print_progress(population);

    printf(".--------------.\n"
           "| Best circuit |\n"
           "'--------------'\n");
    cgp_dump_chr_asciiart(population->chromosomes[population->best_chr_index], stdout);
    printf("\n");
}


/* (optional) ncurses outputing */

#ifdef NCURSES

    #define SLOWLOG(...) do { \
        if (using_ncurses) { _NC_SLOWLOG(__VA_ARGS__); } \
        else { _SLOWLOG(__VA_ARGS__); } \
    } while(false);


    void _nc_print_progress(ga_pop_t population)
    {
        static ga_fitness_t last_best = 0;

        _NC_PROGRESS("Generation %4d: best fitness %.20g",
            population->generation, population->best_fitness);

        if (population->best_fitness > last_best) {
            last_best = population->best_fitness;
            SLOWLOG("Generation %4d: best fitness %.20g",
                population->generation, population->best_fitness);
        }

        if (population->best_chr_index >= 0) {
            char* buffer = NULL;
            size_t bufferSize = 0;
            FILE* memory = open_memstream(&buffer, &bufferSize);

            cgp_dump_chr_asciiart(population->chromosomes[population->best_chr_index], memory);
            fclose(memory);

            werase(circuit_window);
            wprintw(circuit_window, "%s", buffer);
            wrefresh(circuit_window);

            free(buffer);
        }
    }

    void _nc_print_results(ga_pop_t population)
    {
        _nc_print_progress(population);
    }

    void print_progress(ga_pop_t population)
    {
        if (using_ncurses) _nc_print_progress(population);
        else _print_progress(population);
    }

    void print_results(ga_pop_t population)
    {
        if (using_ncurses) _nc_print_results(population);
        else _print_results(population);
    }

#else

    #define SLOWLOG _SLOWLOG
    #define print_progress _print_progress
    #define print_results _print_results

#endif /* NCURSES */


#define SAVE_IMAGE_AND_STATE() { \
    print_results(population); \
    save_image(population, noisy); \
    vault_store(&vault, population); \
    SLOWLOG("Current output image and state stored."); \
}


int main(int argc, char *argv[])
{
    // application return code
    int retval = 0;

    // images
    img_image original = NULL;
    img_image noisy = NULL;
    vault_storage vault = { .directory = "vault" };

    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "i:n:v:hg")) != -1) {
        switch (opt) {
            case 'i':
                original = img_load(optarg);
                break;

            case 'n':
                noisy = img_load(optarg);
                break;

            case 'v':
                vault.directory = (char*) malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(vault.directory, optarg);
                break;

            case 'h':
                fprintf(stderr, "%s", HELP_MESSAGE);
                return 1;

            case 'g':
#ifdef NCURSES
                using_ncurses = true;
                break;
#else
                fprintf(stderr,
                    "Graphic interface is not available.\n"
                    "Compile with -DNCURSES to enable.\n"
                );
                return 1;
#endif

            case '?':
                if (optopt == 'i' || optopt == 'n' || optopt == 'v') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                }

                fprintf(stderr, "Use -h for help.\n");
                return 1;
        }
    }

    if (!original) {
        fprintf(stderr, "Failed to load original image or no filename given.\n");
        return 1;
    }

    if (!noisy) {
        fprintf(stderr, "Failed to load noisy image or no filename given.\n");
        return 1;
    }

    img_save_bmp(original, "results/img_original.bmp");
    img_save_bmp(noisy, "results/img_noisy.bmp");

    // initialize everything
    cgp_init();
    rand_init();
    fitness_init(original, noisy);
    vault_init(&vault);

#ifdef NCURSES
    if (using_ncurses) windows_init();
#endif

    SLOWLOG("Mutation rate: %d genes max", CGP_MUTATION_RATE);
    SLOWLOG("Initial PSNR value:           %.20g", fitness_psnr(original, noisy));


    // try to restart from vault OR create the initial population

    ga_pop_t population;
    if (vault_retrieve(&vault, &population) == 0) {
        SLOWLOG("Population retrieved from vault.");

    } else {
        population = cgp_init_pop(CGP_POP_SIZE);
    }

    cgp_set_mutation_rate(CGP_MUTATION_RATE);
    cgp_set_fitness_func(population, fitness_eval_cgp);
    ga_evaluate_pop(population);
    print_progress(population);


    // install signal handlers

    signal(SIGINT, sigint_handler);
    signal(SIGXCPU, sigxcpu_handler);


    // evolve
    long interrupted_generation = -1;

    while (population->generation < CGP_GENERATIONS) {
        if ((population->generation % PRINT_INTERVAL) == 0) {
            print_progress(population);
        }
        ga_next_generation(population);

        if (cpu_limit_reached) {
            print_results(population);
            SAVE_IMAGE_AND_STATE();
            retval = 10;
            goto cleanup;
        }

        if ((population->generation % VAULT_INTERVAL) == 0) {
            vault_store(&vault, population);
            save_image(population, noisy);
        }

#ifdef NCURSES
        if (using_ncurses) {
            windows_event ev;
            while ((ev = windows_check_events()) != none) {
                switch (ev) {
                    case save_state:
                        SAVE_IMAGE_AND_STATE();
                        break;

                    case quit:
                        SAVE_IMAGE_AND_STATE();
                        goto cleanup;

                    default:
                        break;
                }
            }
        }
#endif

    if (interrupted) {

        if (interrupted_generation >= 0 && interrupted_generation > population->generation - 1000) {
            goto cleanup;
        }

        print_results(population);
        SAVE_IMAGE_AND_STATE();

        interrupted = 0;
        interrupted_generation = population->generation;
    }

    } // while loop

    print_results(population);
    save_image(population, noisy);


    // cleanup everything
cleanup:
#ifdef NCURSES
    if (using_ncurses) windows_destroy();
#endif

    ga_destroy_pop(population);
    cgp_deinit();
    fitness_deinit();

    img_destroy(original);
    img_destroy(noisy);

    return retval;
}
