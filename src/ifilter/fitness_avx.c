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


#include "../fitness.h"
#include "cgp_avx.h"


/**
 * Calculates difference between original and filtered pixel using AVX2
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
double _fitness_get_sqdiffsum_avx(
    img_pixel_t *original,
    img_pixel_t *noisy[WINDOW_SIZE],
    ga_chr_t chr,
    int offset,
    int block_size)
{
    __m256i_aligned avx_inputs[CGP_INPUTS];
    __m256i_aligned avx_outputs[CGP_OUTPUTS];
    unsigned char *outputs_ptr = (unsigned char*) &avx_outputs;

    for (int i = 0; i < CGP_INPUTS; i++) {
        avx_inputs[i] = _mm256_load_si256((__m256i*)(&noisy[i][offset]));
    }

    cgp_get_output_avx(chr, avx_inputs, avx_outputs);

    double sum = 0;
    for (int i = 0; i < block_size; i++) {
        int diff = outputs_ptr[i] - original[offset + i];
        sum += diff * diff;
    }
    return sum;
}

