#include "bandpass.h"
#include <math.h>

void Bandpass::initialize() {
    a0 = a1 = a2 = b1 = b2 = K = Q = norm = z1L = z2L = z1R = z2R = 0.0;
}

void Bandpass::calculateCoefficients(double Q, double Freq) {
    K = tan(M_PI * (Freq / 44100.));
    norm = 1 / (1 + K / Q + K * K);
    a0 = K / Q * norm;
    a1 = 0.;
    a2 = -a0;
    b1 = 2 * (K * K -1) * norm;
    b2 = (1 - K / Q + K * K) * norm;
}

void Bandpass::processSamples(double inputbuffer1, double inputbuffer2, double &outputbuffer1, double &outputbuffer2) {
    outputbuffer1 = inputbuffer1 * a0 + z1L;
    z1L = inputbuffer1 * a1 + z2L - b1 * outputbuffer1;
    z2L = inputbuffer1 * a2 - b2 * outputbuffer1;
    
    outputbuffer2 = inputbuffer2 * a0 + z1R;
    z1R = inputbuffer2 * a1 + z2R - b1 * outputbuffer2;
    z2R = inputbuffer2 * a2 - b2 * outputbuffer2;
}