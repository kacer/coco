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
#include <assert.h>
#include <math.h>

#include "fitness.h"


img_image_t original_image;
img_window_array_t noisy_image_windows;
archive_t _cgp_archive;


static inline double fitness_psnr_coeficient(int pixels_count)
{
    return 255 * 255 * (double)pixels_count;
}


/**
 * Initializes fitness module - prepares test image
 * @param original
 * @param noisy
 */
void fitness_init(img_image_t original, img_image_t noisy,
    archive_t cgp_archive)
{
    assert(original->width == noisy->width);
    assert(original->height == noisy->height);
    assert(original->comp == noisy->comp);

    original_image = original;
    noisy_image_windows = img_split_windows(noisy);
    _cgp_archive = cgp_archive;
}


/**
 * Deinitialize fitness module internals
 */
void fitness_deinit()
{
    img_windows_destroy(noisy_image_windows);
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
    img_image_t filtered = img_create(original_image->width, original_image->height,
        original_image->comp);

    for (int i = 0; i < noisy_image_windows->size; i++) {
        img_window_t *w = &noisy_image_windows->windows[i];

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
    return output_pixel - img_get_pixel(original_image, w->pos_x, w->pos_y);
}


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_cgp(ga_chr_t chr)
{
    double coef = fitness_psnr_coeficient(noisy_image_windows->size);
    double sum = 0;

    for (int i = 0; i < noisy_image_windows->size; i++) {
        img_window_t *w = &noisy_image_windows->windows[i];
        double diff = _fitness_get_diff(chr, w);
        sum += diff * diff;
    }

    return coef / sum;
}


/**
 * Predictes CGP circuit fitness
 *
 * @param  chr
 * @param  predictor
 * @return fitness value
 */
ga_fitness_t fitness_predict_cgp(ga_chr_t cgp_chr, ga_chr_t pred_chr)
{
    pred_genome_t predictor = (pred_genome_t) pred_chr->genome;
    // PSNR coefficcient
    double coef = fitness_psnr_coeficient(predictor->used_genes);
    double sum = 0;

    for (int i = 0; i < predictor->used_genes; i++) {

        // fetch window specified by predictor
        unsigned int index = predictor->genes[i];
        img_window_t *w = &noisy_image_windows->windows[index];

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
    return sum;
}


/**
 * Calculates fitness using the PSNR (peak signal-to-noise ratio) function.
 * The higher the value, the better the filter.
 *
 * @param  original image
 * @param  filtered image
 * @return fitness value (PSNR)
 */
ga_fitness_t fitness_psnr(img_image_t original, img_image_t filtered)
{
    assert(original->width == filtered->width);
    assert(original->height == filtered->height);
    assert(original->comp == filtered->comp);

    double coef = fitness_psnr_coeficient(original->width * original->height);
    double sum = 0;

    for (int x = 0; x < original->width; x++) {
        for (int y = 0; y < original->height; y++) {
            double diff = img_get_pixel(filtered, x, y) - img_get_pixel(original, x, y);
            sum += diff * diff;
        }
    }

    return coef / sum;
}
