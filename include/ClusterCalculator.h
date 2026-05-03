#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
//#include <absl/container/flat_hash_map.h>
//#include <gtl/phmap.hpp>
//#include <boost/unordered_map.hpp>
#include <tbb/tbb.h>

#include "PairHash.h"


//using CsvCacheType = tbb::concurrent_unordered_map<std::pair<int, int>, double, PairHash>;
using CsvCacheType = std::unordered_map<std::pair<int, int>, double, PairHash>;
//using CsvCacheType = boost::unordered_map<std::pair<int, int>, double, PairHash>;
//using CsvCacheType = gtl::flat_hash_map<std::pair<int, int>, double, PairHash>;

//using CsvCacheType = absl::flat_hash_map<std::pair<int, int>, double, PairHash>;
//using CsvCacheType = std::map< std::pair<int, int>, double>;

class ClusterCalculator {
public:
    ClusterCalculator(const std::string &endpointFilePath, const std::string &tsvOutputFilepath, int clusterEndpointNum, int simplifiedPassNum, int taskNumber);

    void cal();

private:
    void readData();

    // 返回值的键为cluster的ID，0代表不使用的traj的index（从0开始），其它数字代表使用的traj
    // 返回值的value表示该ID的cluster拥有的traj对应的index（从0开始）
    std::map<int, std::set<int>> initCluster();
    void initCSVCache(std::vector<double> &csvCache, const std::map<int, std::set<int>> &clusters);

    void initAvailClusterIndex(std::map<int, std::set<int>> &clusters, std::set<int> &availableClusterIndex);

    std::tuple<double, int, int> getMinTSV_clusterIndex(const std::map<int, std::set<int>> &clusters, const std::unordered_map<int, double> &multipleTrajClusterCSVs, const std::vector<double> &csvCache);

    double sumMultipleTrajClusterCSVs(const std::unordered_map<int, double> &multipleTrajClusterCSVs);
    void updateCSVCache_availClusterIndex(const std::map<int, std::set<int>> &clusters, std::vector<double> &csvCache, int clusterIndex1, int clusterIndex2, std::set<int> &availableClusterIndex);

    size_t calCsvCacheSize(const std::map<int, std::set<int>> &clusters);

    double cal_TSV(const std::map<int, std::set<int>> &clusters);
    double cal_CSV(const std::set<int> &trajsIndex);
    std::map<int, std::set<int>> mergeCluster(const std::map<int, std::set<int>> &cluster, int clusterIndex1, int clusterIndex2);
    std::map<int, std::set<int>> simplifyCluster(const std::map<int, std::set<int>> &mergedCluster, const std::set<int> clusterIndices);

    void cal_meanTraj(const std::set<int> &trajsIndex, double *meanTraj_lon, double *meanTraj_lat);

    double dgcdist(double lat1, double lon1, double lat2, double lon2);

    void writeToOutputFile(const std::vector<double> &minTSVs, const std::vector<int> &cluster_data);
    void writeFullOutputFile(const std::vector<double> &minTSVs_data, const std::vector<double> &changeInTSVs_data, const std::vector<int> &cluster_data);
    void writeSimplifiedOutputFile(const std::vector<double> &minTSVs_data, const std::vector<double> &changeInTSVs_data, const std::vector<int> &cluster_data);

    std::string getSimplifiedFilePath(const std::string &notSimplifiedFilePath);

    std::vector<double> cal_changeInTSV(const std::vector<double> &minTSVs);

    void writeToClusterData(const std::map<int, std::set<int>> &clusters, std::vector<int> &cluster_data);

    std::string getElapsedTimePerPassStr(double elapsedTimeInSeconds, int trajNum);


    inline double trajLon(int trajIndex, int endpointIndex) {
        return lon_data[trajIndex * maxEndpointNumInOneTraj + endpointIndex];
    }
    inline double trajLat(int trajIndex, int endpointIndex) {
        return lat_data[trajIndex * maxEndpointNumInOneTraj + endpointIndex];
    }
    // Szudzik函数，这里要求num2 > num1
    inline size_t szudzik(size_t num1, size_t num2) {
        return num1 + num2 * num2;
    }

private:
    std::string endpointFilePath;
    std::string tsvOutputFilepath;

    int clusterEndpointNum;
    size_t trajectoryNumber;
    size_t maxEndpointNumInOneTraj;
    int simplifiedPassNum;
    int m_taskNumber;

    std::unique_ptr<double[]> lon_data;
    std::unique_ptr<double[]> lat_data;


};
