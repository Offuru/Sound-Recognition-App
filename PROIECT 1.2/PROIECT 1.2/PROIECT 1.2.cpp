#include "ProjectUtils.h"

namespace fs = std::filesystem;
using cd = std::complex<float>;
const double PI = acos(-1);
const int chunkSize = 2048;
int recordingSize;
double recordingLength;
std::string databaseHashPath = "..\\..\\Song Hashes Generator\\Song Hashes Generator\\HASH Database";
std::string databaseWavPath = "..\\..\\Song Hashes Generator\\Song Hashes Generator\\WAV Music";

const double hashes_per_sec = 43.06640625/2;

typedef struct WAV_HEADER {
	uint8_t RIFF[4] = { 'R', 'I', 'F', 'F' };
	uint32_t ChunkSize;
	uint8_t WAVE[4] = { 'W', 'A', 'V', 'E' };
	uint8_t fmt[4] = { 'f', 'm', 't', ' ' };
	uint32_t Subchunk1Size = 16;
	uint16_t AudioFormat = 1;
	uint16_t NumOfChan = 1;
	uint32_t SamplesPerSec = 44100;
	uint32_t bytesPerSec = 44100 * 2;
	uint16_t blockAlign = 1;
	uint16_t bitsPerSample = 8;
	uint8_t Subchunk2ID[4] = { 'd', 'a', 't', 'a' };
	uint32_t Subchunk2Size;
} wav_hdr;

void getMicInput(WAVEFORMATEX wfx)
{
	HWAVEIN waveIn;
	waveInOpen(&waveIn,
		WAVE_MAPPER,
		&wfx,
		NULL, NULL,
		CALLBACK_NULL | WAVE_FORMAT_DIRECT
	);

	char buffer[44100 / 2];
	WAVEHDR headers[1] = {};

	headers->lpData = buffer;
	headers->dwBufferLength = 44100 / 2;

	waveInPrepareHeader(waveIn, headers, sizeof(*headers));
	waveInAddBuffer(waveIn, headers, sizeof(*headers));

	std::ofstream outfile("recording.bin", std::ios_base::out | std::ios_base::binary);

	std::cout << "Now recording audio.  Press Escape to stop and exit.\n";
	waveInStart(waveIn);

	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000))
		if (headers->dwFlags & WHDR_DONE)
		{
			outfile.write(headers->lpData, headers->dwBufferLength);

			headers->dwFlags = 0;
			headers->dwFlags = 0;
			headers->dwFlags = 0;
			headers->dwBytesRecorded = 0;
			waveInPrepareHeader(waveIn, headers, sizeof(*headers));
			waveInAddBuffer(waveIn, headers, sizeof(*headers));
		}

	waveInStop(waveIn);
	waveInUnprepareHeader(waveIn, headers, sizeof(*headers));
	waveInClose(waveIn);

	system("cls");
	std::cout << "Recording complete, press ESC to proceed.";
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000));
}

std::vector<char> readFile(const char* filename)
{
	std::ifstream fin(filename, std::ios::binary | std::ios::in);

	fin.unsetf(std::ios::skipws);

	std::streampos fileSize;

	fin.seekg(0, std::ios::end);
	fileSize = fin.tellg();
	fin.seekg(0, std::ios::beg);

	std::vector<char> input;
	input.reserve(fileSize);

	input.insert(input.begin(), std::istream_iterator<char>(fin), std::istream_iterator<char>());

	return input;
}

void generateMicInputHashList()
{
	system("cls");
	std::cout << "Creating recording hash, please wait.";

	auto rawInput = readFile("recording.bin");
	std::vector<std::complex<float>> totalFrequencies;

	for (auto it : rawInput)
		totalFrequencies.push_back((float)it);
	std::vector <std::vector<std::complex<float>>> results;
	results.resize(totalFrequencies.size() / chunkSize);

	for (int i = 0; i < totalFrequencies.size() / chunkSize; ++i)
	{
		std::vector<std::complex<float>> complexArray;
		for (int j = 0; j < chunkSize; ++j)
			complexArray.push_back(totalFrequencies[i * chunkSize + j]);
		fft(complexArray, false);
		results[i] = complexArray;
		std::vector<std::complex<float>>().swap(complexArray);
	}

	std::ofstream printHash("mic_input_hash_table.txt");
	recordingSize = results.size() / hashes_per_sec;
	recordingLength = results.size() * 1. / hashes_per_sec;
	for (int t = 0; t < results.size(); ++t)
	{
		double* highscores, * points;
		highscores = new double[5]{ 0 };
		points = new double[5]{ 0 };
		for (int freq = 30; freq < 300; ++freq)
		{
			double magnitude = log(abs(results[t][freq]) + 1);
			int index = getIndex(freq);

			if (magnitude > highscores[index]) {
				highscores[index] = magnitude;
				points[index] = freq;
			}
		}
		long long int hash = getHash(points[0], points[1], points[2], points[3]);
		printHash << hash << '\n';
		delete[] highscores;
		delete[] points;
	}

	system("cls");
	std::cout << "Recording hash created, press ESC to proceed.";
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000));
}

struct Song {
	std::string songPath;
	int time;
	std::string songName;
	int occurences;
};

int HashDistance(long long int a, long long int b)
{
	int x1 = abs(a / 10000000 %1000 - b / 10000000 %1000);
	int x2 = abs(a / 10000 % 1000 - b / 10000 % 1000);
	int x3 = abs(a / 100 % 100 - b / 100 % 100);
	int x4 = abs(a % 100 - b % 100);
	return x1 + x2 + x3 + x4;
}

std::size_t number_of_files_in_directory(std::filesystem::path path)
{
	using std::filesystem::directory_iterator;
	return std::distance(directory_iterator(path), directory_iterator{});
}

std::vector<Song> searchForMatches()
{
	std::vector<Song> songs;
	long long hash;
	int number_of_files = number_of_files_in_directory(databaseWavPath);
	std::ifstream readInput("mic_input_hash_table.txt");
	std::vector<long long int> hashArrayFromRecording;
	while (readInput >> hash)
		hashArrayFromRecording.push_back(hash);

	int cnt = 0;
	for (const auto& entry : fs::directory_iterator(databaseHashPath))
	{
		++cnt;
		double percentage = (int)((cnt * 100. / number_of_files) * 100) / 100.;
		system("cls");
		std::cout << "Searching database: " << percentage<<"% complete";

		std::size_t current_txt = entry.path().string().find_last_of("/\\");
		std::string current_song = entry.path().string().substr(current_txt + 1);

		current_song.erase(current_song.size() - 4);
		std::ifstream readHash(entry.path().string());
		std::vector<long long int> hashArrayForSong;

		while (readHash >> hash)
			hashArrayForSong.push_back(hash);

		std::vector<long long int> occurences;
		occurences.resize(hashArrayForSong.size());

		for (int i = 0; i < hashArrayFromRecording.size(); i++)
			for (int j = i; j < hashArrayForSong.size(); j++)
				if (HashDistance(hashArrayForSong[j], hashArrayFromRecording[i]) <=10 && j - i >= 0)
					occurences[j - i]++;

		long long int max_occurences = INT_MIN;
		int poz = 0;
		for (int j = 0; j < occurences.size(); j++)
			if (max_occurences < occurences[j])
				max_occurences = occurences[j], poz = j;

		Song currentsong;
		currentsong.songName = current_song;
		currentsong.time = poz;
		currentsong.occurences = max_occurences;
		currentsong.songPath = databaseWavPath + "\\" + current_song + ".wav";
		songs.push_back(currentsong);

		std::vector<long long int>().swap(occurences);
		std::vector<long long int>().swap(hashArrayForSong);
		readHash.close();
	}
	std::sort(songs.begin(), songs.end(), [](Song a, Song b) { return a.occurences > b.occurences; });
	
	return songs;
}

#define BLOCK_SIZE 44100 / 2
#define BLOCK_COUNT 1200

static CRITICAL_SECTION waveCriticalSection;
static WAVEHDR* waveBlocks;
static int waveFreeBlockCount;
static int waveCurrentBlock;

void writeAudio(HWAVEOUT hWaveOut, LPSTR data, int size, int time, std::pair<int, int> timeFrame, float deltaT)
{
	int timeUnit = static_cast<int>(std::floor(1.0 / deltaT));
	WAVEHDR* current;
	int remain;
	current = &waveBlocks[waveCurrentBlock];
	while (size > 0)
	{
		if (current->dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

		if (size < (int)(BLOCK_SIZE - current->dwUser))
		{
			memcpy(current->lpData + current->dwUser, data, size);
			current->dwUser += size;
			break;
		}

		remain = BLOCK_SIZE - current->dwUser;

		memcpy(current->lpData + current->dwUser, data, remain);
		size -= remain;
		data += remain;
		current->dwBufferLength = BLOCK_SIZE;
		waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));

		if (time > timeFrame.first * timeUnit && time < timeFrame.second * timeUnit)
		{
			waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));
			if (time > timeFrame.second * timeUnit)
				break;
		}

		EnterCriticalSection(&waveCriticalSection);
		waveFreeBlockCount--;
		LeaveCriticalSection(&waveCriticalSection);

		while (!waveFreeBlockCount)
			Sleep(10);

		waveCurrentBlock++;
		waveCurrentBlock %= BLOCK_COUNT;
		current = &waveBlocks[waveCurrentBlock];
		current->dwUser = 0;

		if (time > timeFrame.second * timeUnit)
			break;
	}
}

WAVEHDR* allocateBlocks(int size, int count)
{
	unsigned char* buffer;
	int i;
	WAVEHDR* blocks;
	DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * count;

	if ((buffer = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, totalBufferSize)) == NULL)
	{
		fprintf(stderr, "Memory allocation error\n");
		ExitProcess(1);
	}

	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * count;

	for (i = 0; i < count; i++)
	{
		blocks[i].dwBufferLength = size;
		blocks[i].lpData = (LPSTR)buffer;
		buffer += size;
	}

	return blocks;
}

void freeBlocks(WAVEHDR* blockArray)
{
	HeapFree(GetProcessHeap(), 0, blockArray);
}

static void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	int* freeBlockCounter = (int*)dwInstance;

	if (uMsg != WOM_DONE)
		return;

	EnterCriticalSection(&waveCriticalSection);
	*freeBlockCounter++;
	LeaveCriticalSection(&waveCriticalSection);
}

void playAudio(std::string songPath, int start, int duration)
{
	HWAVEOUT hWaveOut;
	HANDLE hFile;
	WAVEFORMATEX wfx;
	char buffer[44100 / 2];

	waveBlocks = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
	waveFreeBlockCount = BLOCK_COUNT;
	waveCurrentBlock = 0;
	InitializeCriticalSection(&waveCriticalSection);

	char* filePath = songPath.data();
	auto succesful = (hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));

	if (succesful == INVALID_HANDLE_VALUE)
		ExitProcess(1);

	setWaveFormat(wfx, 1, 44100, 8);

	auto succesful2 = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)&waveFreeBlockCount, CALLBACK_FUNCTION);
	if (succesful2 != MMSYSERR_NOERROR)
		ExitProcess(1);

	std::pair<int, int> timeFrame = { start,start + duration };
	int time = 0;
	float deltaT = 0.5;

	while (true)
	{
		DWORD readBytes;
		if (!ReadFile(hFile, buffer, sizeof(buffer), &readBytes, NULL))
			break;
		if (readBytes == 0)
			break;
		if (readBytes < sizeof(buffer))
			memset(buffer + readBytes, 0, sizeof(buffer) - readBytes);
		writeAudio(hWaveOut, buffer, sizeof(buffer), time, timeFrame, deltaT);
		time++;
	}

	while (waveFreeBlockCount < BLOCK_COUNT)
		Sleep(10);

	for (int i = 0; i < waveFreeBlockCount; i++)
		if (waveBlocks[i].dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, &waveBlocks[i], sizeof(WAVEHDR));
	DeleteCriticalSection(&waveCriticalSection);
	HeapFree(GetProcessHeap(), 0, waveBlocks);
	waveOutClose(hWaveOut);
	CloseHandle(hFile);
}

int main()
{
	WAVEFORMATEX wfx;
	setWaveFormat(wfx, 1, 44100, 8);
	getMicInput(wfx);
	generateMicInputHashList();
	
	auto start = std::chrono::high_resolution_clock::now();
	auto songs = searchForMatches();

	system("cls");
	auto stop = std::chrono::high_resolution_clock::now();
	
	std::cout << "Three closest matches found:\n";
	for (int i = 0; i < songs.size() && i < 3; i++)
		std::cout << i + 1 << ": " << songs[i].songName << " with " << songs[i].occurences << " matches at " << songs[i].time / hashes_per_sec << "s" << '\n';
	
	auto duration = duration_cast<std::chrono::microseconds>(stop - start);

	std::cout << "\nThe recording's length is: "<<recordingLength<<"s" << '\n';
	std::cout << "\nMatching took: " << (int)((duration.count() / 1000000.) * 100) / 100. << "s";
	
	
	
	std::cout << "\n\nPress ESC to play closest match.";
	while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000));

	system("cls");
	std::cout << "Now playing closest match ("<<songs[0].songName<<").";

	playAudio(songs[0].songPath, songs[0].time / hashes_per_sec, recordingSize);

	return 0;
}