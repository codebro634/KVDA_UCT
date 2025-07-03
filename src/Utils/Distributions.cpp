#include "../../include/Utils/Distributions.h"
#include <vector>
#include <cassert>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace distr {

    std::pair<double,double> confidence_interval(double sum, double squared_sum, double n, double confidence, int min_samples) {
        assert (confidence >= 0 && confidence <= 1);

        if (n < min_samples)
            return {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()};

        int dofs = n - 1;
        double mean = sum / n;
        double sample_variance = std::max(0.0,squared_sum / n - std::pow(mean,2)) * n / static_cast<double>(dofs);
        double studt_critical_value = distr::studt_quantile(1 - (1 -confidence) / 2, dofs, true);
        double conf_range = studt_critical_value * sqrt(sample_variance / n);
        return {mean - conf_range, mean + conf_range};
    }

    std::vector<double> normal_quantiles;

    double normal_quantile(double p) {
        if(normal_quantiles.empty()) {
            std::string path = "resources/Quantiles/gaussian.txt";
            while(!std::filesystem::exists(path) && path.size() < 1000)
                path = "../" + path;
            std::ifstream file(path);
            assert(file.is_open());
            std::string line;
            std::getline(file, line);
            size_t start = 0;
            size_t end = line.find(' ');
            while (end != std::string::npos) {
                auto token = line.substr(start, end - start);
                normal_quantiles.push_back(std::stod(token));
                start = end + 1;
                end = line.find(' ', start);
            }
            auto token = line.substr(start, end - start);
            normal_quantiles.push_back(std::stod(token));
            file.close();
        }

        int idx = std::lround(p / 0.005 - 1);
        return normal_quantiles[idx];

    }

    std::vector<std::vector<double>> chi2_quantiles;

    double chi2_quantile(double p, int df, bool approximate) {
        if(chi2_quantiles.empty()) {
            std::string path = "resources/Quantiles/chi_squared.txt";
            while(!std::filesystem::exists(path) && path.size() < 1000)
                path = "../" + path;
            std::ifstream file(path);
            assert(file.is_open());
            std::string line;
            while (std::getline(file, line)) {
                size_t start = 0;
                size_t end = line.find(' ');
                auto dof_quantiles = std::vector<double>();
                while (end != std::string::npos) {
                    auto token = line.substr(start, end - start);
                    dof_quantiles.push_back(std::stod(token));
                    start = end + 1;
                    end = line.find(' ', start);
                }
                chi2_quantiles.push_back(dof_quantiles);
            }
            file.close();
        }
        int idx = std::lround(p / 0.005 - 1);
        if(df >= static_cast<int>(chi2_quantiles.size())) {
            assert (approximate);
            return df + normal_quantile(p) * sqrt(2.0 * df);
        }else
            return chi2_quantiles[df][idx];
    }


    std::vector<std::vector<double>> studt_quantiles;

    double studt_quantile(double p, int df, bool approximate) {
        if(studt_quantiles.empty()) {
            std::string path = "resources/Quantiles/student_t.txt";
            while(!std::filesystem::exists(path) && path.size() < 1000)
                path = "../" + path;
            std::ifstream file(path);
            assert(file.is_open());
            std::string line;
            while (std::getline(file, line)) {
                size_t start = 0;
                size_t end = line.find(' ');
                auto dof_quantiles = std::vector<double>();
                while (end != std::string::npos) {
                    auto token = line.substr(start, end - start);
                    dof_quantiles.push_back(std::stod(token));
                    start = end + 1;
                    end = line.find(' ', start);
                }
                studt_quantiles.push_back(dof_quantiles);
            }
            file.close();
        }
        int idx = std::lround(p / 0.005 - 1);
        if(df >= static_cast<int>(studt_quantiles.size())) {
            assert (approximate);
            return normal_quantile(p);
        }else
            return studt_quantiles[df][idx];
    }


}







/**
 * Code how to generate the tables with boost
 */


// #include <iostream>
// #include <fstream>
// #include <boost/math/distributions/chi_squared.hpp>
//
//
// std::ofstream outFile("../resources/Quantiles/chi_squared.txt");
//
// // Check if file is open
// if (!outFile.is_open()) {
//     std::cerr << "Error: Unable to open file for writing.\n";
// }
//
// // Set precision for writing
// outFile << std::fixed << std::setprecision(10);
//
// // Quantile calculation
// const double step = 0.005; // Step size
// std::vector<double> probabilities;
//
// // Generate probabilities from 0.01 to 0.99
// for (double p = step; p < 1.0; p += step) {
//     probabilities.push_back(p);
// }
//  //outFile << "Chi-Squared quantiles with step size " << step << "\n";

//
// // Chi-squared distribution with degrees of freedom
// for(int dof = 1; dof <= 10000; dof++) {
//     boost::math::chi_squared_distribution<double> chi_squared_dist(dof);
//
//     //outFile << "Dof:" << dof << " ";
//
//     for (const auto &prob : probabilities) {
//         double quantile = boost::math::quantile(chi_squared_dist, prob);
//         outFile << quantile << " ";
//     }
//     outFile << "\n";
// }
//
// std::cout << "Chi-squared quantiles written to chi_squared_quantiles.txt.\n";
// outFile.close();





// #include <iostream>
// #include <fstream>
// #include <boost/math/distributions/chi_squared.hpp>
// #include <boost/math/distributions/normal.hpp>
//
//
// std::ofstream outFile("../resources/Quantiles/gaussian.txt");
//
// // Check if file is open
// if (!outFile.is_open()) {
//     std::cerr << "Error: Unable to open file for writing.\n";
// }
//
// // Set precision for writing
// outFile << std::fixed << std::setprecision(10);
//
// // Quantile calculation
// const double step = 0.005; // Step size
// //outFile << "Gaussian quantiles with step size " << step << "\n";
// // Generate probabilities from 0.01 to 0.99
// for (double p = step; p < 1.0; p += step) {
//     // Chi-squared distribution with degrees of freedom
//     double quantile = boost::math::quantile(boost::math::normal_distribution<double>(), p);
//     outFile << quantile << " ";
// }
// outFile << "\n";
//
// outFile.close();
//
//
// std::ofstream outFile("../resources/Quantiles/student_t.txt");
//
// // Check if file is open
// if (!outFile.is_open()) {
//     std::cerr << "Error: Unable to open file for writing.\n";
// }
//
// // Set precision for writing
// outFile << std::fixed << std::setprecision(10);
//
// // Quantile calculation
// const double step_size = 0.005; // Step size
// std::vector<double> probabilities;
//
// // Generate probabilities from 0.01 to 0.99
// for (double p = step_size; p < 1.0; p += step_size) {
//     probabilities.push_back(p);
// }
// //outFile << "Chi-Squared quantiles with step size " << step << "\n";
//
// int dofs = 10000;
// // Chi-squared distribution with degrees of freedom
// for(int dof = 1; dof <= dofs; dof++) {
//     boost::math::students_t_distribution<double> stud_t_dist(dof);
//
//     //outFile << "Dof:" << dof << " ";
//
//     for (const auto &prob : probabilities) {
//         double quantile = boost::math::quantile(stud_t_dist, prob);
//         outFile << quantile << " ";
//     }
//     outFile << "\n";
// }
//
// std::cout << "Student-squared quantiles written to stud_t_squared_quantiles.txt.\n";
// outFile.close();



//test distribution
// double step_size = 0.005;
// for(double p = step_size; p < 1.0; p += step_size) {
//     double q = distr::normal_quantile(p);
//     double p2 =  boost::math::quantile(boost::math::normal_distribution<double>(), p);
//     if (std::abs(q - p2) > 0.00001) {
//         std::cout << "Error in normal quantile: " << p << " " << q << " " << p2 << std::endl;
//     }
// }
//
// //test chi2
// for(int dof = 1; dof <= 10000; dof++) {
//     for(double p = step_size; p < 1.0; p += 2*step_size) {
//         double q = distr::chi2_quantile(p, dof);
//         double p2 =  boost::math::quantile(boost::math::chi_squared_distribution<double>(dof), p);
//         if (std::abs(q - p2) > 0.00001) {
//             std::cout << "Error in chi2 quantile: " << p << " " << q << " " << p2 << std::endl;
//         }
//     }
// }
//
//
//
// for(int dof = 1; dof <= dofs; dof++) {
//     for(double p = step_size; p < 1.0; p += step_size) {
//         double q = distr::studt_quantile(p, dof);
//         double p2 =  boost::math::quantile(boost::math::students_t_distribution<double>(dof), p);
//         if (std::abs(q - p2) > 0.00001) {
//             std::cout << "Error in chi2 quantile: " << p << " " << q << " " << p2 << std::endl;
//         }
//     }
// }
//
//
//
// std::cout << "All tests passed" << std::endl;
