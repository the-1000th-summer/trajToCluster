#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <indicators/progress_bar.hpp>

namespace Constant {

const std::string DIMNAME_TRAJ = "trajectory";
const std::string DIMNAME_ENDPOINT = "obs";

const std::string VARNAME_LAT = "lat";
const std::string VARNAME_LON = "lon";

const int MISSINGVAL_CRITERION = -9990;
const double FILL_VALUE = -9999.0;

const std::string TIME_UNITS = "minutes since 1900-01-01 00:00:00";

const std::unordered_map<std::string, std::string> DIAGNOSTIC_VARS_UNITS = {
    {"PRESSURE", "hPa"},
    {"THETA", "K"},
    {"AIR_TEMP", "K"},
    {"RAINFALL", "mm/h"},
    {"MIXDEPTH", "m"},
    {"RELHUMID", "%"},
    {"SPCHUMID", "g/kg"},
    {"H2OMIXRA", "g/kg"},
    {"TERR_MSL", "m"},
    {"SUN_FLUX", "W/m^2"}
};

const std::unordered_map<std::string, std::string> DIAGNOSTIC_VARS_DESCRIPTION = {
    {"PRESSURE", "Pressure"},
    {"THETA", "Potential temperature"},
    {"AIR_TEMP", "Ambient temperature"},
    {"RAINFALL", "Precipitation"},
    {"MIXDEPTH", "Mixing depth"},
    {"RELHUMID", "Relative humidity"},
    {"SPCHUMID", "Specific humidity"},
    {"H2OMIXRA", "Water vapor mixing ratio"},
    {"TERR_MSL", "Terrain height"},
    {"SUN_FLUX", "Solar radiation"}
};

}

class Util {
public:
    [[noreturn]] static void printErrMsgAndExit(const std::string &msg, int lineNum = -1, const std::string &filePath = "");
    static void printWarningMsg(const std::string &msg);
    static std::vector<std::string> readlines(const std::string &filePath);

    static std::vector<std::string> split(const std::string &s, std::string delimiter = " ");
    static std::string trim_copy(std::string s);

    static void removeEmptyLineAtTheEnd(std::vector<std::string> &lines);

    static std::chrono::system_clock::time_point strptime(const std::string &dateTimeStr, const char *dateTimeFormat);
    //static std::string strftime(const std::chrono::system_clock::time_point &inputDatetime, const std::string &datetimeFormat);
    static double date2num(const std::chrono::system_clock::time_point &inputDateTime, const std::string &units);

    static bool startswith(const std::string &inputStr, const char *startStr);

    static std::vector<std::string> listDir_withRegex(const std::string &inputDir, const std::string &regexStr);
    static std::vector<std::string> listDir_hysplit(const std::string &inputDir);
    static std::vector<std::string> listDir_withPrefix(const std::string &inputDir, const std::string &prefix);

    template<typename T>
    static T getElementAt(const std::vector<T> &vec, int index);

private:
    static void ltrim(std::string &s);
    static void rtrim(std::string &s);
    static void trim(std::string &s);
};
