
#include <chrono>
#include <date/date.h>
#include "util.h"
#include "CustomException.h"
#include <filesystem>
#include <iostream>
#include <regex>
#include <fstream>
#include <sstream>

[[noreturn]] void Util::printErrMsgAndExit(const std::string &msg, int lineNum, const std::string &filePath) {
    std::cout << msg << std::endl;
    if (lineNum >= 0) {
        std::cout << "at: line " << lineNum << std::endl;
    }
    if (!filePath.empty()) {
        std::cout << "at: " << filePath << std::endl;
    }
    std::cout << "\nAbort." << std::endl;
    exit(1);
}

void Util::printWarningMsg(const std::string &msg) {
    std::cout << "Warning: " << msg << std::endl;
}


std::vector<std::string> Util::readlines(const std::string &filePath) {
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        Util::printErrMsgAndExit("error opening file: " + filePath);
    }
    std::string line;
    std::vector<std::string> fileLines;
    while (std::getline(inputFile, line)) {
        fileLines.push_back(line);
    }
    inputFile.close();

    return fileLines;
}

std::vector<std::string> Util::split(const std::string &s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        std::string token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        if (!token.empty()) {
            res.push_back(token);
        }
    }
    
    res.push_back(s.substr(pos_start));
    return res;
}

void Util::ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

void Util::rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void Util::trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

std::string Util::trim_copy(std::string s) {
    trim(s);
    return s;
}

void Util::removeEmptyLineAtTheEnd(std::vector<std::string> &lines) {
    while (true) {
        size_t lastLineIndex = lines.size() - 1;
        std::string lastLine = Util::trim_copy(lines[lastLineIndex]);
        if (lastLine.empty()) {
            lines.pop_back();
        } else {
            break;
        }
    }
}


std::chrono::system_clock::time_point Util::strptime(const std::string &dateTimeStr, const char *dateTimeFormat) {
    std::istringstream in{ dateTimeStr };
    date::sys_seconds tp;
    in >> date::parse(dateTimeFormat, tp);
    if (in.fail()) {
        const std::string exceptionMsg = "fail to parse datetime string: " + dateTimeStr + " from format: " + dateTimeFormat;
        throw DateTimeException(exceptionMsg);
    }

    return tp;
}

//std::string Util::strftime(const std::chrono::system_clock::time_point &inputDatetime, const std::string &datetimeFormat) {
//    std::string formatStr = "{:" + datetimeFormat + "}";
//    return std::format(formatStr, inputDatetime);
//}


double Util::date2num(const std::chrono::system_clock::time_point &inputDateTime, const std::string &units) {
    std::regex timeUnitsPattern(R"((hours|minutes|seconds|) since \d{4}-\d{2}-\d{2}\s\d{2}:\d{2}:\d{2})");
    if (!std::regex_match(units, timeUnitsPattern)) {
        Util::printErrMsgAndExit("time units only support (hours|minutes|seconds|) since YYYY-mm-dd HH:MM:SS");
    }
    std::vector<std::string> unitsElements = Util::split(units);
    std::string unitsDatetimeStr = Util::getElementAt(unitsElements, -2) + " " + Util::getElementAt(unitsElements, -1);
    const auto baseDateTime = Util::strptime(unitsDatetimeStr, "%Y-%m-%d %H:%M:%S");

    auto dateTimeInterval_seconds = std::chrono::duration_cast<std::chrono::seconds>(inputDateTime - baseDateTime).count();

    //const auto dateTimeInterval = date::make_time(inputDateTime - baseDateTime);
    //const auto hoursCount = dateTimeInterval.hours().count();
    //const auto minutesCount = dateTimeInterval.minutes().count();
    //const auto secondsCount = dateTimeInterval.seconds().count();

    std::string timeType = unitsElements[0];
    if (timeType == "hours") {
        //return hoursCount + minutesCount / 60.0 + secondsCount / 60.0 / 60.0;
        return dateTimeInterval_seconds / 60.0 / 60.0;
    } else if (timeType == "minutes") {
        //return hoursCount * 60.0 + minutesCount + secondsCount / 60.0;
        return dateTimeInterval_seconds / 60.0;
    } else if (timeType == "seconds") {
        //return hoursCount * 60.0 * 60.0 + minutesCount * 60.0 + secondsCount;
        return dateTimeInterval_seconds;
    }
    Util::printErrMsgAndExit("Unknown Error in function date2num()");
}

bool Util::startswith(const std::string &inputStr, const char *startStr) {
    return inputStr.rfind(startStr, 0) == 0;
}

std::vector<std::string> Util::listDir_withRegex(const std::string &inputDir, const std::string &regexStr) {
    std::vector<std::string> dirElements;
    std::regex fileNamePattern(regexStr);
    for (const auto &entry : std::filesystem::directory_iterator(inputDir)) {
        auto currentFileName = entry.path().filename().string();
        if (std::regex_match(currentFileName, fileNamePattern)) {
            dirElements.push_back(currentFileName);
        }
    }
    std::sort(dirElements.begin(), dirElements.end());
    return dirElements;
}

std::vector<std::string> Util::listDir_hysplit(const std::string &inputDir) {
    return listDir_withRegex(inputDir, "^tdump_(\\d{10})$");
}

std::vector<std::string> Util::listDir_withPrefix(const std::string &inputDir, const std::string &prefix) {
    return listDir_withRegex(inputDir, "^" + prefix + "(\\d{8})$");
}


template <typename T>
T Util::getElementAt(const std::vector<T> &vec, int index) {
    int correctIndex;
    if (index < 0) {
        correctIndex = vec.size() + index;
    } else {
        correctIndex = index;
    }

    if (correctIndex >= 0 && correctIndex < vec.size()) {
        return vec[correctIndex];
    }
    throw std::out_of_range("Index out of range");
}


