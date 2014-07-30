/**
 * Tests CGP initialization.
 * Compile with -DTEST_RANDOMIZE
 */


#include "../cgp.h"


int main(int argc, char const *argv[])
{
    cgp_init();
    cgp_chr chr = cgp_create_chr();
    cgp_destroy_chr(chr);
    cgp_deinit();
}
