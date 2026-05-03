#include "HysplitOutputNcConverter.h"
#include <chrono>
#include <filesystem>
#include <netcdf>
#include "util.h"
#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>
#include <indicators/block_progress_bar.hpp>
#include "TextFileReader.h"

HysplitOutputNcConverter::HysplitOutputNcConverter(const std::string &inputFileDir, const std::string &outputFilePath, int maxEndpointNum) : m_inputFileDir(inputFileDir), m_outputFilePath(outputFilePath), m_maxEndpointNum(maxEndpointNum) {
    
}

void HysplitOutputNcConverter::convert() {
    auto inputFilesName = Util::listDir_hysplit(m_inputFileDir);

    if (inputFilesName.empty()) {
        Util::printErrMsgAndExit("The file that meets the requirements does not exist.\nFile name must be tdump_YYYYmmddHH.\nYou can use the rename function in this program.");
    }
    auto fileTimeInterval = getFileTimeInterval(inputFilesName);


    std::string firstInputFilePath = (std::filesystem::path(m_inputFileDir) / inputFilesName[0]).string();
    auto t_first = TextFileReader(firstInputFilePath, m_maxEndpointNum);
    TrajTextFileData trajTextFileData_first = t_first.read();


    auto outputData = OutputData(inputFilesName.size(), m_maxEndpointNum, trajTextFileData_first.getDiagnosticVarNum());
    outputData.diagnosticVarsName = trajTextFileData_first.diagnosticVarsName;

    bool shouldWarning = true;
    auto currentFileDatetime = getFileDatetime(inputFilesName[0]);

    indicators::ProgressBar bar{
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"="},
        indicators::option::Lead{">"},
        indicators::option::Remainder{" "},
        indicators::option::End{" ]"},
        indicators::option::PostfixText{"Reading HYSPLIT output files..."},
        indicators::option::ForegroundColor{indicators::Color::white},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}},
        indicators::option::ShowPercentage{true}
    };
    bar.set_progress(0);
    indicators::show_console_cursor(false);
    int lastPercentage = 0;

    for (int inputFile_i = 0; inputFile_i < inputFilesName.size(); ++inputFile_i) {
        std::string inputFileName = inputFilesName[inputFile_i];
        if (currentFileDatetime != getFileDatetime(inputFileName) && shouldWarning) {
            Util::printWarningMsg("The time intervals between the files are uneven!");
            shouldWarning = false;
        }

        std::string inputFilePath = (std::filesystem::path(m_inputFileDir) / inputFileName).string();
        auto t = TextFileReader(inputFilePath, m_maxEndpointNum);
        TrajTextFileData trajTextFileData = t.read();

        writeToOutputData(outputData, trajTextFileData, inputFile_i);

        currentFileDatetime += fileTimeInterval;

        const int currentPercentage = inputFile_i * 100 / inputFilesName.size();
        if (currentPercentage > lastPercentage) {
            lastPercentage = currentPercentage;
            bar.set_progress(currentPercentage);
        }
    }
    bar.set_option(indicators::option::PostfixText{"Read finished"});
    bar.set_progress(100);
    indicators::show_console_cursor(true);

    writeToOutputFile(outputData);
}

std::chrono::system_clock::time_point HysplitOutputNcConverter::getFileDatetime(const std::string &inputFileName) {
    return Util::strptime(inputFileName, "tdump_%Y%m%d%H");
}


std::chrono::hours HysplitOutputNcConverter::getFileTimeInterval(const std::vector<std::string> &inputFilesName) {
    const auto firstFileDatetime = getFileDatetime(inputFilesName[0]);
    const auto secondFileDatetime = getFileDatetime(inputFilesName[1]);

    const auto fileTimeInterval = std::chrono::duration_cast<std::chrono::hours>(secondFileDatetime - firstFileDatetime);
    return fileTimeInterval;
}

void HysplitOutputNcConverter::writeToOutputData(OutputData &outputData, const TrajTextFileData &trajTextFileData, int inputFileIndex) {
    int maxEndpointNum = trajTextFileData.getMaxEndpointNum();
    int diagnosticVarNum = trajTextFileData.getDiagnosticVarNum();
    int trajNum = outputData.getTrajNum();

    for (int i = 0; i < maxEndpointNum; ++i) {
        outputData.lat_data[maxEndpointNum * inputFileIndex + i] = trajTextFileData.lat_data[i];
        outputData.lon_data[maxEndpointNum * inputFileIndex + i] = trajTextFileData.lon_data[i];
        outputData.heightAGL_data[maxEndpointNum * inputFileIndex + i] = trajTextFileData.heightAGL_data[i];
        outputData.time_data[maxEndpointNum * inputFileIndex + i] = trajTextFileData.time_data[i];
        for (int diagnosticVar_i = 0; diagnosticVar_i < diagnosticVarNum; ++diagnosticVar_i) {
            outputData.diagnosticVars_data[trajNum * maxEndpointNum * diagnosticVar_i + maxEndpointNum * inputFileIndex + i] = trajTextFileData.diagnosticVars_data[diagnosticVar_i][i];
        }
    }
}

void HysplitOutputNcConverter::writeToOutputFile(const OutputData &outputData) {
    netCDF::NcFile outputFile(m_outputFilePath, netCDF::NcFile::replace);

    // dim
    auto traj_dim = outputFile.addDim(Constant::DIMNAME_TRAJ, outputData.getTrajNum());
    auto obs_dim = outputFile.addDim(Constant::DIMNAME_ENDPOINT, outputData.getMaxEndpointNum());

    // var
    auto time_var = outputFile.addVar("time", netCDF::NcType::nc_DOUBLE, { traj_dim, obs_dim });
    auto lon_var = outputFile.addVar("lon", netCDF::NcType::nc_DOUBLE, { traj_dim, obs_dim });
    auto lat_var = outputFile.addVar("lat", netCDF::NcType::nc_DOUBLE, { traj_dim, obs_dim });
    auto heightAGL_var = outputFile.addVar("heightAGL", netCDF::NcType::nc_DOUBLE, { traj_dim, obs_dim });

    // global attrs
    outputFile.putAtt("featureType", "trajectory");

    // var attrs
    time_var.putAtt("_FillValue", netCDF::NcType::nc_DOUBLE, Constant::FILL_VALUE);
    time_var.putAtt("units", Constant::TIME_UNITS);
    time_var.putAtt("long_name", "time");

    lon_var.putAtt("_FillValue", netCDF::NcType::nc_DOUBLE, Constant::FILL_VALUE);
    lon_var.putAtt("units", "degrees_east");
    lon_var.putAtt("long_name", "longitude");

    lat_var.putAtt("_FillValue", netCDF::NcType::nc_DOUBLE, Constant::FILL_VALUE);
    lat_var.putAtt("units", "degrees_north");
    lon_var.putAtt("long_name", "latitude");

    heightAGL_var.putAtt("_FillValue", netCDF::NcType::nc_DOUBLE, Constant::FILL_VALUE);
    heightAGL_var.putAtt("units", "m");
    heightAGL_var.putAtt("long_name", "height above ground level");

    for (int diagnosticVar_i = 0; diagnosticVar_i < outputData.getDiagnosticVarNum(); ++diagnosticVar_i) {
        std::string diagnosticVarName = outputData.diagnosticVarsName[diagnosticVar_i];
        auto diagnosticVar_var = outputFile.addVar(diagnosticVarName, netCDF::NcType::nc_DOUBLE, { traj_dim, obs_dim });

        diagnosticVar_var.putAtt("_FillValue", netCDF::NcType::nc_DOUBLE, Constant::FILL_VALUE);
        diagnosticVar_var.putAtt("units", Constant::DIAGNOSTIC_VARS_UNITS.at(diagnosticVarName));
        diagnosticVar_var.putAtt("description", Constant::DIAGNOSTIC_VARS_DESCRIPTION.at(diagnosticVarName));
        diagnosticVar_var.putVar(outputData.diagnosticVars_data.get());
    }

    // pour data
    time_var.putVar(outputData.time_data.get());
    lon_var.putVar(outputData.lon_data.get());
    lat_var.putVar(outputData.lat_data.get());
    heightAGL_var.putVar(outputData.heightAGL_data.get());

    outputFile.close();
}


