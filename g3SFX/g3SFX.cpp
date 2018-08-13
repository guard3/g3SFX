//g3SFX source code by guard3

#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <fstream>
#include <filesystem>

class Argument
{
private:
	char mode;
	std::string path_sfx_raw_sdt = "";
	std::string path_sfx_folder_root = "sfx";

	bool version_exists = false;
	std::string version = "pc";

	bool sfx_list_exists = false;
	std::string path_sfx_list = "sfx.lst";

	bool sfx_loop_exists = false;
	std::string path_loop_list = "loop.txt";

public:
	Argument() { return; }
	Argument(int argc, char* argv[]) { parseArgument(argc, argv); }

	void parseArgument(int argc, char* argv[])
	{
		for (int i = 1; i < argc; i++)
		{
			if (std::string(argv[i]) == "-e" || std::string(argv[i]) == "--extract")
			{
				if (mode != 'e' && mode != 'b')
				{
					if (i + 1 != argc)
					{
						if (argv[i + 1][0] != '-')
						{
							path_sfx_raw_sdt = std::string(argv[i + 1]);
							i++;
						}
					}
					mode = 'e';
				}
				else throw std::invalid_argument("Mode already specified.");
			}
			else if (std::string(argv[i]) == "-b" || std::string(argv[i]) == "--build")
			{
				if (mode != 'e' && mode != 'b')
				{
					if (i + 1 != argc)
					{
						if (argv[i + 1][0] != '-')
						{
							//std::cout << "boo" << std::endl;
							path_sfx_folder_root = std::string(argv[i + 1]);
							//std::cout << std::string(argv[i + 1]) << " " << path_sfx_folder_root << std::endl;
							i++;
						}
					}
					mode = 'b';
				}
				else throw std::invalid_argument("Mode already specified.");
			}
			else if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--version")
			{
				if (!version_exists)
				{
					if (i + 1 != argc)
					{
						if (argv[++i][0] != '-') version = std::string(argv[i]);
						else throw std::invalid_argument("Not enough arguments.");
					}
					else throw std::invalid_argument("Not enough arguments.");
					version_exists = true;
				}
				else throw std::invalid_argument("Version already specified");
			}
			else if (std::string(argv[i]) == "-l" || std::string(argv[i]) == "--list")
			{
				if (!sfx_list_exists)
				{
					if (i + 1 != argc)
					{
						if (argv[++i][0] != '-') path_sfx_list = std::string(argv[i]);
						else throw std::invalid_argument("Not enough arguments.");
					}
					else throw std::invalid_argument("Not enough arguments.");
					sfx_list_exists = true;
				}
				else throw std::invalid_argument("List file already specified.");
			}
			else if (std::string(argv[i]) == "--nolst")
			{
				if (!sfx_list_exists) path_sfx_list = "--nolst";
				else throw std::invalid_argument("List file already specified.");
			}
			else if (std::string(argv[i]) == "-p" || std::string(argv[i]) == "--loop")
			{
				if (!sfx_loop_exists)
				{
					if (i + 1 != argc)
					{
						if (argv[++i][0] != '-') path_loop_list = std::string(argv[i]);
						else throw std::invalid_argument("Not enough arguments.");
					}
					else throw std::invalid_argument("Not enough arguments.");
				}
				else throw std::invalid_argument("Loop file already specified.");
			}
			else if (std::string(argv[i]) == "--noloop")
			{
				if (!sfx_loop_exists) path_loop_list = "--noloop";
				else throw std::invalid_argument("Loop file already specified.");
			}
			else throw std::invalid_argument("Invalid argument.");
		}

		if (mode != 'e' && mode != 'b') throw std::invalid_argument("Missing mode parameters.");
		if (!path_sfx_raw_sdt.empty() && path_sfx_raw_sdt.back() == '\\') path_sfx_raw_sdt.pop_back();
		if (path_sfx_folder_root.back() == '\\') path_sfx_folder_root.pop_back();
	}


	char action() { return mode; }
	std::string pathToSfxList() { return path_sfx_list; }
	std::string pathToLoopList() { return path_loop_list; }
	std::string pathToSfxRawSdt() { return path_sfx_raw_sdt; }
	std::string pathToSfxFolder() { return path_sfx_folder_root; }
	bool isNoLst() { return path_sfx_list == "--nolst"; }
	bool isNoLoop() { return path_loop_list == "--noloop"; }
};

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

int main(int argc, char* argv[])
{
	Argument argument;
	try { argument.parseArgument(argc, argv); }
	catch (const std::exception e)
	{
		std::cout << e.what() << std::endl;
		exit(0);
	}

	if (argument.action() == 'e')
	{
		std::ifstream sdt(argument.pathToSfxRawSdt() + "\\sfx.sdt", std::ios::binary);
		if (!sdt)
		{
			std::cerr << argument.pathToSfxRawSdt() << "\\sfx.sdt\nCouldn't open file.\n";
			exit(0);
		}
		std::ifstream raw(argument.pathToSfxRawSdt() + "\\sfx.raw", std::ios::binary);
		if (!raw)
		{
			std::cerr << argument.pathToSfxRawSdt() << "\\sfx.raw\nCouldn't open file.\n";
			exit(0);
		}
		soundData sound_data;
		std::ofstream loop;
		if (!argument.isNoLoop())
		{
			loop.open(argument.pathToLoopList());
			if (!loop)
			{
				std::cerr << argument.pathToLoopList() << "\nCouldn't write to file.\n";
				exit(0);
			}
			loop << ";SFX loop file - by guard3\n;A: Loop Byte Begin (0 for start of file)\n;B: Loop Byte End (-1 for end of file)\n;A\tB\n";
		}
		std::cout << "Extracting...\n";
		if (argument.isNoLst())
		{
			int i = 0;
			//CreateDirectory((argument.pathToSfxRawSdt() + "\\sfx").c_str(), NULL);
			try
			{
				std::experimental::filesystem::create_directories((argument.pathToSfxRawSdt() + "\\sfx").c_str());
			}
			catch (std::exception e) {}
			while (sdt.read(reinterpret_cast<char*>(&sound_data), sizeof(soundData)))
			{
				std::ofstream wav(argument.pathToSfxRawSdt() + "\\sfx\\sfx" + std::to_string(i) + ".wav", std::ios::binary);
				if (!wav)
				{
					std::cerr << argument.pathToSfxRawSdt() << "\\sfx\\sfx" << i << ".wav\nCouldn't write to file.\n";
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

				if (!argument.isNoLoop())
				{
					loop
						<< std::to_string(sound_data.LoopBeginOffset)
						<< "\t"
						<< std::to_string(sound_data.LoopEndOffset)
						<< "\t;sfx"
						<< std::to_string(i)
						<< "\n";
					/*loop << std::hex << sound_data.LoopBeginOffset;
					loop << "\t";
					loop << std::hex << sound_data.LoopEndOffset;
					loop << "\t;sfx" << std::to_string(i) << std::endl;*/
				}

				i++;
				delete[] buffer;
			}

		}
		else
		{
			std::ifstream lst(argument.pathToSfxList());
			if (!lst)
			{
				std::cerr << "Specified list file is invalid.\n";
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

				try
				{
					std::experimental::filesystem::create_directories((argument.pathToSfxRawSdt() + "\\sfx\\" + wav_folder_to_be_created).c_str());
				}
				catch (std::exception e) {}

				std::ofstream wav(argument.pathToSfxRawSdt() + "\\sfx\\" + wav_path_from_lst, std::ios::binary);
				if (!wav)
				{
					std::cerr << argument.pathToSfxRawSdt() << "\\sfx\\" << wav_path_from_lst << "\nCouldn't write to file.\n";
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

				if (!argument.isNoLoop())
				{
					loop
						<< std::to_string(sound_data.LoopBeginOffset)
						<< "\t"
						<< std::to_string(sound_data.LoopEndOffset)
						<< "\t;"
						<< wav_path_from_lst
						<< "\n";
					/*loop << std::hex << sound_data.LoopBeginOffset;
					loop << "\t";
					loop << std::hex << sound_data.LoopEndOffset;
					loop << "\t;sfx" << std::to_string(i) << std::endl;*/
				}
				delete[] buffer;
			}
		}
		if (loop) loop.close();
		sdt.close();
		raw.close();
		std::cout << "All done!\n";
	}
	if (argument.action() == 'b')
	{
		//std::string path_to_sdt_raw_for_extraction = argument.pathToSfxFolder();

		std::ofstream sdt(argument.pathToSfxFolder() + "\\sfx.sdt", std::ios::binary);
		if (!sdt)
		{
			std::cerr << argument.pathToSfxFolder() << "\\sfx.sdt\nCouldn't open file.\n";
			exit(0);
		}
		std::ofstream raw(argument.pathToSfxFolder() + "\\sfx.raw", std::ios::binary);
		if (!raw)
		{
			std::cerr << argument.pathToSfxFolder() << "\\sfx.raw\nCouldn't open file.\n";
			exit(0);
		}
		std::ifstream lst(argument.pathToSfxList());
		if (!lst)
		{
			std::cerr << "Specified list file is invalid.\n";
			exit(0);
		}

		std::ifstream loop;
		if (!argument.isNoLoop())
		{
			loop.open(argument.pathToLoopList());
			if (!loop)
			{
				std::cerr << argument.pathToLoopList() << "\nCouldn't open file.\n";
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

			std::ifstream wav(argument.pathToSfxFolder() + "\\sfx\\" + wav_path_from_lst, std::ios::binary);
			if (!wav)
			{
				std::cerr << argument.pathToSfxFolder() << "\\sfx\\" << wav_path_from_lst << "\nCouldn't open file.\n";
				exit(0);
			}

			wavHeader_pc wav_header;
			wav.read(reinterpret_cast<char*>(&wav_header), sizeof(wavHeader_pc));

			soundData sound_data;
			sound_data.StartingOffset = offset;
			sound_data.Size = wav_header.Subchunk2Size;
			sound_data.SampleRate = wav_header.SampleRate;

			if (argument.isNoLoop())
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
		std::cout << "All done!\n";
	}
	return 0;
}