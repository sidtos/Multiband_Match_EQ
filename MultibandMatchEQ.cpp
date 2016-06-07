#include "MultibandMatchEQ.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include <vector>
#include <numeric>

const int kNumPrograms = 1;

// matrix 250 x 2049
std::vector<std::vector<double> > matrix(250, std::vector<double>(2049, 0.));
// source signal average
std::vector<double> averageVector(2049, 0.);
// target signal average
std::vector<double> targetVector(2049, 0.);
// difference between source and target
std::vector<double> matchingVector(2049, 0.);

enum EParams
{
  kGain = 0,
  kISwitchControl_2,
  kISwitchControl_2b,
  kAmount,
  kAmount2,
  kAmount3,
  kNumParams
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,
  
  kGainX = 14,
  kGainY = 40,
  kISwitchControl_2_X = 28,
  kISwitchControl_2_Y = 138,
  kISwitchControl_2_N = 2,
  kISwitchControl_2b_X = 28,
  kISwitchControl_2b_Y = 222,
  kISwitchControl_2b_N = 2,
  kAmountX = 14,
  kAmountY = 318,
  kAmount2X = 14,
  kAmount2Y = 412,
  kAmount3X = 14,
  kAmount3Y = 506,
  kKnobFrames = 60
};

MultibandMatchEQ::MultibandMatchEQ(IPlugInstanceInfo instanceInfo)
:	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;
  
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 0., -24., 24., 0.01, "dB");
  GetParam(kGain)->SetShape(2.);
  GetParam(kISwitchControl_2)->InitBool("ISwitchControl 2 image multi", 0, "images");
  GetParam(kISwitchControl_2b)->InitBool("ISwitchControl 2 image multi", 0, "images");
  GetParam(kAmount)->InitDouble("Amount", 0., 0.001, 1., 0.001, "%");
  GetParam(kAmount)->SetShape(2.);
  GetParam(kAmount2)->InitDouble("Amount2", 0., 0.001, 1., 0.001, "%");
  GetParam(kAmount2)->SetShape(2.);
  GetParam(kAmount3)->InitDouble("Amount3", 0., 0.001, 1., 0.001, "%");
  GetParam(kAmount3)->SetShape(2.);
  
  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  //pGraphics->AttachPanelBackground(&COLOR_BLACK);
  pGraphics->AttachBackground(BG_ID, BG_FN);
  
  IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  IBitmap bitmap = pGraphics->LoadIBitmap(ISWITCHCONTROL_2_ID, ISWITCHCONTROL_2_FN, kISwitchControl_2_N);
  
  pGraphics->AttachControl(new IKnobMultiControl(this, kGainX, kGainY, kGain, &knob));
  pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_2_X, kISwitchControl_2_Y, kISwitchControl_2, &bitmap));
  pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_2b_X, kISwitchControl_2b_Y, kISwitchControl_2b, &bitmap));
  pGraphics->AttachControl(new IKnobMultiControl(this, kAmountX, kAmountY, kAmount, &knob));
  pGraphics->AttachControl(new IKnobMultiControl(this, kAmount2X, kAmount2Y, kAmount2, &knob));
  pGraphics->AttachControl(new IKnobMultiControl(this, kAmount3X, kAmount3Y, kAmount3, &knob));
  
  // IRECT for graphical display
  // (mRECT.L, mRECT.T, .,mRECT.B)
  IRECT iView(80, 20, 80 + 510, 20 + 560);
  // adding the graphical fft spectrum display
  gFFTlyzer = new gFFTAnalyzer(this, iView, COLOR_GRAY, -1, fftSize, false);
  sourceSpectrum = new gFFTAnalyzer(this, iView, COLOR_GRAY, -1, fftSize, false);
  targetSpectrum = new gFFTAnalyzer(this, iView, COLOR_GRAY, -1, fftSize, false);
  matchingCurve = new gFFTAnalyzer(this, iView, COLOR_GRAY, -1, fftSize, false);
  pGraphics->AttachControl(gFFTlyzer);
  pGraphics->AttachControl(sourceSpectrum);
  pGraphics->AttachControl(targetSpectrum);
  pGraphics->AttachControl(matchingCurve);
  gFFTlyzer->SetdbFloor(-60.);
  sourceSpectrum->SetdbFloor(-60.);
  targetSpectrum->SetdbFloor(-60.);
  matchingCurve->SetdbFloor(-60.);
  gFFTlyzer->SetColors(COLOR_GRAY, COLOR_GRAY, 1);
  sourceSpectrum->SetColors(COLOR_BLACK, COLOR_BLACK, 0);
  targetSpectrum->SetColors(COLOR_GRAY, COLOR_WHITE, 0);
  matchingCurve->SetColors(COLOR_GRAY, COLOR_YELLOW, 0);
  
#ifdef OS_OSX
  char* fontName = "Futura";
  IText::EQuality texttype = IText::kQualityAntiAliased;
#else
  char* fontName = "Calibri";
  IText::EQuality texttype = IText::EQuality::kQualityClearType;
  
#endif
  IText lFont(12, &COLOR_GRAY, fontName, IText::kStyleNormal, IText::kAlignCenter, 0, texttype);
  // adding the vertical frequency lines
  gFFTFreqLines = new gFFTFreqDraw(this, iView, COLOR_GRAY, &lFont);
  pGraphics->AttachControl(gFFTFreqLines);
  
  //setting the min/max freq for fft display and freq lines
  gFFTlyzer->SetMaxFreq(maxF);
  sourceSpectrum->SetMaxFreq(maxF);
  targetSpectrum->SetMaxFreq(maxF);
  matchingCurve->SetMaxFreq(maxF);
  gFFTFreqLines->SetMaxFreq(maxF);
  gFFTlyzer->SetMinFreq(minF);
  sourceSpectrum->SetMinFreq(minF);
  targetSpectrum->SetMinFreq(minF);
  matchingCurve->SetMinFreq(minF);
  gFFTFreqLines->SetMinFreq(minF);
  
  //setting +3dB/octave compensation to the fft display
  gFFTlyzer->SetOctaveGain(0., true);
  sourceSpectrum->SetOctaveGain(0., true);
  targetSpectrum->SetOctaveGain(0., true);
  matchingCurve->SetOctaveGain(0., true);
  
  AttachGraphics(pGraphics);
  
  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
  
  //adding new FFT class with size and overlap, and setting the window function
  sFFT = new Spect_FFT(this, fftSize, 2);
  sFFT->SetWindowType(Spect_FFT::win_BlackmanHarris);
}

MultibandMatchEQ::~MultibandMatchEQ() {}

void MultibandMatchEQ::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];
  
  // calculates the number of bands
  LogAverages(22, 3);
  // vector of filters
  std::vector<Bandpass> filterBank(averages, Bandpass());
  // stores the output of each filter
  std::vector<std::vector<double> > outputStorage(2, std::vector<double>(averages, 0.));
  // stores the low and the high index of each filter
  std::vector<std::vector<double> > indexStorage(2, std::vector<double>(averages, 0.));
  // stores the gain value that must be applied to match the 2 signals
  std::vector<double> gainStorage(averages, 1.);
  
  for (int i = 0; i < averages; i++) {
    centerfrequency = GetAverageCenterFrequency(i);
    //std::cout << centerfrequency << std::endl;
    float averageWidth = GetAverageBandWidth(i);
    float lowFreq  = centerfrequency - averageWidth/2;
    //std::cout << "Low frequency of band " << i+1 << " in Hz: " << lowFreq << std::endl;
    float highFreq = centerfrequency + averageWidth/2;
    //std::cout << "High frequency of band " << i+1 << " in Hz: " << highFreq << std::endl;
    double qValue = centerfrequency/(highFreq - lowFreq);
    //std::cout << "Q of band " << i+1 << " is: " << highFreq << std::endl;
    indexStorage[0][i] = FrequencyToIndex(lowFreq);
    //std::cout << "Band " << i+1 << " low frequency index: " << xl << std::endl;
    indexStorage[1][i] = FrequencyToIndex(highFreq);
    //std::cout << "Band " << i+1 << " high frequency index: " << xr << std::endl;
    filterBank[i] = Bandpass(qValue, centerfrequency);
    //std::cout << "Band " << i+1 << " size: " << filterBank.size() << std::endl;
  }
  
  if (GetGUI()) {
    // send fft data for spectrum display
    const double sr = this->GetSampleRate();
    
    // when nothing is selected
    if (mSwitch == FALSE && mSwitchb == FALSE) {
      // for every column
      for (int c = 0; c < fftSize / 2 + 1; c++) {
        // visualize audio in realtime
        gFFTlyzer->SendFFT(sFFT->GetOutput(c), c, sr);
        // calculate matching curve
        if (c <= 17) {
          // matching band 1: 0-17
          matchingVector[c] = mAmount * (targetVector[c] - averageVector[c]);
        } else if (c <= 180) {
          // matching band 2: 18-180
          matchingVector[c] = mAmount2 * (targetVector[c] - averageVector[c]);
        } else {
          // matching band 3: 181 - 2049
          matchingVector[c] = mAmount3 * (targetVector[c] - averageVector[c]);
        }
        // draw matching curve at highest value of the source
        matchingCurve->SendFFT((matchingVector[c] + offSet), c, sr);
      }
      // reset
      row = 0;
      denominator = 0.;
      for (int i = 0; i < averages; i++) {
        tempSum = tempDenominator = tempAverage = 0.;
        tempGain = 1.;
        tempLowIndex = indexStorage[0][i];
        tempHighIndex = indexStorage[1][i];
        for (int k = tempLowIndex; k <= tempHighIndex; k++) {
          tempSum += 135 * matchingVector[k];
          tempDenominator++;
          tempAverage = tempSum / tempDenominator;
        }
        // avoid negative gain values
        if (tempAverage < -1.) {
          tempAverage = -1.;
        }
        gainStorage[i] = 1. + tempAverage;
      }
      
    } // nothing selected
    
    // start / stop source
    if (mSwitch) {
      // for every column
      for (int c = 0; c < fftSize / 2 + 1; c++) {
        // draw average value
        gFFTlyzer->SendFFT(averageVector[c], c, sr);
        sourceSpectrum->SendFFT(averageVector[c], c, sr);
        // update peak of original signal
        if (averageVector[c] > offSet) {
          offSet = averageVector[c];
        }
        // at the end of the row
        if (c == fftSize / 2) {
          // for every column
          for (int x = 0; x < matrix[row].size(); x++) {
            // reset
            sum = 0.;
            // for every row
            for (int y = 0; y < denominator + 1; y++) {
              // sum all values of the recent column
              sum += matrix[y][x];
              // store average per column in vector
              averageVector[x] = sum / double(denominator + 1);
            }
          }
          // increase row number
          if (row < (maxSize - 1)) {
            row++;
          } else {
            // jump back to the begin
            row = 0;
          }
          // increase the amount of the denominator
          if (denominator < (maxSize - 1)) {
            denominator++;
          }
        } else {
          // fill matrix with pre-calculated values
          matrix[row][c] = sFFT->GetOutput(c);
        }
      }
    } // start / stop source
    
    // start / stop target
    if (mSwitchb) {
      // for every column
      for (int c = 0; c < fftSize / 2 + 1; c++) {
        // draw target value
        gFFTlyzer->SendFFT(targetVector[c], c, sr);
        targetSpectrum->SendFFT(targetVector[c], c, sr);
        // at the end of the row
        if (c == fftSize / 2) {
          // for every column
          for (int x = 0; x < matrix[row].size(); x++) {
            // reset
            sum = 0.;
            // for every row
            for (int y = 0; y < denominator + 1; y++) {
              // sum all values of the recent column
              sum += matrix[y][x];
              // store average per column in vector
              targetVector[x] = sum / double(denominator + 1);
            }
          }
          // increase row number
          if (row < (maxSize - 1)) {
            row++;
          } else {
            // jump back to the begin
            row = 0;
          }
          // increase the amount of the denominator
          if (denominator < (maxSize - 1)) {
            denominator++;
          }
        } else {
          // fill matrix with pre-calculated values
          matrix[row][c] = sFFT->GetOutput(c);
        }
      }
    } // start / stop target
    
    // audio output calculation
    for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2) {
      for (int i = 0; i < averages; i++) {
        if (i == 0) {
          filterbankOutputL = 0.;
          filterbankOutputR = 0.;
        }
        filterBank[i].processSamples(*in1, *in2, outputStorage[0][i], outputStorage[1][i]);
        bandGain = gainStorage[i];
        filterbankOutputL += outputStorage[0][i] * bandGain;
        filterbankOutputR += outputStorage[1][i] * bandGain;
      }
      *out1 = filterbankOutputL * mGain;
      *out2 = filterbankOutputR * mGain;
      
      //send average to FFT class
      audioAverage = (*out1 + *out2) * 0.5;
      //audioAverage = (*out1 + *out2) * 0.5;
      sFFT->SendInput(audioAverage);
    } // audio output calculation

  }
}

void MultibandMatchEQ::Reset()
{
  TRACE;
  IMutexLock lock(this);
}

void MultibandMatchEQ::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);
  
  switch (paramIdx)
  {
    case kGain:
      mGain = GetParam(kGain)->DBToAmp();
      break;
      
    case kISwitchControl_2:
      mSwitch = GetParam(kISwitchControl_2)->Bool();
      break;
      
    case kISwitchControl_2b:
      mSwitchb = GetParam(kISwitchControl_2b)->Bool();
      break;
      
    case kAmount:
      mAmount = GetParam(kAmount)->Value();
      break;
      
    case kAmount2:
      mAmount2 = GetParam(kAmount2)->Value();
      break;
      
    case kAmount3:
      mAmount3 = GetParam(kAmount3)->Value();
      break;
      
    default:
      break;
  }
}
