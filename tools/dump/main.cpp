// savekit-dump: print a savekit binary file as human-readable text.
//
// Usage: savekit-dump <file.bin>

#include <iostream>

#include "savekit/version.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "savekit-dump " << savekit::library_version() << "\n"
                  << "usage: " << argv[0] << " <file.bin>\n";
        return 2;
    }

    // TODO(milestone 5): open the file, read the header, verify the checksum,
    // then walk every tagged field and print its id, type, length and value.
    std::cerr << "savekit-dump: not implemented yet\n";
    return 1;
}
