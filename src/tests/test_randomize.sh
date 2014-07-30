gcc -g -Wall -std=c11 -DTEST_RANDOMIZE ../cgp.c test_randomize.c -o test_randomize
./test_randomize | diff - test_randomize.out
rm test_randomize
