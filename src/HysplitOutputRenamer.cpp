#include "HysplitOutputRenamer.h"

#include <regex>
#include <filesystem>
#include <iostream>

#include "util.h"

HysplitOutputRenamer::HysplitOutputRenamer(const std::string &inputFileDir, const std::string &outputFileDir, bool writeToNewFile, int startYear, int endYear, const std::string &inputFileNamePrefix) :
    m_inputFileDir(inputFileDir), m_outputFileDir(outputFileDir),
    m_writeToNewFile(writeToNewFile),
    m_startYear(startYear), m_endYear(endYear),
    m_inputFileNamePrefix(inputFileNamePrefix) {
    checkParametersValidity();
}

void HysplitOutputRenamer::rename() {
    const auto inputFilesName = Util::listDir_withPrefix(m_inputFileDir, m_inputFileNamePrefix);

    if (inputFilesName.empty()) {
        Util::printErrMsgAndExit("The file that meets the requirements does not exist.\nFile name must be\"" + m_inputFileNamePrefix + "<yymmddHH>\".");
    }
    checkStartEndYear(inputFilesName);

    for (const std::string &inputFileName : inputFilesName) {
        std::string fourDigitYearStr = getFourDigitYearStr(inputFileName);
        std::string outputFileName = m_inputFileNamePrefix + "_" + fourDigitYearStr + inputFileName.substr(m_inputFileNamePrefix.size() + 2);

        std::string inputFilePath = (std::filesystem::path(m_inputFileDir) / inputFileName).string();
        std::string outputFilePath = (std::filesystem::path(m_outputFileDir) / outputFileName).string();
        if (m_writeToNewFile) {
            try {
                std::filesystem::copy_file(inputFilePath, outputFilePath, std::filesystem::copy_options::overwrite_existing);
            } catch (const std::filesystem::filesystem_error &e) {
                Util::printErrMsgAndExit("Copy files failed.\n" + std::string(e.what()));
            }
        } else {
            try {
                std::filesystem::rename(inputFilePath, outputFilePath);
            } catch (const std::filesystem::filesystem_error &e) {
                Util::printErrMsgAndExit("Rename files failed.\n" + std::string(e.what()));
            }
        }
    }
}

void HysplitOutputRenamer::checkParametersValidity() const {
    if (!m_writeToNewFile) {
        if (m_inputFileDir != m_outputFileDir) {
            Util::printErrMsgAndExit("when directly rename HYSPLIT output file, inputFileDir and outputFileDir must be the same!");
        }
    }
}

void HysplitOutputRenamer::checkStartEndYear(const std::vector<std::string> &inputFilesName) {
    if (m_startYear < 0) {
        Util::printErrMsgAndExit("User-specified start year cannot be negative.");
    }
    if (m_endYear < 0) {
        Util::printErrMsgAndExit("User-specified end year cannot be negative.");
    }
    if (m_endYear < m_startYear) {
        Util::printErrMsgAndExit("User-specified end year cannot be less than user-specified start year.");
    }
    if (m_endYear - m_startYear > 99) {
        Util::printErrMsgAndExit("The difference between the user-specified end year and the start year cannot exceed 99 years.");
    }
    std::string startYearStr = std::to_string(m_startYear);
    std::string endYearStr = std::to_string(m_endYear);

    std::regex yearPattern("^\\d{4}$");
    if (!std::regex_match(startYearStr, yearPattern)) {
        Util::printErrMsgAndExit("User-specified start year does not conform to the YYYY format.");
    }
    if (!std::regex_match(endYearStr, yearPattern)) {
        Util::printErrMsgAndExit("User-specified end year does not conform to the YYYY format.");
    }

    std::string firstFileNameYear = getTwoDigitYearStr(inputFilesName[0]);
    if (firstFileNameYear != startYearStr.substr(2)) {
        Util::printErrMsgAndExit("User-specified start year does not equal to the year of the first file in input file directory.");
    }
    std::string lastFileNameYear = getTwoDigitYearStr(Util::getElementAt(inputFilesName, -1));
    if (lastFileNameYear != endYearStr.substr(2)) {
        Util::printErrMsgAndExit("User-specified end year does not equal to the year of the last file in input file directory.");
    }
}

std::string HysplitOutputRenamer::getTwoDigitYearStr(const std::string &inputFileName) {
    return inputFileName.substr(m_inputFileNamePrefix.size(), 2);
}


std::string HysplitOutputRenamer::getFourDigitYearStr(const std::string &inputFileName) {
    std::string twoDigitYearStr = getTwoDigitYearStr(inputFileName);

    const std::string startYear_century = std::to_string(m_startYear).substr(0, 2);
    int candidateFourDigitYear = std::stoi(startYear_century + twoDigitYearStr);
    if (candidateFourDigitYear >= m_startYear && candidateFourDigitYear <= m_endYear) {
        return std::to_string(candidateFourDigitYear);
    }
    const std::string endYear_century = std::to_string(m_endYear).substr(0, 2);
    candidateFourDigitYear = std::stoi(endYear_century + twoDigitYearStr);
    if (candidateFourDigitYear >= m_startYear && candidateFourDigitYear <= m_endYear) {
        return std::to_string(candidateFourDigitYear);
    }
    Util::printErrMsgAndExit("unknown error occurred!");
}

