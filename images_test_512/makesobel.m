
function [ ] = makenoise(N)
    mkdir(int2str(N))

    files = dir('*.png');

    for file = files'
        disp(strcat(file.name, ' --> ', int2str(N), '/', file.name));
        A = imread(file.name);
        B = edge(A, 'sobel');
        imwrite(B, strcat(int2str(N), '/', file.name));

    end
end
