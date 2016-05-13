#include "MultibandMatchEQ.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include <vector>

const int kNumPrograms = 1;

// matrix 250 x 2049
std::vector<std::vector<double> > matrix(500, std::vector<double>(2049, 0.));
std::vector<double> averageVector(2049, 0.);

enum EParams
{
  kGain = 0,
  kISwitchControl_2,
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
  
  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachPanelBackground(&COLOR_WHITE);
  
  IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  IBitmap bitmap = pGraphics->LoadIBitmap(ISWITCHCONTROL_2_ID, ISWITCHCONTROL_2_FN, kISwitchControl_2_N);
  
  pGraphics->AttachControl(new IKnobMultiControl(this, kGainX, kGainY, kGain, &knob));
  pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_2_X, kISwitchControl_2_Y, kISwitchControl_2, &bitmap));
  
  // IRECT for graphical display
  IRECT iView(80, 20, 80 + 510, 20 + 560);
  // adding the graphical fft spectrum display
  gFFTlyzer = new gFFTAnalyzer(this, iView, COLOR_GRAY, -1, fftSize, false);
  pGraphics->AttachControl(gFFTlyzer);
  gFFTlyzer->SetdbFloor(-60.);
  gFFTlyzer->SetColors(COLOR_GRAY, COLOR_BLACK);
  
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
  const double maxF = 20000.;
  const double minF = 20.;
  gFFTlyzer->SetMaxFreq(maxF);
  gFFTFreqLines->SetMaxFreq(maxF);
  gFFTlyzer->SetMinFreq(minF);
  gFFTFreqLines->SetMinFreq(minF);
  //setting +3dB/octave compensation to the fft display
  gFFTlyzer->SetOctaveGain(3., true);
  
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
  
  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2) {
    *out1 = *in1 * mGain;
    *out2 = *in2 * mGain;
    //send average to FFT class
    audioAverage = (*out1 + *out2) * 0.5;
    sFFT->SendInput(audioAverage);
  }
  
  if (GetGUI()) {
    // send fft data for spectrum display
    const double sr = this->GetSampleRate();
    // start / stop freeze
    if (mSwitch) {
      // for every column
      for (int c = 0; c < fftSize / 2 + 1; c++) {
        // draw average value
        gFFTlyzer->SendFFT(averageVector[c], c, sr);
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
          if (row < (maxSize - 1)) {
            row++;
          } else {
            row = 0;
          }
          if (denominator < (maxSize - 1)) {
            denominator++;
          }
        } else {
          matrix[row][c] = sFFT->GetOutput(c);
        }
      }
    } else {
      for (int c = 0; c < fftSize / 2 + 1; c++) {
        gFFTlyzer->SendFFT(sFFT->GetOutput(c), c, sr);
      }
      // reset
      row = 0;
      denominator = 0.;
    }
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
      
    default:
      break;
  }
}
