gcc -g -Wall -std=c11 -DTEST_INIT ../cgp.c test_init.c -o test_init
./test_init | diff - test_init.out
rm test_init
