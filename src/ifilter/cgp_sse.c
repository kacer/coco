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

#include "../cgp/cgp_sse.h"


/**
 * Calculates CGP node output value using SSE instructions
 *
 * @param  n CGP node
 * @param  A Input value A
 * @param  B Input value B
 * @param  Y Output value
 * @return Whether CGP function has changed and evaluation should be restarted
 */
__m128i cgp_sse_get_node_output(cgp_node_t *n, __m128i A,
    __m128i B, bool *should_restart)
{
    *should_restart = false;

    register __m128i_aligned FF = _mm_set1_epi8(0xFF);
    register __m128i tmp, tmp2;  // anything
    register __m128i mask; // mask used for shifts

    if (n->is_constant) {
        return _mm_set1_epi8(n->constant_value);
    }

    switch (n->function) {
        case c255:
            return FF;

        case identity:
            return A;

        case inversion:
            return _mm_sub_epi8(FF, A);

        case b_or:
            return _mm_or_si128(A, B);

        case b_not1or2:
            // we don't have NOT instruction, we need to XOR with FF
            tmp = _mm_xor_si128(FF, A);
            return _mm_or_si128(_mm_xor_si128(FF, A), B);

        case b_and:
            return _mm_and_si128(A, B);

        case b_nand:
            tmp = _mm_and_si128(A, B);
            return _mm_xor_si128(FF, tmp);

        case b_xor:
            return _mm_xor_si128(A, B);

        case rshift1:
            // no SR instruction for 8bit data, we need to shift
            // 16 bits and apply mask
            // IN : [ 1 2 3 4 5 6 7 8 | A B C D E F G H]
            // SHR: [ 0 1 2 3 4 5 6 7 | 8 A B C D E F G]
            // MSK: [ 0 1 2 3 4 5 6 7 | 0 A B C D E F G]
            mask = _mm_set1_epi8(0x7F);
            tmp = _mm_srli_epi16(A, 1);
            return _mm_and_si128(tmp, mask);

        case rshift2:
            // similar to rshift1
            // IN : [ 1 2 3 4 5 6 7 8 | A B C D E F G H]
            // SHR: [ 0 0 1 2 3 4 5 6 | 7 8 A B C D E F]
            // MSK: [ 0 0 1 2 3 4 5 6 | 0 0 A B C D E F]
            mask = _mm_set1_epi8(0x3F);
            tmp = _mm_srli_epi16(A, 2);
            return _mm_and_si128(tmp, mask);

        case swap:
            // SWAP(A, B) (((A & 0x0F) << 4) | ((B & 0x0F)))
            // Shift A left by 4 bits
            // IN : [ 1 2 3 4 5 6 7 8 | A B C D E F G H]
            // SHL: [ 5 6 7 8 A B C D | E F G H 0 0 0 0]
            // MSK: [ 5 6 7 8 0 0 0 0 | E F G H 0 0 0 0]
            mask = _mm_set1_epi8(0xF0);
            tmp = _mm_slli_epi16(A, 4);
            tmp = _mm_and_si128(tmp, mask);

            // Mask B
            // IN : [ 1 2 3 4 5 6 7 8 | A B C D E F G H]
            // MSK: [ 0 0 0 0 5 6 7 8 | 0 0 0 0 E F G H]
            mask = _mm_set1_epi8(0x0F);
            tmp2 = _mm_and_si128(B, mask);

            // Combine
            return _mm_or_si128(tmp, tmp2);

        case add:
            return _mm_add_epi8(A, B);

        case add_sat:
            return _mm_adds_epu8(A, B);

        case avg:
            // shift right first, then add, to avoid overflow
            mask = _mm_set1_epi8(0x7F);
            tmp = _mm_srli_epi16(A, 1);
            tmp = _mm_and_si128(tmp, mask);

            tmp2 = _mm_srli_epi16(B, 1);
            tmp2 = _mm_and_si128(tmp2, mask);

            return _mm_add_epi8(tmp, tmp2);

        case max:
            return _mm_max_epu8(A, B);

        case min:
            return _mm_min_epu8(A, B);

        default:
            assert(false);
    }
}
