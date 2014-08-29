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


#include <stdio.h>
#include <signal.h>

#include "cgp.h"
#include "image.h"
#include "random.h"
#include "fitness.h"
#include "vault.h"

#ifdef NCURSES
    #include "windows.h"
#endif


#define CGP_MUTATION_RATE 5   /* number of max. changed genes */
#define CGP_POP_SIZE 8
#define CGP_GENERATIONS 50000

#define PRINT_INTERVAL 1
#define VAULT_INTERVAL 200


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


void save_image(cgp_pop population, img_image noisy)
{
    char filename[25 + 8 + 1];
    snprintf(filename, 25 + 8 + 1, "results/img_filtered_%08d.bmp", population->generation);
    img_image filtered = fitness_filter_image(population->chromosomes[population->best_chr_index]);
    img_save_bmp(filtered, filename);
    img_destroy(filtered);
}


#ifdef NCURSES


    #define SLOWLOG _NC_SLOWLOG


    void print_progress(cgp_pop population)
    {
        static cgp_fitness_t last_best = 0;

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


    void print_results(cgp_pop population)
    {
        print_progress(population);
    }


#else


    #define SLOWLOG(...) { printf(__VA_ARGS__); printf("\n"); }


    void print_progress(cgp_pop population)
    {
        printf("Generation %4d: best fitness %.20g\n",
            population->generation, population->best_fitness);
    }


    void print_results(cgp_pop population)
    {
        print_progress(population);

        printf(".--------------.\n"
               "| Best circuit |\n"
               "'--------------'\n");
        cgp_dump_chr_asciiart(population->chromosomes[population->best_chr_index], stdout);
        printf("\n");
    }


#endif /* NCURSES */


#define SAVE_IMAGE_AND_STATE() { \
    print_results(population); \
    save_image(population, noisy); \
    vault_store(vault, population); \
    SLOWLOG("Current output image and state stored."); \
}


int main(int argc, char const *argv[])
{
    // application return code
    int retval = 0;

    // load source images

    img_image original = img_load("images/lena_gray_256.png");
    if (!original) {
        fprintf(stderr, "Failed to load original image.\n");
        return 1;
    }

    img_image noisy = img_load("images/lena_gray_256_saltpepper_15.png");
    if (!noisy) {
        fprintf(stderr, "Failed to load noisy image.\n");
        return 1;
    }

    img_save_bmp(original, "results/img_original.bmp");
    img_save_bmp(noisy, "results/img_noisy.bmp");


    // setup vault storage

    vault_storage vault = {
        .directory = "vault",
    };


    // initialize everything

    rand_init();
    fitness_init(original, noisy);
    cgp_init(&fitness_eval_cgp, maximize);
    vault_init(vault);

#ifdef NCURSES
    windows_init();
#endif

    SLOWLOG("Mutation rate: %d genes max", CGP_MUTATION_RATE);
    SLOWLOG("Initial PSNR value:           %.20g", fitness_psnr(original, noisy));


    // try to restart from vault OR create the initial population

    cgp_pop population;
    if (vault_retrieve(vault, &population) == 0) {
        SLOWLOG("Population retrieved from vault.");

    } else {
        population = cgp_create_pop(CGP_POP_SIZE);
        cgp_evaluate_pop(population);
    }


    // install signal handlers

    signal(SIGINT, sigint_handler);
    signal(SIGXCPU, sigxcpu_handler);


    // evolve

    int interrupted_generation = -1;

    while (population->generation < CGP_GENERATIONS) {
        if ((population->generation % PRINT_INTERVAL) == 0) {
            print_progress(population);
        }
        cgp_next_generation(population, CGP_MUTATION_RATE);

        if (cpu_limit_reached) {
            print_results(population);
            SAVE_IMAGE_AND_STATE();
            retval = 10;
            goto cleanup;
        }

        if ((population->generation % VAULT_INTERVAL) == 0) {
            vault_store(vault, population);
        }

#ifdef NCURSES
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
#endif

    if (interrupted) {
        if (interrupted_generation >= 0 && interrupted_generation > population->generation - 10) {
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
    windows_destroy();
#endif

    cgp_destroy_pop(population);
    cgp_deinit();
    fitness_deinit();

    img_destroy(original);
    img_destroy(noisy);

    return retval;
}
