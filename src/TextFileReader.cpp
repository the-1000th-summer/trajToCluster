#include "TextFileReader.h"
#include "CustomException.h"
#include <iostream>
#include <filesystem>

TextFileReader::TextFileReader(const std::string &trajFilePath, int maxEndpointNum) : m_trajFilePath(trajFilePath), m_maxEndpointNum(maxEndpointNum) {
    m_trajStartDateTime = getTrajStartDateTime();
}

TrajTextFileData TextFileReader::read() {
    auto trajLines = Util::readlines(m_trajFilePath);

    auto diagnosticVarsName = getDiagnosticVarsName(trajLines);
    int trajStartLineIndex = getTrajStartLineIndex(trajLines);

    Util::removeEmptyLineAtTheEnd(trajLines);
    int trajEndLineIndex = trajLines.size() - 1;

    auto trajTextFileData = TrajTextFileData(m_maxEndpointNum, diagnosticVarsName.size(), Constant::FILL_VALUE);

    for (int i = 0; i < trajEndLineIndex - trajStartLineIndex + 1; ++i) {
        const int currentLineIndex = trajStartLineIndex + i;

        if (i >= m_maxEndpointNum) {
            Util::printErrMsgAndExit("The number of endpoints exceeds the user-defined HYSPLIT running hours. Please check the value set by option -n.", currentLineIndex, m_trajFilePath);
        }

        auto trajLineElements = Util::split(Util::trim_copy(trajLines[currentLineIndex]));
        auto currentDateTime = m_trajStartDateTime + std::chrono::hours(i);

        checkDateTime(trajLineElements, currentLineIndex, currentDateTime);
        auto [lat, lon, heightAGL] = getLatLonHeightAGL(trajLineElements, currentLineIndex);
        auto diagnosticVars = getDiagnosticVars(trajLineElements, currentLineIndex);

        trajTextFileData.lat_data[i] = lat;
        trajTextFileData.lon_data[i] = lon;
        trajTextFileData.heightAGL_data[i] = heightAGL;
        trajTextFileData.time_data[i] = Util::date2num(currentDateTime, Constant::TIME_UNITS);
        for (int diagnosticVar_i = 0; diagnosticVar_i < diagnosticVarsName.size(); ++diagnosticVar_i) {
            trajTextFileData.diagnosticVars_data[diagnosticVar_i][i] = diagnosticVars[diagnosticVar_i];
        }
    }

    trajTextFileData.diagnosticVarsName = diagnosticVarsName;
    return trajTextFileData;

}


std::chrono::system_clock::time_point TextFileReader::getTrajStartDateTime() {
    std::string trajFileName = std::filesystem::path(m_trajFilePath).filename().string();
    try {
        return Util::strptime(trajFileName, "tdump_%Y%m%d%H");
    } catch (const DateTimeException &e) {
        const std::string errMsg = "parse \"" + trajFileName + "\" failed, please check if the file name matches format: \"tdump_%Y%m%d%H\".";
        Util::printErrMsgAndExit(errMsg);
    }
}

int TextFileReader::getInputFileNumber(const std::vector<std::string> &trajLines) {
    std::string inputFileNumberLine = trajLines[0];
    inputFileNumberLine = Util::trim_copy(inputFileNumberLine);

    const std::string inputFileNumberStr = Util::split(inputFileNumberLine)[0];
    try {
        const int inputFileNumber = std::stoi(inputFileNumberStr);
        return inputFileNumber;
    } catch (...) {
        Util::printErrMsgAndExit("failed to parse input file number line.", 0, m_trajFilePath);
    }
}

int TextFileReader::getTrajNumLineIndex(const std::vector<std::string> &trajLines) {
    const int inputFileNumber = getInputFileNumber(trajLines);
    const int trajNumberLineIndex = inputFileNumber + 1;
    std::string trajNumberLine = Util::trim_copy(trajLines[trajNumberLineIndex]);

    const std::string trajNumberStr = Util::split(trajNumberLine)[0];
    try {
        const int trajNumber = std::stoi(trajNumberStr);
        if (trajNumber != 1) {
            Util::printErrMsgAndExit("traj number must be 1 in one file!", trajNumberLineIndex, m_trajFilePath);
        }
    } catch (...) {
        Util::printErrMsgAndExit("failed to parse traj number line.", trajNumberLineIndex, m_trajFilePath);
    }
    return inputFileNumber + 1;
}

std::vector<std::string> TextFileReader::getDiagnosticVarsName(const std::vector<std::string> &trajLines) {
    const int trajNumLineIndex = getTrajNumLineIndex(trajLines);

    const int diagnosticVarNameLineIndex = trajNumLineIndex + 1 + 1;
    const std::string diagnosticVarNameLine = Util::trim_copy(trajLines[diagnosticVarNameLineIndex]);
    auto diagnosticVarsName = Util::split(diagnosticVarNameLine);
    diagnosticVarsName.erase(diagnosticVarsName.begin());

    return diagnosticVarsName;
}

int TextFileReader::getTrajStartLineIndex(const std::vector<std::string> &trajLines) {
    const int trajNumLineIndex = getTrajNumLineIndex(trajLines);
    return trajNumLineIndex + 1 + 1 + 1;
}

std::tuple<double, double, double> TextFileReader::getLatLonHeightAGL(const std::vector<std::string> &trajLineElements, int currentLineIndex) {
    try {
        double lat = std::stod(trajLineElements[9]);
        double lon = std::stod(trajLineElements[10]);
        double heightAGL = std::stod(trajLineElements[11]);
        return {lat, lon, heightAGL};
    } catch (...) {
        Util::printErrMsgAndExit("failed to parse endpoint location.", currentLineIndex, m_trajFilePath);
    }
}

std::vector<double> TextFileReader::getDiagnosticVars(const std::vector<std::string> &trajLineElements, int currentLineIndex) {
    std::vector<double> diagnosticVars;
    for (auto it = (trajLineElements.begin() + 12); it != trajLineElements.end(); ++it) {
        try {
            diagnosticVars.push_back(std::stod(*it));
        } catch (...) {
            Util::printErrMsgAndExit("failed to parse diagnostic vars. This program only support diagnostic variables of numeric type.", currentLineIndex, m_trajFilePath);
        }
    }
    return diagnosticVars;
}


void TextFileReader::checkDateTime(const std::vector<std::string> &trajLineElements, int currentLineIndex, const std::chrono::system_clock::time_point &currentDateTime) {
    std::string yearStr = trajLineElements[2];
    int year, month, day, hour, minute;
    try {
        year = std::stoi(yearStr);
        month = std::stoi(trajLineElements[3]);
        day = std::stoi(trajLineElements[4]);
        hour = std::stoi(trajLineElements[5]);
        minute = std::stoi(trajLineElements[6]);
    } catch (...) {
        Util::printErrMsgAndExit("failed to parse endpoint datetime.", currentLineIndex, m_trajFilePath);
    }

    if (minute != 0) {
        Util::printErrMsgAndExit("This program do not support data where minutes are not zero.", currentLineIndex, m_trajFilePath);
    }

    if (
        (std::stoi(std::format("{:%y}", currentDateTime)) != year) ||
        (std::stoi(std::format("{:%m}", currentDateTime)) != month) ||
        (std::stoi(std::format("{:%d}", currentDateTime)) != day) ||
        (std::stoi(std::format("{:%H}", currentDateTime)) != hour)
    ) {
        Util::printErrMsgAndExit("This program only supports trajectories with intervals of one hour!", currentLineIndex, m_trajFilePath);
    }
    
}


