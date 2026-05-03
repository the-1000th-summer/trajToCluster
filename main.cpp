#include <iostream>
#include <string>
#include <filesystem>
#include <thread>
#include "ClusterCalculator.h"
#include "TextFileReader.h"
#include "HysplitOutputNcConverter.h"
#include "HysplitOutputRenamer.h"
#include "util.h"
#include <cxxopts.hpp>

enum PathType {
    File,
    Folder
};

enum AnalysisType {
    ClusterAnalysis,
    Rename,
    HYSPLITToNetCDF
};

void printErrorMsgAndExit(const std::string &errorMsg) {
    std::cout << "\n" << errorMsg << "\nAbort.\n" << std::endl;
    exit(1);
}

bool has_only_digits(const std::string &s) {
    return s.find_first_not_of("0123456789") == std::string::npos;
}

int getHoursToCluster(const std::string &inputStr) {
    if (!has_only_digits(inputStr)) {
        printErrorMsgAndExit("hoursToCluster should be a positive integer!");
    }
    const int hoursToCluster = std::stoi(inputStr);
    if (hoursToCluster < 1) {
        printErrorMsgAndExit("hoursToCluster should be a positive integer!");
    }
    return hoursToCluster;
}

int getSimplifiedPassNum(const std::string &inputStr) {
    if (!has_only_digits(inputStr)) {
        printErrorMsgAndExit("hoursToCluster should be a non negative integer!");
    }
    return std::stoi(inputStr);
}

void printRunInfo(const std::string &endpointFilePath, const std::string &tsvOutputFilePath, int hoursToCluster, int simplifiedPassNum) {
    std::cout << "Endpoint file path: " << endpointFilePath << std::endl;
    std::cout << "Output file path: " << tsvOutputFilePath << std::endl;
    std::cout << "Hours to cluster: " << hoursToCluster << std::endl;

    if (simplifiedPassNum < 1) {
        std::cout << "The program has been configured not to output a simplified version of the output file." << std::endl;
    } else {
        std::cout << "Pass number in simplified file: " << simplifiedPassNum << std::endl;
    }

    std::cout << "\n" << std::endl;
}

std::pair<std::string, std::string> getInOutObjAbsolutePath(cxxopts::ParseResult *result) {
    std::string inputObjPath, outputObjPath;
    try {
        inputObjPath = (*result)["inputObjPath"].as<std::string>();
    } catch (cxxopts::exceptions::option_has_no_value) {
        Util::printErrMsgAndExit("Please specify input file/folder path!");
    } catch (std::exception &e) {
        Util::printErrMsgAndExit("Unhandled exception when getting input object path!\n\nException message:\n" + std::string(e.what()));
    }
    try {
        outputObjPath = (*result)["outputObjPath"].as<std::string>();
    } catch (cxxopts::exceptions::option_has_no_value) {
        Util::printErrMsgAndExit("Please specify output file/folder path!");
    } catch (std::exception &e) {
        Util::printErrMsgAndExit("Unhandled exception when getting output object path!\n\nException message:\n" + std::string(e.what()));
    }

    inputObjPath = std::filesystem::absolute(inputObjPath).string();
    outputObjPath = std::filesystem::absolute(outputObjPath).string();

    return {inputObjPath, outputObjPath};
}

void checkParentPathExistence(const std::string &objPath) {
    std::string parentPath = std::filesystem::path(objPath).parent_path().string();
    if (!std::filesystem::exists(parentPath)) {
        Util::printErrMsgAndExit("Path: " + parentPath + " doesn't exist!");
    }
}

AnalysisType getAnalysisType(const std::string &inputObjPath, const std::string &outputObjPath) {
    if (!std::filesystem::exists(inputObjPath)) {
        Util::printErrMsgAndExit("Input file/folder path doesn't exist!");
    }
    checkParentPathExistence(outputObjPath);

    PathType inputObjPathType;
    if (std::filesystem::is_regular_file(inputObjPath)) {
        inputObjPathType = PathType::File;
    } else if (std::filesystem::is_directory(inputObjPath)) {
        inputObjPathType = PathType::Folder;
    } else if (std::filesystem::is_symlink(inputObjPath)) {
        Util::printErrMsgAndExit("This program doesn't support symbol link as input.");
    } else {
        Util::printErrMsgAndExit("Unknown input file/folder type!");
    }

    PathType outputObjPathType;
    if (std::filesystem::exists(outputObjPath) && std::filesystem::is_directory(outputObjPath)) {
        outputObjPathType = PathType::Folder;
    } else {
        outputObjPathType = PathType::File;
    }

    if (inputObjPathType == PathType::File && outputObjPathType == PathType::File) {
        return AnalysisType::ClusterAnalysis;
    }
    if (inputObjPathType == PathType::Folder && outputObjPathType == PathType::Folder) {
        return AnalysisType::Rename;
    }
    if (inputObjPathType == PathType::Folder && outputObjPathType == PathType::File) {
        return AnalysisType::HYSPLITToNetCDF;
    }
    if (inputObjPathType == PathType::File && outputObjPathType == PathType::Folder) {
        Util::printErrMsgAndExit("When input object is file, output object cannot be folder.");
    }
    Util::printErrMsgAndExit("Unknown error!");
}

template <typename T>
T tryGetCmdLineArgValue(cxxopts::ParseResult *result, const std::string &cmdLineArg, const std::string &argDescription) {
    try {
        return (*result)[cmdLineArg].as<T>();
    } catch (cxxopts::exceptions::option_has_no_value &e) {
        Util::printErrMsgAndExit(e.what());
    } catch (std::exception &e) {
        Util::printErrMsgAndExit("Unhandled exception when getting " + argDescription + ".\n\nException message:\n" + std::string(e.what()));
    }
}

void runAnalysis(cxxopts::ParseResult *result) {
    auto [inputObjPath, outputObjPath] = getInOutObjAbsolutePath(result);
    AnalysisType analysisType = getAnalysisType(inputObjPath, outputObjPath);

    if (analysisType == AnalysisType::ClusterAnalysis) {
        int clusterEndpointNum = tryGetCmdLineArgValue<int>(result, "T", "hours to cluster");
        bool outputSimplifiedResult = (*result)["S"].as<bool>();
        int simplifiedPassNum = 0;
        if (outputSimplifiedResult) {
            simplifiedPassNum = (*result)["N"].as<int>();
        }
        int taskNumber = (*result)["J"].as<int>();
        if (taskNumber < 0) {
            taskNumber = 0;
        }
        const auto processor_count = std::thread::hardware_concurrency();
        if (taskNumber > processor_count) {
            Util::printErrMsgAndExit("User-specified parallel task number: " + std::to_string(taskNumber) + " is larger than the number of CPU cores: " + std::to_string(processor_count));
        }

        std::cout << "User settings:" << std::endl;
        std::cout << "Input file path: " << inputObjPath << std::endl;
        std::cout << "Output file path: " << outputObjPath << std::endl;
        std::cout << "Hours to cluster: " << clusterEndpointNum << std::endl;
        if (outputSimplifiedResult) {
            std::cout << "Output simplified result: YES" << std::endl;
            std::cout << "Pass number in simplified result: " << simplifiedPassNum << std::endl;
        } else {
            std::cout << "Output simplified result: NO" << std::endl;
        }
        if (taskNumber > 0) {
            std::cout << "Parallel task number: " << taskNumber << std::endl;
        } else {
            std::cout << "Use all CPU cores." << std::endl;
        }
        
        std::cout << std::endl;

        auto c = ClusterCalculator(inputObjPath, outputObjPath, clusterEndpointNum, simplifiedPassNum, taskNumber);
        c.cal();
    } else if (analysisType == AnalysisType::Rename) {
        bool writeToNewFile = !(*result)["r"].as<bool>();

        int startYear = tryGetCmdLineArgValue<int>(result, "s", "start year");
        int endYear = tryGetCmdLineArgValue<int>(result, "e", "end year");
        std::string inputFileNamePrefix = tryGetCmdLineArgValue<std::string>(result, "p", "input file name prefix");

        std::cout << "User settings:" << std::endl;
        std::cout << "Input folder path: " << inputObjPath << std::endl;
        std::cout << "Output folder path: " << outputObjPath << std::endl;
        std::cout << "Start year: " << startYear << std::endl;
        std::cout << "End year: " << endYear << std::endl;
        std::cout << "Write to new file: " << (writeToNewFile ? "YES" : "NO") << std::endl;
        std::cout << "Input file name prefix: " << inputFileNamePrefix << std::endl;

        auto r = HysplitOutputRenamer(inputObjPath, outputObjPath, writeToNewFile, startYear, endYear, inputFileNamePrefix);
        r.rename();
    } else if (analysisType == AnalysisType::HYSPLITToNetCDF) {
        int maxEndpointNum = tryGetCmdLineArgValue<int>(result, "n", "trajectory run time") + 1;

        std::cout << "User settings:" << std::endl;
        std::cout << "Input folder path: " << inputObjPath << std::endl;
        std::cout << "Output file path: " << outputObjPath << std::endl;
        std::cout << "Trajectory run time: " << maxEndpointNum << std::endl;
        std::cout << std::endl;

        auto n = HysplitOutputNcConverter(inputObjPath, outputObjPath, maxEndpointNum);
        n.convert();
    } else {
        Util::printErrMsgAndExit("Unknown error!");
    }
}

void startProgram(int argc, char *argv[]) {
    cxxopts::Options options("trajToCluster", "trajToCluster: Aerosol Diffusion Path Clustering Analysis Software");

    options.add_options()
        ("T,hours-to-cluster", "Cluster analysis: The number of hours each trajectory will be analyzed for. Trajectories with a lifetime less than this number of hours will not be analyzed", cxxopts::value<int>())
        ("S,output-simplified-result", "Cluster analysis: Output the simplified file. The number of iterations included is determined by -N.", cxxopts::value<bool>()->default_value("false"))
        ("N,simplified-pass-num", "Cluster analysis: The number of iterations included in the simplified output file", cxxopts::value<int>()->default_value("50"))
        ("J,max-parallel-task", "Cluster analysis: Maximum parallel tasks in cluster analysis. Specifying a number less than 1 indicates the utilization of all CPU cores.", cxxopts::value<int>()->default_value("0"))
        ("n,trajectory-run-time", "HYSPLIT to netCDF: Specify the running hours of HYSPLIT for each trajectory.", cxxopts::value<int>())
        ("p,prefix", "Rename: The prefix of HYSPLIT trajectory files name", cxxopts::value<std::string>())
        ("r,rename-input-file", "Rename: Rename HYSPLIT trajectory files instead of output new files", cxxopts::value<bool>()->default_value("false"))
        ("s,start-year", "Rename: The start year of HYSPLIT trajectory files", cxxopts::value<int>())
        ("e,end-year", "Rename: The end year of HYSPLIT trajectory files", cxxopts::value<int>())
        ("h,help", "Display available options") // a bool parameter
        ("v,version", "Print version information")
    ;

    // hidden options
    options.add_options("hiddenOpts")
        ("inputObjPath", "Input file/folder path", cxxopts::value<std::string>())
        ("outputObjPath", "Output file/folder path", cxxopts::value<std::string>())
    ;
    options.parse_positional({ "inputObjPath", "outputObjPath" });
    options.positional_help("<input file/folder path> <output file/folder path>");

    std::unique_ptr<cxxopts::ParseResult> result;
    try {
        result = std::make_unique<cxxopts::ParseResult>(options.parse(argc, argv));
    } catch (cxxopts::exceptions::no_such_option &e) {
        Util::printErrMsgAndExit(e.what());
    } catch (std::exception &e) {
        Util::printErrMsgAndExit("Exception occurred when parsing command line arguments.\n\nException message:\n" + std::string(e.what()));
    }

    if (result->count("help")) {
        std::cout << options.help({ "" }) << std::endl;
        exit(0);
    }
    if (result->count("version")) {
        std::cout << "trajToCluster v1.0" << std::endl;
        exit(0);
    }

    runAnalysis(result.get());

    std::cout << "Done." << std::endl;
}

int main(int argc, char *argv[]) {
    startProgram(argc, argv);
}
