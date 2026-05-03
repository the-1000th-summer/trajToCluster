#pragma once

#include <memory>
#include <vector>
#include <string>

class TrajTextFileData {
public:
    TrajTextFileData(int maxEndpointNum, int diagnosticVarNum, double fillValue);

    int getMaxEndpointNum() const;
    int getDiagnosticVarNum() const;
    double getFillValue() const;

    std::vector<double> lat_data;
    std::vector<double> lon_data;
    std::vector<double> heightAGL_data;
    std::vector<double> time_data;
    std::vector<std::vector<double>> diagnosticVars_data;
    std::vector<std::string> diagnosticVarsName;

private:
    int m_maxEndpointNum;
    int m_diagnosticVarNum;
    double m_fillValue;
};

class OutputData {
public:
    OutputData(int trajNum, int maxEndpointNum, int diagnosticVarNum);

    int getTrajNum() const;
    int getMaxEndpointNum() const;
    int getDiagnosticVarNum() const;

    std::unique_ptr<double[]> lat_data;
    std::unique_ptr<double[]> lon_data;
    std::unique_ptr<double[]> heightAGL_data;
    std::unique_ptr<double[]> time_data;
    std::unique_ptr<double[]> diagnosticVars_data;
    std::vector<std::string> diagnosticVarsName;

    

private:
    int m_trajNum;
    int m_maxEndpointNum;
    int m_diagnosticVarNum;
};

