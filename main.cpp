#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
const std::string VERSION = "v0.0.1";
void notImplemented(int count, char** args) {
	std::cout << "Not implemented" << '\n';
}
void help(int count, char** args) {
	std::cout << "QuantumC Formatter version " << VERSION << R"(
qconform help - This command.
qconform list - Lists current default formatting conventions.
qconform conv <filename | default> - Changes formatting conventions to either default, or to the conventions stored in >filename>
qconform check [config file path | default] <filename> - Checks formatting against formatting conventions. Current saved default unless specified file path / default
qconform apply <filename> - Same as above but also applys.
qconform setup - Creates all the folders and files for the cli to work, and adds it to your PATH.
qconform create - Adds formatting rules, creates the rules file, and addes it, with an alias.
qconform alias [-d/--delete | -l/--list] - Creates a quick alias for a rule file and stores it in the saved directorys in the .qconform folder. -d/--delete is the vis-versa, and -l/--list lists all aliases
)";
}
struct Command {
	std::string name;
	std::function<void(int count, char** args)> command;
};
std::vector<Command> commands = {{"help", help}}; 
int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Too few args. Try qconform help" << '\n';
		return 1;
	}
	for (Command comm : commands) {
		if (argv[1] == comm.name) {
			try {
				comm.command(argc, argv);
				return 0;
			} catch (std::string e) {
				std::cout << "Error: " << e << '\n';
			} catch (const char* e) {
				std::cout << "Error: " << e << '\n';
			} catch (char* e) {
				std::cout << "Error: " << e << '\n';
			} catch (...) {
				std::cout << "Unknown error." << '\n';
			}
		}
	}
	std::cout << "Unknown command" << '\n';
	return 1;
}
