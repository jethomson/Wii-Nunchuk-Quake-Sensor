// Chebyshev coefficients for a 4 pole low-pass filter
// with a 3dB freq. of 50 Hz when the sampling rate is 250

// filter length
#define N 5

float b[N] = {0.0272941, 0.109176, 0.163765, 0.109176, 0.0272941};
float a[N] = {1, -1.4094, 1.32638, -0.608985, 0.130899};
