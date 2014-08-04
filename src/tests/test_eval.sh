gcc -g -Wall -std=c11 -DTEST_EVAL ../cgp.c ../cgp_dump.c test_eval.c -o test_eval
./test_eval | diff - test_eval.out
rm test_eval
