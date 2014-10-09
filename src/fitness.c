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
#include <math.h>

#include "cpu.h"
#include "fitness.h"
#include "cgp_avx.h"


static img_image_t _original_image;
static img_window_array_t _noisy_image_windows;
static archive_t _cgp_archive;
static archive_t _pred_archive;
static double _psnr_coeficient;

static long _cgp_evals;


static inline double fitness_psnr_coeficient(int pixels_count)
{
    return 255 * 255 * (double)pixels_count;
}


/**
 * Initializes fitness module - prepares test image
 * @param original
 * @param noisy
 * @param cgp_archive
 * @param pred_archive
 */
void fitness_init(img_image_t original, img_image_t noisy,
    archive_t cgp_archive, archive_t pred_archive)
{
    assert(original->width == noisy->width);
    assert(original->height == noisy->height);
    assert(original->comp == noisy->comp);

    _original_image = original;
    _noisy_image_windows = img_split_windows(noisy);
    _cgp_archive = cgp_archive;
    _pred_archive = pred_archive;
    _psnr_coeficient = fitness_psnr_coeficient(_noisy_image_windows->size);
    _cgp_evals = 0;
}


/**
 * Deinitialize fitness module internals
 */
void fitness_deinit()
{
    img_windows_destroy(_noisy_image_windows);
}


/**
 * Returns number of performed CGP evaluations
 */
long fitness_get_cgp_evals()
{
    return _cgp_evals;
}


/**
 * Filters image using given filter. Caller is responsible for freeing
 * the filtered image
 *
 * @param  chr
 * @return fitness value
 */
img_image_t fitness_filter_image(ga_chr_t chr)
{
    img_image_t filtered = img_create(_original_image->width, _original_image->height,
        _original_image->comp);

    for (int i = 0; i < _noisy_image_windows->size; i++) {
        img_window_t *w = &_noisy_image_windows->windows[i];

        cgp_value_t *inputs = w->pixels;
        cgp_value_t output_pixel;
        cgp_get_output(chr, inputs, &output_pixel);

        img_set_pixel(filtered, w->pos_x, w->pos_y, output_pixel);
    }

    return filtered;
}


/**
 * Calculates difference between original and filtered pixel
 *
 * @param  chr
 * @param  w
 * @return
 */
double _fitness_get_diff(ga_chr_t chr, img_window_t *w)
{
    cgp_value_t *inputs = w->pixels;
    cgp_value_t output_pixel;
    cgp_get_output(chr, inputs, &output_pixel);
    #pragma omp atomic
        _cgp_evals += 1;
    return output_pixel - img_get_pixel(_original_image, w->pos_x, w->pos_y);
}


/**
 * Calculates difference between original and filtered pixel
 *
 * @param  chr
 * @param  w
 * @return
 */
double _fitness_get_sqdiffsum_avx(ga_chr_t chr, img_window_t *w)
{
    cgp_value_t inputs[32][CGP_INPUTS] __attribute__ ((aligned (32)));
    for (int i = 0; i < 32; i++) {
        memcpy(inputs[i], w[i].pixels, sizeof(cgp_value_t) * CGP_INPUTS);
    }

    cgp_value_t outputs[32][1] __attribute__ ((aligned (32)));

    cgp_get_output_avx(chr, inputs, outputs);

    double sum = 0;
    for (int i = 0; i < 32; i++) {
        cgp_value_t output_pixel = outputs[i][0];
        double diff = output_pixel - img_get_pixel(_original_image, w[i].pos_x, w[i].pos_y);
        sum += diff * diff;
    }
    #pragma omp atomic
        _cgp_evals += 32;
    return sum;
}


double _fitness_get_sqdiffsum_scalar(ga_chr_t chr)
{
    double sum = 0;
    for (int i = 0; i < _noisy_image_windows->size; i++) {
        img_window_t *w = &_noisy_image_windows->windows[i];
        double diff = _fitness_get_diff(chr, w);
        sum += diff * diff;
    }
    return sum;
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

#ifdef AVX2

    if(can_use_intel_core_4th_gen_features()) {
        assert((_noisy_image_windows->size % 32) == 0);
        for (int i = 0; i < _noisy_image_windows->size; i += 32) {
            img_window_t *w = &_noisy_image_windows->windows[i];
            double subsum = _fitness_get_sqdiffsum_avx(chr, w);
            sum += subsum;
        }

    } else {
        sum = _fitness_get_sqdiffsum_scalar(chr);
    }

#else

    sum = _fitness_get_sqdiffsum_scalar(chr);

#endif

    return _psnr_coeficient / sum;
}


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_or_predict_cgp(ga_chr_t chr)
{
    if (_pred_archive && _pred_archive->stored > 0) {
        return fitness_predict_cgp(chr, arc_get(_pred_archive, 0));
    } else {
        return fitness_eval_cgp(chr);
    }
}


/**
 * Predictes CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_predict_cgp(ga_chr_t cgp_chr, ga_chr_t pred_chr)
{
    pred_genome_t predictor = (pred_genome_t) pred_chr->genome;

    // PSNR coefficcient is different here (less pixels are used)
    double coef = fitness_psnr_coeficient(predictor->used_genes);
    double sum = 0;

    for (int i = 0; i < predictor->used_genes; i++) {

        // fetch window specified by predictor
        pred_gene_t index = predictor->genes[i];
        assert(index < _noisy_image_windows->size);
        img_window_t *w = &_noisy_image_windows->windows[index];

        double diff = _fitness_get_diff(cgp_chr, w);
        sum += diff * diff;
    }

    return coef / sum;
}


/**
 * Evaluates predictor fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_predictor(ga_chr_t pred_chr)
{
    double sum = 0;
    for (int i = 0; i < _cgp_archive->stored; i++) {
        ga_chr_t cgp_chr = arc_get(_cgp_archive, i);
        double predicted = fitness_predict_cgp(cgp_chr, pred_chr);
        sum += fabs(cgp_chr->fitness - predicted);
    }
    return sum / _cgp_archive->stored;
}
