
int main_old(int argc, char* argv[])
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