#include <complex>
#include <iostream>
#include <malloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"

const double PI = 3.141592653589793238460;

typedef std::complex<double> Complex;

void fft(Complex *input, size_t N) {
    if(N <= 1) {
        return;
    }

    Complex *even = (Complex *)malloc((N / 2) * sizeof(Complex));
    Complex *odd = (Complex *)malloc((N / 2) * sizeof(Complex));

    for(int i = 0; i < N / 2; i++) {
        even[i] = input[2 * i];
        odd[i] = input[2 * i + 1];
    }

    fft(even, N / 2);
    fft(odd, N / 2);

    for(size_t i = 0; i < N / 2; i++) {
        Complex t = std::polar(1.0, -2 * PI * i / N) * odd[i];
        input[i] = even[i] + t;
        input[i + N / 2] = even[i] - t;
    }
}

void inverse_fft(Complex *input, size_t N) {
    for(size_t i = 0; i < N; i++) {
        input[i] = std::conj(input[i]);
    }

    fft(input, N);

    for(size_t i = 0; i < N; i++) {
        input[i] = std::conj(input[i]);
        input[i] /= N;
    }
}

unsigned int nextPOT(unsigned int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

int main() {
    int w = 0;
    int h = 0;
    int c = 0;

    unsigned char *image = stbi_load("input.jpg", &w, &h, &c, STBI_rgb);
    std::cout << w << "x" << h << ", " << c << " components" << std::endl;

    const int size = w * h * c;
    const int padded_size = nextPOT(size);

    Complex *temp = (Complex *)calloc(padded_size,  sizeof(Complex));

    for(size_t i = 0; i < size; i++) {
        temp[i] = image[i];
    }

    fft(temp, padded_size);
    inverse_fft(temp, padded_size);

    for(int i = 0; i < size; i++) {
        image[i] = (unsigned char)temp[i].real();
    }

    tje_encode_to_file("output.jpg", w, h, c, image);

    return 0;
}
