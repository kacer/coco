function [ ] = makenoise()

    for N = [5:5:95]
        mkdir(strcat('impulse',  int2str(N)))
        mkdir(int2str(N))

        files = dir('*.png');

        for file = files'   %'
            disp(strcat(file.name, ' --> ', 'impulse',  int2str(N), '/', file.name));
            A = imread(file.name);
            B = impulsenoise(A, N / 100, 1);
            imwrite(B, strcat('impulse', int2str(N), '/', file.name));

            disp(strcat(file.name, ' --> ', int2str(N), '/', file.name));
            B = imnoise(A, 'salt & pepper', N / 100);
            imwrite(B, strcat(int2str(N), '/', file.name));
        end
    end

end
