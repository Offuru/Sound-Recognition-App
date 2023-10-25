#include "..\..\PROIECT 1.2\PROIECT 1.2\ProjectUtils.cpp"
namespace fs = std::filesystem;
const int chunkSize = 2048;
const int FUZ_FACTOR = 2;

std::vector<char> readFile(std::string const& filename)
{
	
	std::ifstream file(filename, std::ios::binary | std::ios::in);

	file.unsetf(std::ios::skipws);

	
	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	std::cout << fileSize << '\n';
	
	std::vector<char> vec;
	vec.reserve(fileSize);

	vec.insert(vec.begin(),
		std::istream_iterator<char>(file),
		std::istream_iterator<char>());

	return vec;
}


int main() {
	std::string path = "WAV Music\\";
	for (const auto& entry : fs::directory_iterator(path))
	{
		std::string database_folder = "HASH Database\\";

		std::size_t current_song = entry.path().string().find_last_of("/\\");
		std::string current_wav = entry.path().string().substr(current_song + 1);


		std::vector<char> v = readFile(entry.path().string());
		v.erase(v.begin(), v.begin() + 44);
		std::string destination = database_folder + current_wav;
		destination.erase(destination.size() - 4); //sterge .wav
		destination += ".txt";
		//std::ofstream print(destination, std::ios::binary | std::ios::out);
		std::cout << destination << "\n";
		// in v we have raw pcm data!
		std::vector<std::complex<float>> totalFrequencies;
		for (auto it : v)
			totalFrequencies.push_back((float)it);
		v.clear();


		//the hashing process begins from here
		std::vector <std::vector<std::complex<float>>> results;
		results.resize(totalFrequencies.size() / chunkSize);
		for (int i = 0; i < totalFrequencies.size() / chunkSize; i++)
		{
			std::vector<std::complex<float>> complexArray;
			for (int j = 0; j < chunkSize; j++)
				complexArray.push_back(totalFrequencies[i * chunkSize + j]);
			fft(complexArray, false);
			results[i] = complexArray;
			std::vector<std::complex<float>>().swap(complexArray);
		}
		std::ofstream fout(destination);
		for (int t = 0; t < results.size(); ++t)
		{
			double* highscores;
			int* points;
			highscores = new double[5] {0};
			points = new int[5] {0};
			for (int freq = 30; freq < 300; ++freq)
			{
				double mag = log(abs(results[t][freq]) + 1);
				int index = getIndex(freq);
				if (mag > highscores[index]) {
					highscores[index] = mag;
					points[index] = freq;
				}
			}
			long long int h = getHash(points[0], points[1], points[2], points[3]);
			fout << h << '\n';
			delete[] highscores;
			delete[] points;
		}
		fout.close();

	}
	return 0;
}