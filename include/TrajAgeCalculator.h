#pragma once

#include <vector>

class TrajAgeCalculator {
public:
    TrajAgeCalculator(size_t trajectoryNumber, size_t maxEndpointNumInOneTraj);

    std::vector<int> cal(double *lat_data);
    int calOneTrajAge(double *lat_data, int traj_i);


private:
    size_t trajectoryNumber;
    size_t maxEndpointNumInOneTraj;
};
