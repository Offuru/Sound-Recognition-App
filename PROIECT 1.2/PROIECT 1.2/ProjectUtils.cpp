#include "ProjectUtils.h"

void setWaveFormat(WAVEFORMATEX& wfx, int channelCount, int samplesPerSecond, int bitsPerSample)
{
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = channelCount;
    wfx.nSamplesPerSec = samplesPerSecond;
    wfx.wBitsPerSample = bitsPerSample;
    wfx.nBlockAlign = wfx.wBitsPerSample * wfx.nChannels / 8;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
}

void fft(std::vector<std::complex<float>>& amplitudes, bool invert)
{
    if (amplitudes.size() == 1)
        return;

    typedef std::complex<float> complex;
    const double pi = acos(-1);

    std::vector<complex> arr1(amplitudes.size() / 2), arr2(amplitudes.size() / 2);

    for (int i = 0; 2 * i < amplitudes.size(); i++)
    {
        arr1[i] = amplitudes[2 * i];
        arr2[i] = amplitudes[2 * i + 1];
    }
    fft(arr1, invert);
    fft(arr2, invert);

    double ang = 2 * pi / amplitudes.size() * (invert ? -1 : 1);
    complex w(1), wn(cos(ang), sin(ang));
    for (int i = 0; 2 * i < amplitudes.size(); i++)
    {
        amplitudes[i] = arr1[i] + w * arr2[i];
        amplitudes[i + amplitudes.size() / 2] = arr1[i] - w * arr2[i];
        if (invert)
        {
            amplitudes[i] /= 2;
            amplitudes[i + amplitudes.size() / 2] /= 2;
        }
        w *= wn;
    }
}

long long int getHash(int p1, int p2, int p3, int p4)
{
    long long h = (p4) * 10000000 + (p3) * 10000 + (p2) * 100 + (p1);
    return h;
}

int RANGE[] = { 40, 80, 120, 180, 300 };

int getIndex(int freq)
{
    int i = 0;
    while (RANGE[i] < freq)
        i++;
    return i;
}