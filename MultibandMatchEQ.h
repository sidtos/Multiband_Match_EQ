#ifndef __MULTIBANDMATCHEQ__
#define __MULTIBANDMATCHEQ__

#include "IPlug_include_in_plug_hdr.h"
#include "FFTRect.h"

class MultibandMatchEQ : public IPlug
{
public:
  MultibandMatchEQ(IPlugInstanceInfo instanceInfo);
  ~MultibandMatchEQ();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  double mGain;
  double audioAverage;
  bool mSwitch = false;
  Spect_FFT * sFFT;
  gFFTAnalyzer * gFFTlyzer;
  gFFTFreqDraw * gFFTFreqLines;
  
  const int fftSize = 4096;
  int row = 0;
  double sum = 0.;
};

#endif

