/*
To produce a 14.1 MHz signal with a 1.92 MHz sample rate, 
this code uses digital down-conversion (subsampling), where 
the 14.1 MHz carrier is treated as an alias of a baseband 
frequency within the Nyquist zone of 1.92 MHz. 
Target Carrier: 14.1 MHzSample Frequency (\(F_{s}\)): 
1.92 MHz
Effective Baseband Carrier f_base: 14.1 (mod 1.92)=0.66 MHz (or 660 kHz). 

g++ Tone1k_14-1_short.cpp -o Tone1k_14-1_short.exe
*/



#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <complex>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Structure to hold I/Q pairs
/*
struct IQSample {
    double i;
    double q;
};
*/

short I;
short Q;

int main() {
    const double fs = 1.92e6;       // Sampling frequency: 1.92 MHz
    const double toneFreq = 1000.0; // 1 kHz tone
    const double fc = 14.1e6;       // Target carrier: 14.1 MHz
	const double samples_per_ms = 1920; // Number of samples for 1ms of data
    const int numSamples = samples_per_ms * 10;    // Generate 10ms of data
	const double amplitude = 30000; //Amplitude for 16-bit short (max 32767)
	

    // Determine the effective baseband carrier due to aliasing
    // 14.1 / 1.92 = 7.34375, so it aliases to 0.34375 * 1.92 = 0.66 MHz
    double effectiveCarrier = fmod(fc, fs); 
    
    std::vector<std::complex<short>> iqSignals(numSamples);
	// Formula for Upper Sideband in baseband: e^(j*2*pi*f*t) = cos(2*pi*f*t) + j*sin(2*pi*f*t)
    for (int n = 0; n < numSamples; ++n) {
        double t = n / fs;

        // --- USB Generation (I/Q Baseband) ---
        // For upper sideband, baseband is cos(2πft) + jsin(2πft)
        // Here, we directly generate the aliased complex carrier
        // to move the tone to the desired USB spot.
        
        // Phase of the 1 kHz tone
        double phaseTone = 2.0 * M_PI * toneFreq * t;
        // Phase of the effective carrier (aliased 14.1 MHz)
        double phaseCarrier = 2.0 * M_PI * effectiveCarrier * t;

        // I/Q Generation: 
        // I = cos(tone)cos(carrier) - sin(tone)sin(carrier)
        // Q = sin(tone)cos(carrier) + cos(tone)sin(carrier)
        // This simplifies to I = cos(phaseTone + phaseCarrier), Q = sin(phaseTone + phaseCarrier)
        
        I = amplitude * cos(phaseTone + phaseCarrier);
        Q = amplitude * sin(phaseTone + phaseCarrier);
		iqSignals[n]= std::complex<short>(I, Q);

    }

    // Output first few samples for verification
    std::cout << "Sample\tI\t\tQ" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << i << "\t" << iqSignals[i].real() << "\t" << iqSignals[i].imag() << std::endl;
    }

	std::ofstream outFile("Tone1K_14-1_short.bin", std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing." << std::endl;
        return 1;
    }
	for (const auto& sample : iqSignals) {
		outFile.write(reinterpret_cast<const char*>(&sample), sizeof(std::complex<short>));
	}
    outFile.close();

    return 0;
}
