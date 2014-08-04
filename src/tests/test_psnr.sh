gcc -g -Wall -std=c11 ../fitness.c ../image.c ../cgp.c test_psnr.c -o test_psnr
./test_psnr
rm test_psnr
