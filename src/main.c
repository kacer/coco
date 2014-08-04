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


#define CGP_MUTATION_RATE 5  /* number of max. changed genes */
#define CGP_POP_SIZE 8
#define CGP_GENERATIONS 20000


// SIGINT handler
volatile sig_atomic_t interrupted = 0;
void sigint_handler(int _)
{
    signal(SIGINT, sigint_handler);
    interrupted = 1;
}


void print_results(int generation, cgp_pop population, img_image noisy)
{
    printf("Generation %4d: best fitness %.20lf (index %d)\n", generation, population->best_fitness, population->best_chr_index);

    printf(".--------------.\n"
           "| Best circuit |\n"
           "'--------------'\n");
    cgp_dump_chr_asciiart(population->chromosomes[population->best_chr_index], stdout);
    printf("\n");

    img_image filtered = fitness_filter_image(population->chromosomes[population->best_chr_index], noisy);
    img_save_bmp(filtered, "img_filtered.bmp");
    img_destroy(filtered);
}


int main(int argc, char const *argv[])
{
    img_image original = img_load("./images/lena_gray_256.png");
    if (!original) {
        fprintf(stderr, "Failed to load original image.\n");
        return 1;
    }

    img_image noisy = img_load("./images/lena_gray_256_25_uniform.png");
    if (!noisy) {
        fprintf(stderr, "Failed to load noisy image.\n");
        return 1;
    }

    img_save_bmp(original, "img_original.bmp");
    img_save_bmp(noisy, "img_noisy.bmp");

    rand_init();
    fitness_init(original, noisy);
    cgp_init(&fitness_eval_cgp, maximize);

    printf("Initial PSNR value:           %.20lf\n", fitness_psnr(original, noisy));

    /*
    cgp_chr chr = cgp_create_chr();
    cgp_evaluate_chr(chr);
    cgp_dump_chr(chr, stdout, asciiart);
    putchar('\n');
    */

    cgp_pop population = cgp_create_pop(CGP_POP_SIZE);
    cgp_evaluate_pop(population);

    signal(SIGINT, sigint_handler);

    int generation = 0;
    int interrupted_generation = -1;
    for (; generation < CGP_GENERATIONS; generation++) {
        printf("Generation %4d: best fitness %.20lf (index %d)\n", generation, population->best_fitness, population->best_chr_index);
        cgp_next_generation(population, CGP_MUTATION_RATE);

        if (interrupted) {
            if (interrupted_generation >= 0 && interrupted_generation > generation - 10) {
                break;
            }
            print_results(generation, population, noisy);
            interrupted = 0;
            interrupted_generation = generation;
        }
    }

    print_results(generation, population, noisy);

    cgp_destroy_pop(population);
    cgp_deinit();
    fitness_deinit();

    img_destroy(original);
    img_destroy(noisy);

    return 0;
}
