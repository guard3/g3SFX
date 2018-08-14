//g3SFX source code by guard3
//TODO: Add Xbox and PS2(?) support

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <Windows.h>

struct soundData
{
	uint32_t StartingOffset;
	uint32_t Size;
	uint32_t SampleRate;
	int32_t LoopBeginOffset;
	int32_t LoopEndOffset;
};

struct wavHeader_pc
{
	char ChunkID[4] = { 'R', 'I', 'F', 'F' };
	uint32_t ChunkSize = 36;
	char Format[4] = { 'W', 'A', 'V', 'E' };
	char Subchunk1ID[4] = { 'f', 'm', 't', ' ' };
	uint32_t Subchunk1Size = 16;
	uint16_t AudioFormat = 1;
	uint16_t NumChannels = 1;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign = 2;
	uint16_t BitsPerSample = 16;
	char Subchunk2ID[4] = { 'd', 'a', 't', 'a' };
	uint32_t Subchunk2Size;
};

class ConfigurationFile
{
private:
	char cf_mode;
	char* cf_source = new char[MAX_PATH];
	char* cf_list = new char[MAX_PATH];
	char* cf_loop = new char[MAX_PATH];
	char* cf_output = new char[MAX_PATH];
public:
	ConfigurationFile() {};
	ConfigurationFile(char* _IniFileName) { open(_IniFileName); }
	~ConfigurationFile()
	{
		delete[] cf_source;
		delete[] cf_list;
		delete[] cf_loop;
		delete[] cf_output;
	}
	void open(std::string _IniFileName)
	{
		//Getting absolute INI path
		while (_IniFileName.front() == '\\' || _IniFileName.front() == '/')
		{
			_IniFileName.erase(0, 1);
			if (_IniFileName.empty()) throw "Invalid INI path specified.";
		}
		char* full_path_name_buffer = new char[MAX_PATH];
		GetFullPathName(_IniFileName.c_str(), MAX_PATH, full_path_name_buffer, NULL);
		_IniFileName = full_path_name_buffer;
		delete[] full_path_name_buffer;
		while (_IniFileName.back() == '\\') _IniFileName.pop_back();

		//Getting INI root folder path
		std::string ini_path_str = _IniFileName;
		std::size_t found = ini_path_str.rfind("\\");
		ini_path_str = ini_path_str.substr(0, found);

		//Store mode info to cf_mode
		char* mode_buffer = new char[3];
		DWORD copied = GetPrivateProfileString("MODE", "mode", NULL, mode_buffer, 3, _IniFileName.c_str());
		if (GetLastError() == 2) throw "Could not get mode info; either wrong INI path or missing parameter.";
		if (strcmp(mode_buffer, "e") != 0 && strcmp(mode_buffer, "b")) throw "Invalid mode specified.";
		cf_mode = mode_buffer[0];
		delete[] mode_buffer;

		//Store paths
		GetPrivateProfileString(cf_mode == 'e' ? "EXTRACT" : "BUILD", "source", (ini_path_str + (cf_mode == 'e' ? "" : "\\sfx")).c_str(),  cf_source, MAX_PATH, _IniFileName.c_str());
		GetPrivateProfileString(cf_mode == 'e' ? "EXTRACT" : "BUILD", "list", cf_mode == 'e' ? "noloop" : (ini_path_str + "\\sfx.lst").c_str(), cf_list, MAX_PATH, _IniFileName.c_str());
		if (cf_mode == 'b' && strcmp(cf_list, "nolist") == 0) throw "A list file is required to build an archive.";
		GetPrivateProfileString(cf_mode == 'e' ? "EXTRACT" : "BUILD", "loop", "noloop", cf_loop, MAX_PATH, _IniFileName.c_str());
		GetPrivateProfileString(cf_mode == 'e' ? "EXTRACT" : "BUILD", "output", (ini_path_str + (cf_mode == 'e' ? "\\sfx" : "")).c_str(), cf_output, MAX_PATH, _IniFileName.c_str());
	}
	char mode() { return cf_mode; }
	std::string source() { return cf_source; }
	std::string list() { return cf_list; }
	std::string loop() { return cf_loop; }
	std::string output() { return cf_output; }
	bool isNoLoop() { return strcmp(cf_loop, "noloop") == 0; }
	bool isNoList() { return strcmp(cf_list, "nolist") == 0; }
};

int main(int argc, char* argv[])
{
	//Ensure there's only one argument
	if (argc == 1)
	{
		std::cout
			<< "g3SFX - a SFX utility for GTA III/VC and Manhunt by guard3\n"
			<< "Usage: g3SFX [path to INI file]";
		exit(0);
	}
	if (argc > 2)
	{
		std::cout << "Too many arguments given.";
		exit(0);
	}

	//TODO: Add check to ensure that the file extension is indeed an INI
	//Parsing the ini
	ConfigurationFile ini;
	try { ini.open(argv[1]); }
	catch (const char* e)
	{
		std::cerr << e;
		exit(0);
	}

	//Extract or Build
	//TODO: Make huge blocks into separate more "manageable" functions. Possibly get rid of gotos
	if (ini.mode() == 'e')
	{
		std::ifstream sdt(ini.source() + "\\sfx.sdt", std::ios::binary);
		if (!sdt)
		{
			std::cerr << ini.source() << "\\sfx.sdt\nCouldn't open file.";
			exit(0);
		}
		std::ifstream raw(ini.source() + "\\sfx.raw", std::ios::binary);
		if (!raw)
		{
			std::cerr << ini.source() << "\\sfx.raw\nCouldn't open file.";
			exit(0);
		}
		soundData sound_data;
		std::ofstream loop;
		if (!ini.isNoLoop())
		{
			loop.open(ini.loop());
			if (!loop)
			{
				std::cerr << ini.loop() << "\nCouldn't write to file.";
				exit(0);
			}
			loop << ";SFX loop file - by guard3\n;A: Loop Byte Begin (0 for start of file)\n;B: Loop Byte End (-1 for end of file)\n;A\tB\n";
		}
		std::cout << "Extracting...\n";
		if (ini.isNoList())
		{
			int i = 0;
			try { std::experimental::filesystem::create_directories(ini.output().c_str()); }
			catch (std::exception e) {}
			while (sdt.read(reinterpret_cast<char*>(&sound_data), sizeof(soundData)))
			{
				std::ofstream wav(ini.output() + "\\sfx" + std::to_string(i) + ".wav", std::ios::binary);
				if (!wav)
				{
					std::cerr << ini.output() << "\\sfx" << i << ".wav\nCouldn't write to file.";
					exit(0);
				}
				wavHeader_pc wav_header;
				wav_header.ChunkSize += sound_data.Size;
				wav_header.SampleRate = sound_data.SampleRate;
				wav_header.ByteRate = sound_data.SampleRate * 2;
				wav_header.Subchunk2Size = sound_data.Size;

				char* buffer = new char[sound_data.Size];
				raw.read(buffer, sound_data.Size);
				wav.write(reinterpret_cast<const char*>(&wav_header), sizeof(wav_header));
				wav.write(buffer, sound_data.Size);
				wav.close();

				if (!ini.isNoLoop())
				{
					loop
						<< std::to_string(sound_data.LoopBeginOffset)
						<< "\t"
						<< std::to_string(sound_data.LoopEndOffset)
						<< "\t;sfx"
						<< std::to_string(i)
						<< "\n";
				}

				i++;
				delete[] buffer;
			}
		}
		else
		{
			std::ifstream lst(ini.list());
			if (!lst)
			{
				std::cerr << "Could not open list file.";
				exit(0);
			}
			std::string wav_path_from_lst;
			while (std::getline(lst, wav_path_from_lst))
			{
				//Ensure that the read line is a path
			start_string_processing:
				while (wav_path_from_lst.empty()) std::getline(lst, wav_path_from_lst);
				while (wav_path_from_lst.front() == ' ' || wav_path_from_lst.front() == '\t' || wav_path_from_lst.front() == '.' || wav_path_from_lst.front() == '\\')
				{
					wav_path_from_lst.erase(0, 1);
					if (wav_path_from_lst.empty()) goto start_string_processing;
				}
				if (wav_path_from_lst.front() == ';' || wav_path_from_lst.front() == '-')
				{
					std::getline(lst, wav_path_from_lst);
					goto start_string_processing;
				}
				while (wav_path_from_lst.back() == ' ' || wav_path_from_lst.back() == '\t' || wav_path_from_lst.back() == '\\') wav_path_from_lst.pop_back();
				if (wav_path_from_lst == "quit") break;

				std::size_t found = wav_path_from_lst.find("\\\\");
				while (found != std::string::npos)
				{
					wav_path_from_lst.replace(found, 2, "\\");
					found = wav_path_from_lst.find("\\\\");
				}

				std::string wav_folder_to_be_created;
				found = wav_path_from_lst.rfind("\\");
				if (found != std::string::npos) wav_folder_to_be_created = wav_path_from_lst.substr(0, found);
				else wav_folder_to_be_created = "";

				try { std::experimental::filesystem::create_directories((ini.output() + "\\" + wav_folder_to_be_created).c_str()); }
				catch (std::exception e) {}

				std::ofstream wav(ini.output() + "\\" + wav_path_from_lst, std::ios::binary);
				if (!wav)
				{
					std::cerr << ini.output() << "\\" << wav_path_from_lst << "\nCouldn't write to file.";
					exit(0);
				}

				sdt.read(reinterpret_cast<char*>(&sound_data), sizeof(soundData));
				wavHeader_pc wav_header;
				wav_header.ChunkSize += sound_data.Size;
				wav_header.SampleRate = sound_data.SampleRate;
				wav_header.ByteRate = sound_data.SampleRate * 2;
				wav_header.Subchunk2Size = sound_data.Size;

				char* buffer = new char[sound_data.Size];
				raw.read(buffer, sound_data.Size);
				wav.write(reinterpret_cast<const char*>(&wav_header), sizeof(wav_header));
				wav.write(buffer, sound_data.Size);
				wav.close();

				if (!ini.isNoLoop())
				{
					loop
						<< std::to_string(sound_data.LoopBeginOffset)
						<< "\t"
						<< std::to_string(sound_data.LoopEndOffset)
						<< "\t;"
						<< wav_path_from_lst
						<< "\n";
				}
				delete[] buffer;
			}
		}
		if (loop) loop.close();
		sdt.close();
		raw.close();
		std::cout << "All done!";
	}
	if (ini.mode() == 'b')
	{
		try { std::experimental::filesystem::create_directories(ini.output().c_str()); }
		catch (std::exception e) {}
		std::ofstream sdt(ini.output() + "\\sfx.sdt", std::ios::binary);
		if (!sdt)
		{
			std::cerr << ini.output() << "\\sfx.sdt\nCouldn't open file.";
			exit(0);
		}
		std::ofstream raw(ini.output() + "\\sfx.raw", std::ios::binary);
		if (!raw)
		{
			std::cerr << ini.output() << "\\sfx.raw\nCouldn't open file.";
			exit(0);
		}
		std::ifstream lst(ini.list());
		if (!lst)
		{
			std::cerr << "Could not open list file.";
			exit(0);
		}

		std::ifstream loop;
		if (!ini.isNoLoop())
		{
			loop.open(ini.loop());
			if (!loop)
			{
				std::cerr << ini.loop() << "\nCouldn't open file.";
				exit(0);
			}
		}
		std::cout << "Building...\n";
		std::string wav_path_from_lst;
		uint32_t offset = 0;
		while (std::getline(lst, wav_path_from_lst))
		{
			//Ensure that the read line is a path
		start_string_processing2:
			while (wav_path_from_lst.empty()) std::getline(lst, wav_path_from_lst);
			while (wav_path_from_lst.front() == ' ' || wav_path_from_lst.front() == '\t' || wav_path_from_lst.front() == '.' || wav_path_from_lst.front() == '\\')
			{
				wav_path_from_lst.erase(0, 1);
				if (wav_path_from_lst.empty()) goto start_string_processing2;
			}
			if (wav_path_from_lst.front() == ';' || wav_path_from_lst.front() == '-')
			{
				std::getline(lst, wav_path_from_lst);
				goto start_string_processing2;
			}
			while (wav_path_from_lst.back() == ' ' || wav_path_from_lst.back() == '\t' || wav_path_from_lst.back() == '\\') wav_path_from_lst.pop_back();
			if (wav_path_from_lst == "quit") break;

			std::size_t found = wav_path_from_lst.find("\\\\");
			while (found != std::string::npos)
			{
				wav_path_from_lst.replace(found, 2, "\\");
				found = wav_path_from_lst.find("\\\\");
			}

			std::ifstream wav(ini.source() + "\\" + wav_path_from_lst, std::ios::binary);
			if (!wav)
			{
				std::cerr << ini.source() << "\\" << wav_path_from_lst << "\nCouldn't open file.";
				exit(0);
			}

			wavHeader_pc wav_header;
			wav.read(reinterpret_cast<char*>(&wav_header), sizeof(wavHeader_pc));

			soundData sound_data;
			sound_data.StartingOffset = offset;
			sound_data.Size = wav_header.Subchunk2Size;
			sound_data.SampleRate = wav_header.SampleRate;

			if (ini.isNoLoop())
			{
				sound_data.LoopBeginOffset = 0;
				sound_data.LoopEndOffset = -1;
			}
			else
			{
				std::string loop_line;
				std::getline(loop, loop_line);
			start_string_processing3:
				while (loop_line.empty()) std::getline(loop, loop_line);
				std::size_t found = loop_line.find(";");
				if (found != std::string::npos)
				{
					loop_line = loop_line.substr(0, found);
				}
				if (loop_line.empty())
				{
					std::getline(loop, loop_line);
					goto start_string_processing3;
				}
				int32_t loop_begin;
				int32_t loop_end;
				std::stringstream loop_line_stream(loop_line);
				loop_line_stream >> loop_begin >> loop_end;
				sound_data.LoopBeginOffset = loop_begin;
				sound_data.LoopEndOffset = loop_end;
			}

			char* buffer = new char[wav_header.Subchunk2Size];
			wav.read(buffer, wav_header.Subchunk2Size);
			wav.close();
			raw.write(buffer, wav_header.Subchunk2Size);

			sdt.write(reinterpret_cast<const char*>(&sound_data), sizeof(soundData));
			delete[] buffer;

			offset += wav_header.Subchunk2Size;
		}

		sdt.close();
		raw.close();
		lst.close();
		if (loop) loop.close();
		std::cout << "All done!";
	}
}