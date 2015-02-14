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


#pragma once


#include "../ga.h"
#include "image.h"


static const int FITNESS_SSE2_STEP = 16;
static const int FITNESS_AVX2_STEP = 32;


/**
 * SIMD fitness evaluator prototype
 */
typedef double (*fitness_simd_func_t)(
    img_pixel_t *original,
    img_pixel_t *noisy[WINDOW_SIZE],
    ga_chr_t chr,
    int offset,
    int valid_pixels_count);


/**
 * Calculates difference between original and filtered pixel using SSE2
 * instructions.
 *
 * One call equals 16 CGP evaluations.
 *
 * @param  original_image
 * @param  noisy_image_simd
 * @param  chr
 * @param  offset Where to start in arrays
 * @param  block_size How many pixels to process
 * @return
 */
double _fitness_get_sqdiffsum_sse(
    img_pixel_t *original,
    img_pixel_t *noisy[WINDOW_SIZE],
    ga_chr_t chr,
    int offset,
    int block_size);


/**
 * Calculates difference between original and filtered pixel using AVX2
 * instructions.
 *
 * One call equals 32 CGP evaluations.
 *
 * @param  original_image
 * @param  noisy_image_simd
 * @param  chr
 * @param  offset Where to start in arrays
 * @param  block_size How many pixels to process
 * @return
 */
double _fitness_get_sqdiffsum_avx(
    img_pixel_t *original,
    img_pixel_t *noisy[WINDOW_SIZE],
    ga_chr_t chr,
    int offset,
    int block_size);


/**
 * For testing purposes only
 */
void fitness_test_init(img_image_t original_image,
    img_window_array_t noisy_image_windows,
    img_pixel_t *noisy_image_simd[WINDOW_SIZE]);
