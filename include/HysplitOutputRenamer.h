#pragma once
#include <string>
#include <vector>

class HysplitOutputRenamer {
public:
    HysplitOutputRenamer(const std::string &inputFileDir, const std::string &outputFileDir, bool writeToNewFile, int startYear, int endYear, const std::string &inputFileNamePrefix);

    void rename();

private:
    void checkParametersValidity() const;
    void checkStartEndYear(const std::vector<std::string> &inputFilesName);
    std::string getTwoDigitYearStr(const std::string &inputFileName);
    std::string getFourDigitYearStr(const std::string &inputFileName);

private:
    std::string m_inputFileDir;
    std::string m_outputFileDir;
    bool m_writeToNewFile;
    int m_startYear;
    int m_endYear;
    std::string m_inputFileNamePrefix;
};
