// savekit-dump: print a savekit binary file as human-readable text.
//
// Usage: savekit-dump <file.bin>
// Exit code: 0 = ok, 1 = couldnt read the file or its not a valid savekit file, 2 = wrong usage

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "savekit/dump.hpp"
#include "savekit/file_format.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {   // need exactly 2: the program name and the file
        std::cerr << "usage: savekit-dump <file.bin>\n";
        return 2;
    }

    try {
        std::vector<std::uint8_t> bytes = savekit::read_file(argv[1]);   // 1. read the file from disk
        std::string text = savekit::dump_file(bytes);                    // 2. turn it into readable text
        std::cout << text;                                               // 3. print it
        return 0;
    } catch (const savekit::FormatError& e) {
        // the file opened but isnt a valid savekit file (wrong magic, bad crc...)
        std::cerr << "error: not a valid savekit file: " << e.what() << "\n";
        return 1;
    } catch (const std::runtime_error& e) {
        // couldnt open the file. FormatError is also a runtime_error so it has to be caught first above
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
