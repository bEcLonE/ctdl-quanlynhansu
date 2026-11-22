#ifndef TEXT_IO_H
#define TEXT_IO_H

#include <istream>
#include <string>

inline bool readLine(std::istream& input, std::string& line) {
    if (!std::getline(input, line)) {
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return true;
}

#endif
