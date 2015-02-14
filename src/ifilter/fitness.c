/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master's Thesis
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

#include "../cpu.h"
#include "../random.h"
#include "../fitness.h"
#include "fitness.h"
#include "inputdata.h"


static double _psnr_coeficient;


/* Private functions */


bool _fitness_get_diff(ga_chr_t chr, img_window_t *w, int *diff);
double _fitness_get_sqdiffsum_scalar(ga_chr_t chr);
double _fitness_get_sqdiffsum_simd(ga_chr_t chr, img_pixel_t *original,
    img_pixel_t *noisy[WINDOW_SIZE], int data_length);
double _fitness_predict_cgp_scalar(ga_chr_t cgp_chr, pred_genome_t predictor);

static inline double fitness_psnr_coeficient(int pixels_count)
{
    return 255 * 255 * (double)pixels_count;
}


/** Public and "friend" API ***************************************************/


/**
 * Initializes fitness module
 * @param config
 * @param input
 * @param cgp_archive
 * @param pred_archive
 */
void _fitness_init(config_t *config, input_data_t *input,
    archive_t cgp_archive, archive_t pred_archive)
{
    _psnr_coeficient = fitness_psnr_coeficient(input->fitness_cases);
}


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_cgp(ga_chr_t chr)
{
    double sum = 0;

    if(can_use_simd()) {
        sum = _fitness_get_sqdiffsum_simd(chr, fitness_input_data->img_original->data,
            fitness_input_data->img_noisy_simd, fitness_input_data->fitness_cases);

    } else {
        sum = _fitness_get_sqdiffsum_scalar(chr);
    }

    return _psnr_coeficient / sum;
}


/**
 * Predictes CGP circuit fitness
 *
 * @param  cgp_chr
 * @param  pred_chr
 * @return fitness value
 */
ga_fitness_t fitness_predict_cgp(ga_chr_t cgp_chr, ga_chr_t pred_chr)
{
    pred_genome_t predictor = (pred_genome_t) pred_chr->genome;

    // PSNR coefficcient is different here (less pixels are used)
    double coef = fitness_psnr_coeficient(predictor->used_pixels);
    double sum = 0;

    if (can_use_simd()) {
        sum = _fitness_get_sqdiffsum_simd(cgp_chr, predictor->output_simd,
            predictor->inputs_simd, predictor->used_pixels);

    } else {
        sum = _fitness_predict_cgp_scalar(cgp_chr, predictor);
    }

    return coef / sum;
}




/**
 * Fills simd-friendly predictor arrays with correct image data
 * @param  genome
 */
void fitness_prepare_predictor_for_simd(pred_genome_t predictor)
{
    for (int i = 0; i < predictor->used_pixels; i++) {
        pred_gene_t index = predictor->pixels[i];
        assert(index < fitness_input_data->fitness_cases);

        predictor->output_simd[i] = fitness_input_data->img_original->data[index];
        for (int w = 0; w < WINDOW_SIZE; w++) {
            predictor->inputs_simd[w][i] = fitness_input_data->img_noisy_simd[w][index];
        }
    }
}


/** Pixel value diff calculation - scalar and SIMD ****************************/


/**
 * Calculates difference between original and filtered pixel
 *
 * @param  chr
 * @param  w
 * @param  diff
 * @return Whether CGP function has changed and evaluation should be restarted
 */
bool _fitness_get_diff(ga_chr_t chr, img_window_t *w, int *diff)
{
    cgp_value_t *inputs = w->pixels;
    cgp_value_t output_pixel;
    bool should_restart = cgp_get_output(chr, inputs, &output_pixel);
    *diff = output_pixel - img_get_pixel(fitness_input_data->img_original, w->pos_x, w->pos_y);
    return should_restart;
}


double _fitness_get_sqdiffsum_scalar(ga_chr_t chr)
{
    double sum = 0;
    int diff;
    for (int i = 0; i < fitness_input_data->fitness_cases; i++) {
        img_window_t *w = &fitness_input_data->img_noisy_windows->windows[i];
        bool should_restart = _fitness_get_diff(chr, w, &diff);
        if (should_restart) {
            i = 0;
            continue;
        }
        sum += diff * diff;
    }
    #pragma omp atomic
        fitness_cgp_evals += fitness_input_data->fitness_cases;
    return sum;
}


double _fitness_get_sqdiffsum_simd(ga_chr_t chr, img_pixel_t *original, img_pixel_t *noisy[WINDOW_SIZE], int data_length)
{
    #ifdef SYMREG
        return 0;
    #endif

    fitness_simd_func_t func = NULL;
    int block_size = 0;
    double sum = 0;

    #ifdef AVX2
        if(can_use_intel_core_4th_gen_features()) {
            func = _fitness_get_sqdiffsum_avx;
            block_size = FITNESS_AVX2_STEP;
        }
    #endif

    #ifdef SSE2
        if(can_use_sse2()) {
            func = _fitness_get_sqdiffsum_sse;
            block_size = FITNESS_SSE2_STEP;
        }
    #endif

    assert(func != NULL);

    int offset = 0;
    int unaligned_bytes = data_length % block_size;
    data_length -= unaligned_bytes;

    for (; offset < data_length; offset += block_size) {
        sum += func(original, noisy, chr, offset, block_size);
        #pragma omp atomic
            fitness_cgp_evals += block_size;
    }

    // fix image data not fitting into register
    // offset is set correctly here - it points to first pixel after
    // aligned data (we subtracted no. unaligned bytes before)
    if (unaligned_bytes > 0) {
        sum += func(original, noisy, chr, offset, unaligned_bytes);
        #pragma omp atomic
            fitness_cgp_evals += unaligned_bytes;
    }

    return sum;
}


double _fitness_predict_cgp_scalar(ga_chr_t cgp_chr, pred_genome_t predictor)
{
    double sum = 0;
    int diff;

    for (int i = 0; i < predictor->used_pixels; i++) {
        // fetch window specified by predictor
        pred_gene_t index = predictor->pixels[i];
        assert(index < fitness_input_data->fitness_cases);
        img_window_t *w = &fitness_input_data->img_noisy_windows->windows[index];

        bool should_restart = _fitness_get_diff(cgp_chr, w, &diff);
        if (should_restart) {
            i = 0;
            continue;
        }
        sum += diff * diff;
    }

    #pragma omp atomic
        fitness_cgp_evals += predictor->used_pixels;

    return sum;
}
