#ifndef __MULTIBANDMATCHEQ__
#define __MULTIBANDMATCHEQ__

#include "IPlug_include_in_plug_hdr.h"
#include "FFTRect.h"
#include "bandpass.h"
#include "math.h"

class MultibandMatchEQ : public IPlug
{
public:
  MultibandMatchEQ(IPlugInstanceInfo instanceInfo);
  ~MultibandMatchEQ();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  
  // -------------------------------------
  
  void LogAverages(int minBandwidth, int bandsPerOctave) {
    float nyq = GetSampleRate() / 2.;
    octaves = 1;
    while ((nyq /= 2) > minBandwidth)
    {
      octaves++;
    }
    avgPerOctave = bandsPerOctave;
    averages = octaves * bandsPerOctave;
  }
  
  float GetAverageCenterFrequency(int i) {
    // which "octave" is this index in?
    int octave = i / avgPerOctave;
    // which band within that octave is this?
    int offset = i % avgPerOctave;
    float lowFreq, hiFreq, freqStep;
    // figure out the low frequency for this octave
    if (octave == 0) {
      lowFreq = 0;
    } else {
      lowFreq = (GetSampleRate() / 2) / pow(2, octaves - octave);
    }
    // and the high frequency for this octave
    hiFreq = (GetSampleRate() / 2) / pow(2, octaves - octave - 1);
    // each average band within the octave will be this big
    freqStep = (hiFreq - lowFreq) / avgPerOctave;
    // figure out the low frequency of the band we care about
    float f = lowFreq + offset*freqStep;
    // the center of the band will be the low plus half the width
    return f + freqStep/2;
  }
  
  float GetAverageBandWidth(int averageIndex) {
    // which "octave" is this index in?
    int octave = averageIndex / avgPerOctave;
    float lowFreq, hiFreq, freqStep;
    // figure out the low frequency for this octave
    if (octave == 0) {
      lowFreq = 0;
    } else {
      lowFreq = (GetSampleRate() / 2) / pow(2, octaves - octave);
    }
    // and the high frequency for this octave
    hiFreq = (GetSampleRate() / 2) / pow(2, octaves - octave - 1);
    // each average band within the octave will be this big
    freqStep = (hiFreq - lowFreq) / avgPerOctave;
    
    return freqStep;
  }
  
  int FrequencyToIndex(float freq) {
    // special case: freq is lower than the bandwidth of spectrum[0]
    if (freq < bandWidth / 2) return 0;
    // special case: freq is within the bandwidth of spectrum[spectrum.length - 1]
    if (freq > GetSampleRate() / 2 - bandWidth / 2) return fftSize/2 + 1;
    // all other cases
    float fraction = freq / GetSampleRate();
    int i = round(fftSize * fraction);
    return i;
  }

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
  
  const double maxF = GetSampleRate() / 2.;
  const double minF = 20.;
  const int fftSize = 4096;
  int row = 0;
  double sum = 0.;
  int maxSize = 250;
  double denominator = 0.;
  double deOctGained = 0.;
  double filterbankOutputL = 0.;
  double filterbankOutputR = 0.;
  double tempSum = 0.;
  double tempDenominator = 0.;
  double tempAverage = 0.;
  double tempGain = 1.;
  int tempLowIndex, tempHighIndex;
  
  int octaves, avgPerOctave;
  float averages;
  float bandWidth = (2. / fftSize) * (GetSampleRate() / 2.);
  float centerfrequency = 0.;
  double bandGain = 1.;
};

#endif

