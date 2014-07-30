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


#include <stdio.h>

#include "cgp.h"
#include "random.h"

int main(int argc, char const *argv[])
{
    rand_init();
    cgp_init();
    cgp_chr chr = cgp_create_chr();
    cgp_dump_chr(chr, stdout, asciiart);
    putchar('\n');
    cgp_dump_chr(chr, stdout, readable);
    putchar('\n');
    cgp_dump_chr(chr, stdout, compat);
    putchar('\n');
    cgp_destroy_chr(chr);

    /* code */
    cgp_deinit();
    return 0;
}
