#pragma once

#include <string>
#include "util.h"
#include "TrajTextFileData.h"



class TextFileReader {
public:
    TextFileReader(const std::string &trajFilePath, int maxEndpointNum);

    TrajTextFileData read();
private:
    

    std::chrono::system_clock::time_point getTrajStartDateTime();
    int getInputFileNumber(const std::vector<std::string> &trajLines);
    int getTrajNumLineIndex(const std::vector<std::string> &trajLines);
    std::vector<std::string> getDiagnosticVarsName(const std::vector<std::string> &trajLines);
    int getTrajStartLineIndex(const std::vector<std::string> &trajLines);
    std::tuple<double, double, double> getLatLonHeightAGL(const std::vector<std::string> &trajLineElements, int currentLineIndex);
    std::vector<double> getDiagnosticVars(const std::vector<std::string> &trajLineElements, int currentLineIndex);

    void checkDateTime(const std::vector<std::string> &trajLineElements, int currentLineIndex, const std::chrono::system_clock::time_point& currentDateTime);

private:
    std::string m_trajFilePath;
    std::chrono::system_clock::time_point m_trajStartDateTime;
    int m_maxEndpointNum;
};

