#include "ClusterCalculator.h"

#include <assert.h>

#include "util.h"
#include <netcdf>
#include <cmath>
#include <climits>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <thread>

#include "TrajAgeCalculator.h"
#include "PairHash.h"


ClusterCalculator::ClusterCalculator(const std::string &endpointFilePath, const std::string &tsvOutputFilepath, int clusterEndpointNum, int simplifiedPassNum, int taskNumber) : endpointFilePath(endpointFilePath), tsvOutputFilepath(tsvOutputFilepath), clusterEndpointNum(clusterEndpointNum), simplifiedPassNum(simplifiedPassNum), m_taskNumber(taskNumber) {
    
}

void ClusterCalculator::cal() {
    std::cout << "Reading input data... ";
    readData();
    std::cout << "Read finished." << std::endl;

    if (m_taskNumber > 0) {
        tbb::global_control global_limit(oneapi::tbb::global_control::max_allowed_parallelism, m_taskNumber);
    }

    auto clusters = initCluster();
    std::cout << "Trajectory number: " << clusters.size() << std::endl;

    std::set<int> availableClusterIndex;
    initAvailClusterIndex(clusters, availableClusterIndex);

    std::vector<double> minTSVs;
    std::vector<int> cluster_data;
    
    int passNum = trajectoryNumber - clusters.at(0).size() - 1;
    cluster_data.reserve(trajectoryNumber * passNum * 2);

    // 拥有多个轨迹的cluster的index
    std::set<int> multipleTrajClusterIndex = {};

    // index为结合的两个cluster的index(Szudzik函数)，值为结合后的cluster的CSV
    size_t csvCacheSize = calCsvCacheSize(clusters);
    std::vector<double> csvCache(csvCacheSize, 0.0);

    std::unordered_map<int, double> multipleTrajClusterCSVs;

    //std::cout << "(clusterID): (traj index): minTSV" << std::endl;

    tbb::tick_count t0 = tbb::tick_count::now();
    std::cout << "Initializing CSV cache... ";
    initCSVCache(csvCache, clusters);

    tbb::tick_count t1 = tbb::tick_count::now();
    const double elapsed_time_init = (t1 - t0).seconds();
    std::cout << "Initialization finished. Time spent: " << elapsed_time_init << " seconds" << std::endl;

    indicators::ProgressBar progressBar{
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"="},
        indicators::option::Lead{">"},
        indicators::option::Remainder{" "},
        indicators::option::End{" ]"},
        indicators::option::PostfixText{"Calculating clusters..."},
        indicators::option::ForegroundColor{indicators::Color::white},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}},
        indicators::option::ShowPercentage{true}
    };
    progressBar.set_progress(0);
    indicators::show_console_cursor(false);

    const int trajNum = clusters.size();
    int lastPercentage = 0;

    tbb::tick_count t0_cc = tbb::tick_count::now();
    while (clusters.size() > 2) {
        //std::cout << "cluster size: " << clusters.size() << std::endl;

        //tbb::tick_count t0 = tbb::tick_count::now();

        auto [minTSV, min_clusterIndex1, min_clusterIndex2] = getMinTSV_clusterIndex(clusters, multipleTrajClusterCSVs, csvCache);

        //tbb::tick_count t1 = tbb::tick_count::now();
        //double elapsed_time = (t1 - t0).seconds();
        //std::cout << "calculate Time: " << elapsed_time << " seconds" << std::endl;


        //t0 = tbb::tick_count::now();

        clusters = mergeCluster(clusters, min_clusterIndex1, min_clusterIndex2);
        multipleTrajClusterIndex.insert(min_clusterIndex1);
        multipleTrajClusterIndex.erase(min_clusterIndex2);

        multipleTrajClusterCSVs.erase(min_clusterIndex2);
        multipleTrajClusterCSVs[min_clusterIndex1] = cal_CSV(clusters.at(min_clusterIndex1));


        //t1 = tbb::tick_count::now();
        //elapsed_time = (t1 - t0).seconds();
        //std::cout << "merge Time: " << elapsed_time << " seconds" << std::endl;


        //t0 = tbb::tick_count::now();

        updateCSVCache_availClusterIndex(clusters, csvCache, min_clusterIndex1, min_clusterIndex2, availableClusterIndex);

        //t1 = tbb::tick_count::now();
        //elapsed_time = (t1 - t0).seconds();
        //std::cout << "update Time: " << elapsed_time << " seconds" << std::endl;

        //std::cout << "Min TSV: " << minTSV << std::endl;

        minTSVs.push_back(minTSV);
        writeToClusterData(clusters, cluster_data);

        const int currentPercentage = 100 * (trajNum - clusters.size()) / (trajNum - 1);
        if (currentPercentage > lastPercentage) {
            lastPercentage = currentPercentage;
            progressBar.set_option(indicators::option::PostfixText{ "cluster size: " + std::to_string(clusters.size()) });
            progressBar.set_progress(currentPercentage);
        }
    }

    progressBar.set_option(indicators::option::PostfixText{"Cluster calculation finished"});
    progressBar.set_progress(100);
    indicators::show_console_cursor(true);

    tbb::tick_count t1_cc = tbb::tick_count::now();
    const double elapsed_time_calCluster = (t1_cc - t0_cc).seconds();
    std::cout <<
    "calculate Time: " << elapsed_time_calCluster << " seconds. " <<
    getElapsedTimePerPassStr(elapsed_time_calCluster, trajNum) <<
    std::endl;

    writeToOutputFile(minTSVs, cluster_data);
}

void ClusterCalculator::writeToClusterData(const std::map<int, std::set<int>>& clusters, std::vector<int>& cluster_data) {
    int outputClusterIndex = 0;
    for (auto clusters_it = clusters.cbegin(); clusters_it != clusters.cend(); ++clusters_it) {
        for (const int &trajIndex : clusters_it->second) {
            cluster_data.push_back(outputClusterIndex);
            cluster_data.push_back(trajIndex);
        }
        ++outputClusterIndex;
    }
}

std::string ClusterCalculator::getElapsedTimePerPassStr(double elapsedTimeInSeconds, int trajNum) {
    double elapsedTimePerPass = elapsedTimeInSeconds / (trajNum - 1);
    if (elapsedTimePerPass < 1) {
        return std::to_string(elapsedTimePerPass * 1000) + " milliseconds/pass";
    }
    return std::to_string(elapsedTimeInSeconds) + " seconds/pass";
}


std::tuple<double, int, int> ClusterCalculator::getMinTSV_clusterIndex(const std::map<int, std::set<int>>& clusters, const std::unordered_map<int, double>& multipleTrajClusterCSVs, const std::vector<double>& csvCache) {
    size_t clustersKeySize = clusters.size() - 1;
    std::vector<int> clustersKey;
    clustersKey.reserve(clustersKeySize);

    // 从第二个clusters开始，因为第一个cluster里都是不满足寿命要求的traj
    for (auto it = std::next(clusters.begin()); it != clusters.end(); ++it) {
        clustersKey.push_back(it->first);
    }

    using priv_h_t = tbb::enumerable_thread_specific<std::tuple<double, int, int>>;

    const std::tuple<double, int, int> initialTuple = std::make_tuple(std::numeric_limits<double>::max(), 0, 0);
    priv_h_t priv_h(initialTuple);

    tbb::parallel_for(tbb::blocked_range<size_t>{0, clustersKeySize - 1}, [&](const tbb::blocked_range<size_t> &r) {
        priv_h_t::reference localMin = priv_h.local();
        for (size_t keyVec_i1 = r.begin(); keyVec_i1 < r.end(); ++keyVec_i1) {
            const int clusterIndex1 = clustersKey[keyVec_i1];

            for (size_t keyVec_i2 = keyVec_i1 + 1; keyVec_i2 < clustersKeySize; ++keyVec_i2) {
                const int clusterIndex2 = clustersKey[keyVec_i2];

                bool firstCHasMultipleTraj = multipleTrajClusterCSVs.contains(clusterIndex1);
                bool secondCHasMultipleTraj = multipleTrajClusterCSVs.contains(clusterIndex2);

                double deltaTSV = 0.0;

                // 如果合并的cluster有多条traj，从unordered_map中去除它，以便接下来的计算
                if (firstCHasMultipleTraj) {
                    double firstCluster_CSV = multipleTrajClusterCSVs.at(clusterIndex1);
                    deltaTSV -= firstCluster_CSV;
                }
                if (secondCHasMultipleTraj) {
                    double secondCluster_CSV = multipleTrajClusterCSVs.at(clusterIndex2);
                    deltaTSV -= secondCluster_CSV;
                }

                double csv = csvCache.at(szudzik(clusterIndex1, clusterIndex2));
                deltaTSV += csv;

                // 发现了更小TSV的合并组合
                if (deltaTSV < std::get<0>(localMin)) {
                    std::get<0>(localMin) = deltaTSV;
                    std::get<1>(localMin) = clusterIndex1;
                    std::get<2>(localMin) = clusterIndex2;
                }

            }
        }
    });


    double minDeltaTSV = std::numeric_limits<double>::max();
    int min_clusterIndex1, min_clusterIndex2;

    for (auto it = priv_h.begin(); it != priv_h.end(); ++it) {
        if (std::get<0>(*it) < minDeltaTSV) {
            minDeltaTSV = std::get<0>(*it);
            min_clusterIndex1 = std::get<1>(*it);
            min_clusterIndex2 = std::get<2>(*it);
        }
    }

    double minTSV = sumMultipleTrajClusterCSVs(multipleTrajClusterCSVs) + minDeltaTSV;
    return { minTSV, min_clusterIndex1, min_clusterIndex2 };
}


void ClusterCalculator::readData() {
    netCDF::NcFile endPointFile(endpointFilePath, netCDF::NcFile::read);

    trajectoryNumber = endPointFile.getDim(Constant::DIMNAME_TRAJ).getSize();
    maxEndpointNumInOneTraj = endPointFile.getDim(Constant::DIMNAME_ENDPOINT).getSize();

    lon_data = std::make_unique<double[]>(trajectoryNumber * maxEndpointNumInOneTraj);
    lat_data = std::make_unique<double[]>(trajectoryNumber * maxEndpointNumInOneTraj);

    endPointFile.getVar(Constant::VARNAME_LON).getVar(lon_data.get());
    endPointFile.getVar(Constant::VARNAME_LAT).getVar(lat_data.get());

    endPointFile.close();

}

std::map<int, std::set<int>> ClusterCalculator::initCluster() {
    auto t = TrajAgeCalculator(trajectoryNumber, maxEndpointNumInOneTraj);
    auto trajAge = t.cal(lat_data.get());

    std::map<int, std::set<int>> cluster;
    cluster[0] = {};

    int cluster_i = 1;
    for (int traj_i = 0; traj_i < trajectoryNumber; ++traj_i) {
        if (trajAge[traj_i] < clusterEndpointNum) {
            cluster[0].insert(traj_i);
        } else {
            cluster[cluster_i] = {traj_i};
            ++cluster_i;
        }
    }

    return cluster;
}

void ClusterCalculator::initCSVCache(std::vector<double>& csvCache, const std::map<int, std::set<int>>& clusters) {
    int clusterMaxIndexP1 = clusters.rbegin()->first + 1;

    tbb::parallel_for(1, clusterMaxIndexP1, 1, [&](int clusterIndex1) {
        for (int clusterIndex2 = clusterIndex1 + 1; clusterIndex2 < clusterMaxIndexP1; ++clusterIndex2) {
            // 此时除了第一个cluster，一个cluster对应一个traj，所以这里用begin()获取cluster的traj
            const int trajIndex1 = *(clusters.at(clusterIndex1).begin());
            const int trajIndex2 = *(clusters.at(clusterIndex2).begin());

            double csv = cal_CSV({ trajIndex1, trajIndex2 });
            csvCache[szudzik(clusterIndex1, clusterIndex2)] = csv;
        }
    });
}


void ClusterCalculator::initAvailClusterIndex(std::map<int, std::set<int>> &clusters, std::set<int>& availableClusterIndex) {
    int maxClusterIndex = clusters.rbegin()->first;
    for (int clusterIndex = 1; clusterIndex < maxClusterIndex + 1; ++clusterIndex) {
        availableClusterIndex.insert(clusterIndex);
    }
}

double ClusterCalculator::sumMultipleTrajClusterCSVs(const std::unordered_map<int, double>& multipleTrajClusterCSVs) {
    double tsv = 0.0;
    for (auto it = multipleTrajClusterCSVs.begin(); it != multipleTrajClusterCSVs.end(); ++it) {
        tsv += it->second;
    }
    return tsv;
}

void ClusterCalculator::updateCSVCache_availClusterIndex(const std::map<int, std::set<int>>& clusters, std::vector<double>& csvCache, int clusterIndex1, int clusterIndex2, std::set<int>& availableClusterIndex) {
    availableClusterIndex.erase(clusterIndex2);

    std::vector<int> avaClusterIndex1;
    // 第一个vector包含的元素个数不会超过clusterIndex1
    avaClusterIndex1.reserve(clusterIndex1);

    auto clusterIndex_it = availableClusterIndex.begin();
    for (; clusterIndex_it != availableClusterIndex.cend(); ++clusterIndex_it) {
        if (*clusterIndex_it <= clusterIndex1) {
            avaClusterIndex1.push_back(*clusterIndex_it);
        } else {
            break;
        }
    }

    //int a = avaClusterIndex_1.size();

    std::vector<int> avaClusterIndex2;
    // 第一个vector包含的元素个数正好是预留的空间
    avaClusterIndex2.reserve(availableClusterIndex.size() - avaClusterIndex1.size());

    for (; clusterIndex_it != availableClusterIndex.cend(); ++clusterIndex_it) {
        avaClusterIndex2.push_back(*clusterIndex_it);
    }

    int avaClusterIndex1_size = avaClusterIndex1.size();
    tbb::parallel_for(0, avaClusterIndex1_size, 1, [&](int i) {
        std::set<int> trajs_csvToBeCalculated = clusters.at(avaClusterIndex1[i]);
        std::set<int> trajs_toBeMerged = clusters.at(clusterIndex1);
        trajs_csvToBeCalculated.merge(trajs_toBeMerged);
        csvCache[szudzik(avaClusterIndex1[i], clusterIndex1)] = cal_CSV(trajs_csvToBeCalculated);
    });

    int avaClusterIndex2_size = avaClusterIndex2.size();
    tbb::parallel_for(0, avaClusterIndex2_size, 1, [&](int i) {
        std::set<int> trajs_csvToBeCalculated = clusters.at(clusterIndex1);
        std::set<int> trajs_toBeMerged = clusters.at(avaClusterIndex2[i]);
        trajs_csvToBeCalculated.merge(trajs_toBeMerged);
        csvCache[szudzik(clusterIndex1, avaClusterIndex2[i])] = cal_CSV(trajs_csvToBeCalculated);
    });

}

size_t ClusterCalculator::calCsvCacheSize(const std::map<int, std::set<int>>& clusters) {
    int clusterMaxIndex = clusters.rbegin()->first;
    // +1很重要，因为最大值会是(clusterMaxIndex - 1) + clusterMaxIndex * clusterMaxIndex
    return (clusterMaxIndex - 1) + clusterMaxIndex * clusterMaxIndex + 1;
}

double ClusterCalculator::cal_TSV(const std::map<int, std::set<int>> &clusters) {
    double tsv = 0.0;

    for (auto cluster_it = clusters.begin(); cluster_it != clusters.end(); ++cluster_it) {
        tsv += cal_CSV(cluster_it->second);
    }
    return tsv;
}

double ClusterCalculator::cal_CSV(const std::set<int> &trajsIndex) {
    //if (trajsIndex.size() == 1) { return 0.0; }

    auto meanTraj_lon = std::make_unique<double[]>(clusterEndpointNum);
    auto meanTraj_lat = std::make_unique<double[]>(clusterEndpointNum);

    cal_meanTraj(trajsIndex, meanTraj_lon.get(), meanTraj_lat.get());

    // cluster spatial variation (CSV)
    double cluster_SV = 0.0;
    for (int trajIndex : trajsIndex) {
        for (int endpoint_i = 0; endpoint_i < clusterEndpointNum; ++endpoint_i) {
            double currentTrajLat = trajLat(trajIndex, endpoint_i);
            double currentTrajLon = trajLon(trajIndex, endpoint_i);
            double meanTraj_lat_point = meanTraj_lat[endpoint_i];
            double meanTraj_lon_point = meanTraj_lon[endpoint_i];

            double endpointDist = dgcdist(
                currentTrajLat,
                currentTrajLon,
                meanTraj_lat_point,
                meanTraj_lon_point
            );
            cluster_SV += pow(endpointDist, 2);
        }
    }

    return cluster_SV;
}

std::map<int, std::set<int>> ClusterCalculator::mergeCluster(const std::map<int, std::set<int>> &cluster, int clusterIndex1, int clusterIndex2) {
    auto newCluster = cluster;
    newCluster[clusterIndex1].merge(newCluster[clusterIndex2]);
    newCluster.erase(clusterIndex2);
    return newCluster;
}

std::map<int, std::set<int>> ClusterCalculator::simplifyCluster(const std::map<int, std::set<int>> &mergedCluster, const std::set<int> clusterIndices) {
    std::map<int, std::set<int>> simplifiedCluster = {};
    for (int clusterIndex : clusterIndices) {
        simplifiedCluster[clusterIndex] = mergedCluster.at(clusterIndex);
    }
    return simplifiedCluster;
}



void ClusterCalculator::cal_meanTraj(const std::set<int> &trajsIndex, double *meanTraj_lon, double *meanTraj_lat) {
    for (int trajIndex : trajsIndex) {
        for (int endpoint_i = 0; endpoint_i < clusterEndpointNum; ++endpoint_i) {
            meanTraj_lon[endpoint_i] += lon_data[trajIndex * maxEndpointNumInOneTraj + endpoint_i];
            meanTraj_lat[endpoint_i] += lat_data[trajIndex * maxEndpointNumInOneTraj + endpoint_i];
        }
    }

    for (int endpoint_i = 0; endpoint_i < clusterEndpointNum; ++endpoint_i) {
        meanTraj_lon[endpoint_i] /= trajsIndex.size();
        meanTraj_lat[endpoint_i] /= trajsIndex.size();
    }
}

void ClusterCalculator::writeToOutputFile(const std::vector<double> &minTSVs, const std::vector<int> &cluster_data) {
    auto changeInTSVs_data = cal_changeInTSV(minTSVs);
    std::cout << "cluster data element number: " << cluster_data.size() << std::endl;

    
    writeFullOutputFile(minTSVs, changeInTSVs_data, cluster_data);
    if (simplifiedPassNum > 0) {
        writeSimplifiedOutputFile(minTSVs, changeInTSVs_data, cluster_data);
    }
    
}

void ClusterCalculator::writeFullOutputFile(const std::vector<double> &minTSVs_data, const std::vector<double> &changeInTSVs_data, const std::vector<int> &cluster_data) {
    netCDF::NcFile tsvFile(tsvOutputFilepath, netCDF::NcFile::replace);

    auto pass_dim = tsvFile.addDim("pass", minTSVs_data.size());
    auto psss_M1_dim = tsvFile.addDim("pass_minus1", minTSVs_data.size() - 1);
    auto traj_dim = tsvFile.addDim("trajNum", trajectoryNumber);
    auto index_dim = tsvFile.addDim("indexDim", 2);

    auto tsv_var = tsvFile.addVar("tsv", netCDF::NcType::nc_DOUBLE, pass_dim);
    auto tsv_change_var = tsvFile.addVar("tsv_change", netCDF::NcType::nc_DOUBLE, psss_M1_dim);
    auto cluster_var = tsvFile.addVar("cluster", netCDF::NcType::nc_INT, { pass_dim, traj_dim, index_dim });

    cluster_var.setCompression(true, true, 4);

    // global attrs
    tsvFile.putAtt("description", "trajectory index starts from 0. Index of meaningful clusters starts from 1. Cluster index 0 means being excluded from analysing cluster.");
    tsvFile.putAtt("hoursToCluster", netCDF::NcType::nc_INT, clusterEndpointNum);

    // pour data
    tsv_var.putVar(minTSVs_data.data());
    tsv_change_var.putVar(changeInTSVs_data.data());
    cluster_var.putVar(cluster_data.data());

    tsvFile.close();
}

void ClusterCalculator::writeSimplifiedOutputFile(const std::vector<double> &minTSVs_data, const std::vector<double> &changeInTSVs_data, const std::vector<int> &cluster_data) {

    std::string simplifiedFilePath = getSimplifiedFilePath(tsvOutputFilepath);
    netCDF::NcFile simplifiedTsvFile(simplifiedFilePath, netCDF::NcFile::replace);

    // dim
    auto pass_dim = simplifiedTsvFile.addDim("pass_simp", simplifiedPassNum);
    auto traj_dim = simplifiedTsvFile.addDim("trajNum", trajectoryNumber);
    auto index_dim = simplifiedTsvFile.addDim("indexDim", 2);

    // var
    auto tsv_var = simplifiedTsvFile.addVar("tsv", netCDF::NcType::nc_DOUBLE, pass_dim);
    auto tsv_change_var = simplifiedTsvFile.addVar("tsv_change", netCDF::NcType::nc_DOUBLE, pass_dim);
    auto cluster_var = simplifiedTsvFile.addVar("cluster", netCDF::NcType::nc_INT, { pass_dim, traj_dim, index_dim });

    cluster_var.setCompression(true, true, 4);

    // global attrs
    simplifiedTsvFile.putAtt("description", "trajectory index starts from 0. Index of meaningful clusters starts from 1. Cluster index 0 means being excluded from analysing cluster.");
    simplifiedTsvFile.putAtt("WARNING", "This is a simplified version.");
    simplifiedTsvFile.putAtt("hoursToCluster", netCDF::NcType::nc_INT, clusterEndpointNum);

    // pour data
    tsv_var.putVar(minTSVs_data.data() + (minTSVs_data.size() - simplifiedPassNum));
    tsv_change_var.putVar(changeInTSVs_data.data() + (changeInTSVs_data.size() - simplifiedPassNum));
    cluster_var.putVar(
        cluster_data.data() + (cluster_data.size() - simplifiedPassNum * trajectoryNumber * 2)
    );

    simplifiedTsvFile.close();
    
}

std::string ClusterCalculator::getSimplifiedFilePath(const std::string &notSimplifiedFilePath) {
    auto notSimplifiedFileStem = std::filesystem::path(notSimplifiedFilePath).stem().string();
    auto simplifiedFileStem = notSimplifiedFileStem + "_simplified";
    auto fileExtension = std::filesystem::path(notSimplifiedFilePath).extension().string();
    auto simplifiedFileName = simplifiedFileStem + fileExtension;
    return (std::filesystem::path(notSimplifiedFilePath).parent_path() / simplifiedFileName).string();
}


std::vector<double> ClusterCalculator::cal_changeInTSV(const std::vector<double>& minTSVs) {
    std::vector<double> changeInTSVs;
    changeInTSVs.reserve(minTSVs.size() - 1);

    for (size_t i = 1; i < minTSVs.size(); ++i) {
        double changeInTSV = (minTSVs[i] - minTSVs[i-1]) / minTSVs[i-1] * 100;
        changeInTSVs.push_back(changeInTSV);
    }
    return changeInTSVs;
}




double ClusterCalculator::dgcdist(double lat1, double lon1, double lat2, double lon2) {
    const double RAD = 0.01745329238474369;
    //const double UNIT = 57.29577995691645;   // return in degrees
    const double UNIT = 6371.22;               // return in kilometers
    const double lat1R = lat1 * RAD;
    const double lat2R = lat2 * RAD;

    const double lonR = std::min(std::min(abs(lon1 - lon2), abs(360.0 - lon1 + lon2)), abs(360.0 - lon2 + lon1)) * RAD;
    const double oVal = atan2(
        sqrt(pow(cos(lat2R) * sin(lonR), 2) + pow(cos(lat1R) * sin(lat2R) - sin(lat1R) * cos(lat2R) * cos(lonR), 2)),
        sin(lat1R) * sin(lat2R) + cos(lat1R) * cos(lat2R) * cos(lonR)
    ) * UNIT;

    //if (std::isnan(oVal)) {
    //    std::cout << oVal << std::endl;
    //}
    return oVal;
    
    //return pow(lat2 - lat1, 2) + pow(lon2 - lon1, 2);
}
