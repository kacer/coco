gcc -g -Wall -std=c11 -DTEST_REPLACE ../cgp.c ../cgp_dump.c test_replace.c -o test_replace
./test_replace | diff - test_replace.out
rm test_replace
