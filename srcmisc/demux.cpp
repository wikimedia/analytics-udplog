#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <filesystem>
#include <unordered_map>

std::filesystem::path configFilePath("/etc/udp2log-demux");

bool validateName(char * start, char * end) {
	if (start == end) {
		// Don't allow empty name
		return false;
	}
	for (auto p = start; p != end; p++) {
		char c = *p;
		if (c == '.' || c == '/') {
			// Replace these characters with underscores
			*p = '_';
		} else if (c < 32 || c > 126) {
			// These are beyond the pale and cause the line to be dropped
			return false;
		}
	}
	return true;
}

int main(int argc, char** argv) {
	using namespace boost::program_options;
	using std::cerr, std::endl;
	using std::filesystem::path, std::filesystem::exists;
	constexpr auto append = std::ios_base::app;

	std::string baseDir;

	options_description optDesc;
	optDesc.add_options()
			("help",
					"Show help message.")
			("basedir", value<std::string>(&baseDir),
					"destination path of log files");

	variables_map vm;
	try {
		store(parse_command_line(argc, argv, optDesc), vm);
		if (exists(configFilePath)) {
			store(parse_config_file(configFilePath.c_str(), optDesc), vm);
		}
		notify(vm);
	} catch (std::exception & e) {
		cerr << e.what() << endl;
		cerr << "Usage: " << argv[0] << " [..options..]\nOptions:\n" << optDesc;
		return 1;
	}
	if (vm.count("help")) {
		cerr << optDesc << "\n";
		return 1;
	}
	if (!vm.count("basedir")) {
		cerr << "--basedir is required\n";
		return 1;
	}

	path basePath(baseDir);

	char * buffer = nullptr;
	size_t bufferSize = 0;
	std::unordered_map<std::string, std::shared_ptr<std::ofstream>> openFiles;

	while (true) {
		errno = 0;
		ssize_t lineSize = getline(&buffer, &bufferSize, stdin);
		if (lineSize < 0) {
			if (errno) {
				perror("getline");
			}
			break;
		}
		std::string_view line(buffer, lineSize);
		auto spacePos = line.find(' ');
		if (spacePos == std::string::npos) {
			// No name
			continue;
		}

		if (!validateName(buffer, buffer + spacePos)) {
			continue;
		}

		auto name = std::string(line.substr(0, spacePos)) + ".log";
		auto text = line.substr(spacePos + 1);
		auto fileIter = openFiles.find(name);
		if (fileIter == openFiles.end()) {
			auto newFile = std::make_shared<std::ofstream>(basePath / name, append);
			if (newFile->fail()) {
				cerr << "Failed to open file \"" << name << "\"\n";
				continue;
			}
			auto res = openFiles.emplace(std::make_pair(name, newFile));
			fileIter = res.first;
		}
		auto file = fileIter->second;
		file->write(text.data(), text.size());
		if (file->bad()) {
			cerr << "I/O error writing to file \"" << name << "\"\n";
			openFiles.erase(fileIter);
		}
	}
}
