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


/*
    CPU new instruction set check.
    Source: https://software.intel.com/en-us/articles/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family
 */


#pragma once


#include <cpuid.h>


/**
 * Checks whether current CPU supports AVX2 and other New Haswell features
 */
int can_use_intel_core_4th_gen_features();


/**
 * Checks whether current CPU supports SSE4.1 instruction set
 */
int can_use_sse4_1();


/**
 * Checks whether current CPU supports SSE2 instruction set
 */
int can_use_sse2();
