#ifndef __MULTIBANDMATCHEQ__
#define __MULTIBANDMATCHEQ__

#include "IPlug_include_in_plug_hdr.h"
#include "FFTRect.h"
#include "bandpass.h"

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
  double mAmount;
  double offSet = 0.;
  double audioAverage;
  bool mSwitch = false;
  bool mSwitchb = false;
  Spect_FFT * sFFT;
  gFFTAnalyzer * gFFTlyzer;
  gFFTAnalyzer * sourceSpectrum;
  gFFTAnalyzer * targetSpectrum;
  gFFTAnalyzer * matchingCurve;
  gFFTFreqDraw * gFFTFreqLines;
  
  const double maxF = 20000.;
  const double minF = 20.;
  const int fftSize = 4096;
  int row = 0;
  double sum = 0.;
  int maxSize = 250;
  double denominator = 0.;
  double deOctGained = 0.;
  
  Bandpass mBandPass;
  double filterOutR, filterOutL;
};

#endif

