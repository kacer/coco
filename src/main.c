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


#define CGP_MUTATION_RATE 5   /* number of max. changed genes */
#define CGP_POP_SIZE 8
#define CGP_GENERATIONS 30000

#define PRINT_INTERVAL 10
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
    interrupted = 1;
    cpu_limit_reached = 1;
}


void print_results(cgp_pop population, img_image noisy)
{
    printf("Generation %4d: best fitness %.20g (index %d)\n", population->generation, population->best_fitness, population->best_chr_index);

    printf(".--------------.\n"
           "| Best circuit |\n"
           "'--------------'\n");
    cgp_dump_chr_asciiart(population->chromosomes[population->best_chr_index], stdout);
    printf("\n");

    char filename[25 + 8 + 1];
    snprintf(filename, 25 + 8 + 1, "results/img_filtered_%08d.bmp", population->generation);
    img_image filtered = fitness_filter_image(population->chromosomes[population->best_chr_index]);
    img_save_bmp(filtered, filename);
    img_destroy(filtered);
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

    printf("Mutation rate: %d genes max\n", CGP_MUTATION_RATE);
    printf("Initial PSNR value:           %.20g\n", fitness_psnr(original, noisy));


    // setup vault storage

    vault_storage vault = {
        .directory = "vault",
    };


    // initialize everything

    rand_init();
    fitness_init(original, noisy);
    cgp_init(&fitness_eval_cgp, maximize);
    vault_init(vault);


    // try to restart from vault OR create the initial population

    cgp_pop population;
    if (vault_retrieve(vault, &population) == 0) {
        printf("Population retrieved from vault.\n");

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
            printf("Generation %4d: best fitness %.20g (index %d)\n", population->generation, population->best_fitness, population->best_chr_index);
        }
        cgp_next_generation(population, CGP_MUTATION_RATE);

        if (interrupted) {
            if (interrupted_generation >= 0 && interrupted_generation > population->generation - 10) {
                goto cleanup;
            }
            print_results(population, noisy);

            if (cpu_limit_reached) {
                vault_store(vault, population);
                fprintf(stderr, "Exit caused by SIGXCPU signal.\n");
                retval = 10;
                goto cleanup;
            }

            interrupted = 0;
            interrupted_generation = population->generation;
            vault_store(vault, population);

        } else if ((population->generation % VAULT_INTERVAL) == 0) {
            vault_store(vault, population);
        }
    }
    print_results(population, noisy);


    // cleanup everything
cleanup:
    cgp_destroy_pop(population);
    cgp_deinit();
    fitness_deinit();

    img_destroy(original);
    img_destroy(noisy);

    return retval;
}
