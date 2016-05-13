#include "MultibandMatchEQ.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include <vector>

const int kNumPrograms = 1;

std::vector<std::vector<double> > matrix(1, std::vector<double>(2049, 0.));
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
  
  //Backup
  /*if (GetGUI()) {
    // send fft data for spectrum display
    const double sr = this->GetSampleRate();
    for (int c = 0; c < fftSize / 2 + 1; c++) {
      gFFTlyzer->SendFFT(sFFT->GetOutput(c), c, sr);
    }
  }*/
  
  if (GetGUI()) {
    // send fft data for spectrum display
    const double sr = this->GetSampleRate();
    int averageLimit = 250;
    
    if (mSwitch) {
      //std::cout << "check";
      for (int c = 0; c < fftSize / 2 + 1; c++) {
        gFFTlyzer->SendFFT(averageVector[c], c, sr);
        
        if (c == fftSize / 2) {
          // add empty row
          matrix.push_back(std::vector<double>(2049, 0.));
          for (int x = 0; x < matrix[row].size(); x++) {
            sum = 0.;
            for (int y = 0; y < row + 1; y++) {
              sum += matrix[y][x];
              averageVector[x] = sum / double(row + 1);
            }
          }
          std::cout << row << std::endl;
          row++;
        } else {
          matrix[row][c] = sFFT->GetOutput(c);
        }
      }
    } else {
      for (int c = 0; c < fftSize / 2 + 1; c++) {
        gFFTlyzer->SendFFT(sFFT->GetOutput(c), c, sr);
      }
      // resets
      row = 0.;
      matrix.resize(1);
      std::fill(matrix[0].begin(), matrix[0].end(), 0.);
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
