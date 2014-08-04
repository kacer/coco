/**
 * Tests CGP chromosome replacement.
 */


#include <stdio.h>

#include "../cgp.h"


int main(int argc, char const *argv[])
{
    cgp_init(NULL, minimize);
    cgp_chr chr = cgp_create_chr();
    cgp_chr chr2 = cgp_create_chr();

    cgp_dump_chr_compat(chr, stdout);
    printf("\n\n");
    cgp_dump_chr_compat(chr2, stdout);
    printf("\n\n");

    cgp_replace_chr(chr2, chr);
    cgp_dump_chr_compat(chr2, stdout);
    printf("\n");

    cgp_destroy_chr(chr);
    cgp_destroy_chr(chr2);
    cgp_deinit();
}
