#pragma comment(lib, "winmm.lib")

#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <iterator>
#include <filesystem>
#include <cstddef>
#include <string>
#include <chrono>
void setWaveFormat(WAVEFORMATEX& wfx, int channelCount, int samplesPerSecond, int bitsPerSample);

void fft(std::vector< std::complex<float>>& amplitudes, bool invert);

long long int getHash(int p1,int p2, int p3, int p4);

int getIndex(int freq);