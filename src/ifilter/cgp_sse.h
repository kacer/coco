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
    typedef __m128i __m128i_aligned alignas(16);
#elif __GNUC__ || __IBMC__ || __IBMCPP__ || 0x5110 <= __SUNPRO_C
    typedef __m128i __m128i_aligned __attribute__ ((aligned (16)));
#endif


/**
 * Calculate output of given chromosome and inputs using SSE instructions
 * @param chr
 * @param inputs
 * @param outputs
 */
void cgp_get_output_sse(ga_chr_t chromosome,
    __m128i_aligned inputs[CGP_INPUTS], __m128i_aligned outputs[CGP_OUTPUTS]);
