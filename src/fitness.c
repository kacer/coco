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

#include "fitness.h"


img_image original_image;
img_window_array noisy_image_windows;


/**
 * Initializes fitness module - prepares test image
 * @param original
 * @param noisy
 */
void fitness_init(img_image original, img_image noisy)
{
    assert(original->width == noisy->width);
    assert(original->height == noisy->height);
    assert(original->comp == noisy->comp);

    original_image = original;
    noisy_image_windows = img_split_windows(noisy);
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
 * the filtered image.
 *
 * @param  chr
 * @return fitness value
 */
img_image fitness_filter_image(cgp_chr chr, img_image noisy)
{
    img_image filtered = img_create(original_image->width, original_image->height,
        original_image->comp);

    for (int i = 0; i < noisy_image_windows->size; i++) {
        img_window *w = &noisy_image_windows->windows[i];

        cgp_value_t *inputs = w->pixels;
        cgp_value_t output_pixel[1];
        cgp_get_output(chr, inputs, output_pixel);

        img_set_pixel(filtered, w->pos_x, w->pos_y, output_pixel[0]);
    }

    return filtered;
}


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
cgp_fitness_t fitness_eval_cgp(cgp_chr chr)
{
    img_image filtered = fitness_filter_image(chr, original_image);
    cgp_fitness_t fitness = fitness_psnr(original_image, filtered);
    img_destroy(filtered);

    return fitness;
}


/**
 * Calculates fitness using the PSNR (peak signal-to-noise ratio) function.
 * The higher the value, the better the filter.
 *
 * @param  original image
 * @param  filtered image
 * @return fitness value (PSNR)
 */
cgp_fitness_t fitness_psnr(img_image original, img_image filtered)
{
    assert(original->width == filtered->width);
    assert(original->height == filtered->height);
    assert(original->comp == filtered->comp);

    double coef = (255 * 255) / ((double)original->width * original->height);
    double sum = 0;

    for (int x = 0; x < original->width; x++) {
        for (int y = 0; y < original->height; y++) {
            double diff = img_get_pixel(filtered, x, y) - img_get_pixel(original, x, y);
            sum += diff * diff;
        }
    }

    return coef / sum;
}
