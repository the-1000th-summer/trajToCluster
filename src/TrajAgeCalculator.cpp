#include "TrajAgeCalculator.h"
#include "util.h"
#include <string>
#include <stdexcept>

TrajAgeCalculator::TrajAgeCalculator(size_t trajectoryNumber, size_t maxEndpointNumInOneTraj) : trajectoryNumber(trajectoryNumber), maxEndpointNumInOneTraj(maxEndpointNumInOneTraj) {
    
}

std::vector<int> TrajAgeCalculator::cal(double *lat_data) {
    auto trajAge = std::vector<int>(trajectoryNumber);

    for (int traj_i = 0; traj_i < trajectoryNumber; ++traj_i) {
        trajAge[traj_i] = calOneTrajAge(lat_data, traj_i);
    }
    return trajAge;
}

int TrajAgeCalculator::calOneTrajAge(double *lat_data, int traj_i) {
    // index倒着检查以加快速度
    for (int endpoint_i = maxEndpointNumInOneTraj - 1; endpoint_i > -1; --endpoint_i) {
        if (lat_data[traj_i * maxEndpointNumInOneTraj + endpoint_i] > Constant::MISSINGVAL_CRITERION) {
            // 轨迹的寿命等于轨迹数据条数-1，这里正好等于endpoint_i
            return endpoint_i;
        }
    }
    throw std::runtime_error("traj: " + std::to_string(traj_i) + "full of missing values!");
}


