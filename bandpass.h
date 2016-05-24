#ifndef __MultibandMatchEQ__bandpass__
#define __MultibandMatchEQ__bandpass__

#include <stdio.h>

class Bandpass {
public:
    void calculateCoefficients(double Q, double Freq);
    void initialize();
    void processSamples(double inputbuffer1, double inputbuffer2, double &outputbuffer1, double &outputbuffer2);
    
protected:
    double a0, a1, a2, b1, b2;
    double Freq, Q, norm, K;
    double z1L, z2L, z1R, z2R;
};

#endif /* defined(__MultibandMatchEQ__bandpass__) */
