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
  double filterbankOutputL = 0.;
  double filterbankOutputR = 0.;
  
  Bandpass bandPass25;
  double filter25R, filter25L;
  Bandpass bandPass50;
  double filter50R, filter50L;
  Bandpass bandPass100;
  double filter100R, filter100L;
  Bandpass bandPass200;
  double filter200R, filter200L;
  Bandpass bandPass400;
  double filter400R, filter400L;
  Bandpass bandPass800;
  double filter800R, filter800L;
  Bandpass bandPass1600;
  double filter1600R, filter1600L;
  Bandpass bandPass3200;
  double filter3200R, filter3200L;
  Bandpass bandPass6400;
  double filter6400R, filter6400L;
  Bandpass bandPass12800;
  double filter12800R, filter12800L;
};

#endif

