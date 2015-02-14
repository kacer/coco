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

#include <assert.h>

#include "../fitness.h"
#include "../cgp/cgp_sse.h"


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
 * @param  valid_pixels_count How many pixels to process
 * @return
 */
double _fitness_get_sqdiffsum_sse(
    img_pixel_t *original,
    img_pixel_t *noisy,
    int row_length,
    ga_chr_t chr,
    int offset,
    int valid_pixels_count)
{
    img_pixel_t outputs[CGP_OUTPUTS * SSE2_BLOCK_SIZE];
    bool should_restart;
    int block_size = cgp_sse_get_output(chr, noisy, offset, outputs, row_length, &should_restart);
    assert(valid_pixels_count <= block_size);

    double sum = 0;
    for (int i = 0; i < valid_pixels_count; i++) {
        img_pixel_t filtered = cgp_sse_extract_output(outputs, 0, i);
        int diff = filtered - original[offset + i];
        sum += diff * diff;
    }
    return sum;
}

