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


#pragma once

#include "cgp_core.h"


/**
 * Calculate output of given chromosome and inputs using AVX instructions
 * @param chr
 * @param inputs
 * @param outputs
 */
void cgp_get_output_avx(ga_chr_t chromosome, unsigned char _inputs[32][CGP_INPUTS], unsigned char _outputs[32][CGP_OUTPUTS]);
