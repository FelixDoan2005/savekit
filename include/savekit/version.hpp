#pragma once

#include <string_view>

namespace savekit {

// Library version (not the file-format version, which lives in the file header).
std::string_view library_version() noexcept;

}
