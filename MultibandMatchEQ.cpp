#include "MultibandMatchEQ.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include <vector>

const int kNumPrograms = 1;

// matrix 250 x 2049
std::vector<std::vector<double> > matrix(250, std::vector<double>(2049, 0.));
std::vector<double> averageVector(2049, 0.);
std::vector<double> targetVector(2049, 0.);
std::vector<double> matchingVector(2049, 0.);


enum EParams
{
  kGain = 0,
  kISwitchControl_2,
  kISwitchControl_2b,
  kAmount,
  kNumParams
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,
  
  kGainX = 20,
  kGainY = 20,
  kISwitchControl_2_X = 20,
  kISwitchControl_2_Y = 80,
  kISwitchControl_2_N = 2,
  kISwitchControl_2b_X = 20,
  kISwitchControl_2b_Y = 150,
  kISwitchControl_2b_N = 2,
  kAmountX = 20,
  kAmountY = 210,
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
  
  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachPanelBackground(&COLOR_WHITE);
  
  IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  IBitmap bitmap = pGraphics->LoadIBitmap(ISWITCHCONTROL_2_ID, ISWITCHCONTROL_2_FN, kISwitchControl_2_N);
  
  pGraphics->AttachControl(new IKnobMultiControl(this, kGainX, kGainY, kGain, &knob));
  pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_2_X, kISwitchControl_2_Y, kISwitchControl_2, &bitmap));
  pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_2b_X, kISwitchControl_2b_Y, kISwitchControl_2b, &bitmap));
  pGraphics->AttachControl(new IKnobMultiControl(this, kAmountX, kAmountY, kAmount, &knob));
  
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
  gFFTlyzer->SetColors(COLOR_GRAY, COLOR_BLACK);
  sourceSpectrum->SetColors(COLOR_GRAY, COLOR_RED);
  targetSpectrum->SetColors(COLOR_GRAY, COLOR_BLUE);
  matchingCurve->SetColors(COLOR_GRAY, COLOR_GREEN);
  
#ifdef OS_OSX
  char* fontName = "Futura";
  IText::EQuality texttype = IText::kQualityAntiAliased;
#else
  char* fontName = "Calibri";
  IText::EQuality texttype = IText::EQuality::kQualityClearType;
  
#endif
  IText lFont(12, &COLOR_BLACK, fontName, IText::kStyleNormal, IText::kAlignCenter, 0, texttype);
  // adding the vertical frequency lines
  gFFTFreqLines = new gFFTFreqDraw(this, iView, COLOR_BLACK, &lFont);
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
  
  filterOutL = filterOutR = 0.0;
  mBandPass.calculateCoefficients(23., 100.);
  
  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2) {
    //*out1 = *in1 * mGain;
    //*out2 = *in2 * mGain;
    
    mBandPass.processSamples(*in1, *in2, filterOutL, filterOutR);
    //send average to FFT class
    audioAverage = (filterOutL * mGain + filterOutR * mGain) * 0.5;
    sFFT->SendInput(audioAverage);
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
        matchingVector[c] = mAmount * (targetVector[c] - averageVector[c]) + offSet;
        //matchingVector[c] = 0.0707;
        // draw matching curve at highest value of the source
        matchingCurve->SendFFT(matchingVector[c], c, sr);
      }
      // reset
      row = 0;
      denominator = 0.;
    }
    
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
    }
    
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
    }
  }
}

void MultibandMatchEQ::Reset()
{
  TRACE;
  IMutexLock lock(this);
  mBandPass.initialize();
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
      
    default:
      break;
  }
}
