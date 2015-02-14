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

#include <immintrin.h>

#include "../cgp/cgp_core.h"


static const int SSE2_BLOCK_SIZE = sizeof(__m128i) / sizeof(cgp_value_t);


typedef __m128i __m128i_aligned __attribute__ ((aligned (16)));


/**
 * Calculate output of given chromosome and inputs using SSE instructions
 *
 * The output_data array is in fact two-dimensional and it should be
 * at least (SSE2_BLOCK_SIZE * CGP_OUTPUTS) big.
 *
 * @param chr
 * @param input_data
 * @param input_offset offset from which to copy input data to SSE registers
 * @param output_data will be written here (two-dimensional array!)
 * @return number of items written to output_data
 */
int cgp_get_output_sse(
    ga_chr_t chromosome,
    cgp_value_t *input_data[CGP_INPUTS],
    int input_offset,
    cgp_value_t *output_data);


static inline cgp_value_t cgp_get_output_value_sse(
    cgp_value_t *output_data,
    int output_idx,
    int offset)
{
    return output_data[output_idx * SSE2_BLOCK_SIZE + offset];
}


static inline void cgp_set_output_values_sse(
    cgp_value_t *output_data,
    int output_idx,
    __m128i value)
{
    __m128i *dest = (__m128i*)(&output_data[output_idx * SSE2_BLOCK_SIZE]);
    _mm_store_si128(dest, value);
}
