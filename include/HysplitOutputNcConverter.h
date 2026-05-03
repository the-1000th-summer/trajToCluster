#pragma once
#include <string>
#include <chrono>
#include <filesystem>

#include "TrajTextFileData.h"

class HysplitOutputNcConverter {
public:
    HysplitOutputNcConverter(const std::string &inputFileDir, const std::string &outputFilePath, int maxEndpointNum);

    void convert();

private:
    std::chrono::system_clock::time_point getFileDatetime(const std::string &inputFileName);
    std::chrono::hours getFileTimeInterval(const std::vector<std::string> &inputFilesName);


    void writeToOutputData(OutputData &outputData, const TrajTextFileData &trajTextFileData, int inputFileIndex);
    void writeToOutputFile(const OutputData &outputData);

private:
    std::string m_inputFileDir;
    std::string m_outputFilePath;
    int m_maxEndpointNum;
};
