#include <complex>
#include <iostream>
#include <malloc.h>
#include <windows.h>

double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter() {
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    PCFreq = double(li.QuadPart) / 1000.0;
    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}

double GetCounter() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart - CounterStart) / PCFreq;
}

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

    free(even);
    free(odd);
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

void separable_fft(Complex **data, uint32_t width, uint32_t height, bool forward) {
    for(uint32_t i = 0; i < height; i++) {
        Complex *row = (Complex *)malloc(width * sizeof(Complex));

        for(uint32_t j = 0; j < width; j++) {
            row[j] = data[i][j];
        }

        if(forward == true) {
            fft(row, height);
        } else {
            inverse_fft(row, height);
        }

        for(uint32_t j = 0; j < width; j++) {
            data[i][j] = row[j];
        }

        free(row);
    }

    for(uint32_t j = 0; j < width; j++) {
        Complex *column = (Complex *)malloc(height * sizeof(Complex));

        for(uint32_t i = 0; i < height; i++) {
            column[i] = data[i][j];
        }

        if(forward == true) {
            fft(column, width);
        } else {
            inverse_fft(column, width);
        }

        for(uint32_t i = 0; i < height; i++) {
            data[i][j] = column[i];
        }

        free(column);
    }
}

uint8_t **read(const char *filename, char *magic, uint32_t *w, uint32_t *h, uint32_t *d) {
    FILE *f = fopen(filename, "rb");

    if(f == NULL) {
        return NULL;
    }

    fscanf(f, "%2s\n%d %d\n%d\n", magic, w, h, d);

    if(magic[0] != 'P' && magic[1] != '5') {
        return NULL;
    }

    uint8_t *raw = (uint8_t *)malloc((*w) * (*h) * sizeof(uint8_t));
    fread(raw, 1, (*w) * (*h), f);
    fclose(f);

    uint8_t **data = (uint8_t **)malloc((*h) * sizeof(uint8_t *));

    for(uint32_t i = 0; i < *h; i++) {
        data[i] = (uint8_t *)malloc((*w) * sizeof(uint8_t));

        for(uint32_t j = 0; j < *w; j++) {
            data[i][j] = raw[i * *w + j];
        }
    }

    return data;
}

bool write(const char *filename, char *magic, uint32_t w, uint32_t h, uint32_t d, uint8_t **data) {
    FILE *f = fopen(filename, "wb");

    if(f == NULL) {
        return false;
    }

    fprintf(f, "%2s\n%d %d\n%d\n", magic, w, h, d);

    uint8_t *raw = (uint8_t *)malloc(w * h * sizeof(uint8_t));

    for(uint32_t i = 0; i < h; i++) {
        for(uint32_t j = 0; j < w; j++) {
            raw[i * w + j] = data[i][j];
        }
    }

    fwrite(raw, 1, w * h, f);
    fclose(f);

    return true;
}

uint32_t nextPOT(uint32_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

int main(int argc, char **argv) {
    if(argc < 2) {
        std::cerr << "Missing input filename." << std::endl;
        return 1;
    }

    char magic[2];
    uint32_t w, h, d;
    uint8_t **data = read(argv[1], magic, &w, &h, &d);

    double kdata[9][9] = {
        {4, 3, 2, 1, 0, -1, -2, -3, -4},
        {5, 4, 3, 2, 0, -2, -3, -4, -5},
        {6, 5, 4, 3, 0, -3, -4, -5, -6},
        {7, 6, 5, 4, 0, -4, -5, -6, -7},
        {8, 7, 6, 5, 0, -5, -6, -7, -8},
        {7, 6, 5, 4, 0, -4, -5, -6, -7},
        {6, 5, 4, 3, 0, -3, -4, -5, -6},
        {5, 4, 3, 2, 0, -2, -3, -4, -5},
        {4, 3, 2, 1, 0, -1, -2, -3, -4},
    };

    uint32_t kw = sizeof(kdata) / sizeof(kdata[0]);
    uint32_t kh = sizeof(kdata[0]) / sizeof(kdata[0][0]);

    uint32_t width = nextPOT(w + kw - 1);
    uint32_t height = nextPOT(h + kh - 1);

    Complex **image = (Complex **)calloc(height, sizeof(Complex *));
    Complex **kernel = (Complex **)calloc(height, sizeof(Complex *));

    for(uint32_t i = 0; i < height; i++) {
        image[i] = (Complex *)malloc(width * sizeof(Complex));
        kernel[i] = (Complex *)malloc(width * sizeof(Complex));
    }

    for(uint32_t i = 0; i < h; i++) {
        for(uint32_t j = 0; j < w; j++) {
            image[i][j] = data[i][j];
        }
    }

    for(uint32_t i = 0; i < kh; i++) {
        for(uint32_t j = 0; j < kw; j++) {
            kernel[i][j] = kdata[i][j];
        }
    }

    StartCounter();
    separable_fft(image, width, height, true);
    std::cout << GetCounter() << std::endl;

    StartCounter();
    separable_fft(kernel, width, height, true);
    std::cout << GetCounter() << std::endl;

    for(uint32_t i = 0; i < height; i++) {
        for(uint32_t j = 0; j < width; j++) {
            image[i][j] *= kernel[i][j];
        }
    }

    for(uint32_t i = 0; i < h; i++) {
        free(kernel[i]);
    }
    free(kernel);

    StartCounter();
    separable_fft(image, width, height, false);
    std::cout << GetCounter() << std::endl;

    for(uint32_t i = 0; i < h; i++) {
        for(uint32_t j = 0; j < w; j++) {
            data[i][j] = (uint8_t)image[i][j].real();
        }
    }

    write("output.pgm", magic, w, h, d, data);

    return 0;
}
