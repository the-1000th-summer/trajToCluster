#include "TrajTextFileData.h"

#include <vector>

TrajTextFileData::TrajTextFileData(int maxEndpointNum, int diagnosticVarNum, double fillValue) : m_maxEndpointNum(maxEndpointNum), m_diagnosticVarNum(diagnosticVarNum), m_fillValue(fillValue) {
    lat_data = std::vector<double>(maxEndpointNum, fillValue);
    lon_data = std::vector<double>(maxEndpointNum, fillValue);
    heightAGL_data = std::vector<double>(maxEndpointNum, fillValue);
    time_data = std::vector<double>(maxEndpointNum, fillValue);
    for (int diagnosticVar_i = 0; diagnosticVar_i < diagnosticVarNum; ++diagnosticVar_i) {
        std::vector<double> oneDiagnosticVarData(maxEndpointNum, fillValue);
        diagnosticVars_data.push_back(oneDiagnosticVarData);
    }
}

int TrajTextFileData::getMaxEndpointNum() const {
    return m_maxEndpointNum;
}

int TrajTextFileData::getDiagnosticVarNum() const {
    return m_diagnosticVarNum;
}

double TrajTextFileData::getFillValue() const {
    return m_fillValue;
}



OutputData::OutputData(int trajNum, int maxEndpointNum, int diagnosticVarNum) :
    m_trajNum(trajNum),
    m_maxEndpointNum(maxEndpointNum),
    m_diagnosticVarNum(diagnosticVarNum),
    lat_data{std::make_unique<double[]>(trajNum * maxEndpointNum)},
    lon_data{std::make_unique<double[]>(trajNum * maxEndpointNum)},
    heightAGL_data{std::make_unique<double[]>(trajNum * maxEndpointNum)},
    time_data{std::make_unique<double[]>(trajNum * maxEndpointNum)},
    diagnosticVars_data{ std::make_unique<double[]>(diagnosticVarNum * trajNum * maxEndpointNum)}
{
    
}

int OutputData::getTrajNum() const {
    return m_trajNum;
}

int OutputData::getMaxEndpointNum() const {
    return m_maxEndpointNum;
}

int OutputData::getDiagnosticVarNum() const {
    return m_diagnosticVarNum;
}




