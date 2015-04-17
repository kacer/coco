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


#if defined(_ISOC11_SOURCE)
    typedef __m256i __m256i_aligned alignas(32);
#elif __GNUC__ || __IBMC__ || __IBMCPP__ || 0x5110 <= __SUNPRO_C
    typedef __m256i __m256i_aligned __attribute__ ((aligned (32)));
#endif

/**
 * Calculate output of given chromosome and inputs using AVX instructions
 * @param chr
 * @param inputs
 * @param outputs
 */
void cgp_get_output_avx(ga_chr_t chromosome, __m256i_aligned inputs[CGP_INPUTS], __m256i_aligned outputs[CGP_OUTPUTS]);
