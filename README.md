#Multiband Match EQ

The Multiband Match EQ is a multiplatform audioplugin written in c++. It basically contains a filterbank that automatically adapts to the frequency spectrum of stored signals. A more visual presentation can be found on my website: http://studenthome.hku.nl/~sid.tossenberger/portfolio/multiband-match-eq/

This repository contains the source code:
* MultibandMatchEQ.cpp
* MultibandMatchEQ.h
* bandpass.cpp
* bandpass.h
* FFTRect.h (FFT class I used for the analyzer)

and documentation files:
* README.md
* eerste_idee.txt
* logbook.txt
* systeemontwerp.pdf

The full documentation can be found in this readme file for better readability.

##Concept

The goal is to code a plugin similar to *iZotope's Matching EQ*, *Fabfilter's EQ Match*, *Logic's Match EQ* and *TBPro Audio's Euphonia*. However, all these plugins do not let the user adjust the amount of matching per band (low, mid and high frequencies).

After doing some research on how the listed plugins work I decided to split the development into 2 main fragments: the analyzer and the equalizer. Once the analyzer is working I am able to store the data in vectors and calculate the curve of the EQ.

##Planning

Since this is an assignment for school I have a total of 7 weeks to develop the first version of the plugin. The planning I did on day one looks as follows:

- [x] 3 weeks: analyzer
- [x] 2 weeks: filterbank
- [x] 1 week: combining analyzer and filterbank
- [x] 1 week: GUI

##Logbook

**21.05.2016**

The development of the plugin can be divided in two parts: FFT calculations and filtering. 
I chose to begin with the spectral half. The following components are implemented and seem 
to work:

- A visual representation of the frequency spectrum, including:
  * the possibility to calculate the average of each frequency band, which is achieved by:
    * filling a two-dimensional vector
    * calculating the average per column
    * sending the average to the GUI
- A start/stop button to fix the spectrum, by:
  * reading the last row of the 2d vector
  * drawing the fixed spectrum
- A 2nd spectrum, that:
  * draws a new spectrum
  * resets the 2d vector
  * calculates the average per column
  * calculates matched curve, by: 
    * subtracting both spectrums 
    * adding a specific offset
  * sends the matched curve to the GUI
- A slider to scale the amount of the matched curve

When trying to draw the matched curve I came across the following problem:
In order to visualize the frequency spectrum in a proper manner, 3dB of octave gain are 
applied. Because the human ear perceives high frequencies to be louder than they really are, 
the spectrum adds 3dB of gain per octave. This ensures that the spectrum looks like the way 
we think to hear it. However, when a horizontal line should be drawn, the octave gain causes 
it to appear diagonally.

A possible solution:
De-activate the octavegain for the matched curve and multiply the vector by another vector 
filled with (for example) y=x^2+1. This must happen before adding the offset.

Note:
If there's time left at the end of the project I will implement the solution. However, 
creating the filter has a higher priority for now.

**23.05.2016**

After explaining my project to Pieter Suurmond I decided to design a x-band filterbank of 
several bandpass filters. Pieter stated that in "Elements of Computer Music" (page 134) by 
Richard Moore, a formula can be found to calculate the filter coefficients and at the same 
time obtain the same bandwidth for every frequency. However, the values I get (for the 
variable R) are not as expected.

In "Writing difference equations for digital filters" by Brian T. Boulter 
(http://www.apicsllc.com/apics/Sr_3/Sr_3.htm) a method is presented to map a butterworth 
low pass filter to a bandpass filter. Yet again I get strange values (for the variable c).

To check if my calculations are correct I use the "Biquad calculator v2" 
(http://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/) by Nigel Redmond. Luckily, 
he provides the source code for his web application which helped me alot to calculate the 
correct coefficients. I implemented a first filter in my program and it seems to work.

The next step would be to split the spectrum in a number of bands (possibly as stated in 
http://stackoverflow.com/questions/10349597/2nd-order-iir-filter-coefficients-for-a-butterworth-bandpass-eq)
and create a bandpass filter for each band.

**30.05.2016**

Several bandpass filters were implemented. Starting at 25 Hz I took twice the value for
the next bandpass resulting in a total of 10 filters. When testing the response of the
filterbank using white noise a subtle comb filter effect can be heard. 

The next step would be to calculate the average volume of the matching curve for each
bandpass. Then, to get rid of the comb filter effect, each filter should be created
dynamically in order to use an unlimited amount of filters.

Problem: the vector of the matching curve is filled with 2049 values. However, as 
frequencies are represented logarithmically, 2049 can't be divided by 10.

Notes:
- Frequencies: 22050
- Vectorsize: 2049
- Number of bands: 10

**04.06.2016**

The minim library for Processing provides an example where the FFT averages are
calculated, both linearly and logarithmically. On github I found the source code
(https://github.com/ddf/Minim/blob/master/src/ddf/minim/analysis/FourierTransform.java)
of their FFT class. After implementing these functions I am able to calculate the
corresponding index of each frequency. Furthermore, I now split the FFT in 10 octaves,
each divided by 3, resulting in a total of 30 bands.

Next steps: 
- link a bandpass filter to the centerfrequency of each band
- calculate the bandwith of the filter using the precalculated values for the low and the
  high frequencies

**05.06.2016**

The bandpass filters are now created dynamically by giving them the bandwidth and the
centerfrequency as parameters. After that, the filters are stored in a vector of objects.
Furthermore, I calculate the average gain per band and store these values in a vector
as well. In the output loop I multiply the "gain per band" values with the corresponding
filter.

Next steps:
* the matching amount can't be adjusted per band yet, as stated in the title
* custom GUI

**05.06.2016**

The possibility to seperately adjust the EQ for low, mid and high frequencies is now implemented. Moreover, I did the design for the Graphical User Interface which covers knobs, toggles and background images. 

Besides that, I focussed on completing the documentation of the process of developing this plugin. I added comments to the source code and built a subpage on my website.

##Future Improvements

* storing spectrums as presets
* automatically adjusting master gain
* a better sounding filterbank (especially in the high frequency band)
