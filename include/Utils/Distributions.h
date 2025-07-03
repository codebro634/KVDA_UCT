#ifndef DISTRIBUTIONS_H
#define DISTRIBUTIONS_H
#include <utility>

namespace distr {
    double normal_quantile(double p);
    double chi2_quantile(double p, int df, bool approximate = false);
    double studt_quantile(double p, int df, bool approximate = false);

    std::pair<double,double> confidence_interval(double sum, double squared_sum, double n, double confidence, int min_samples = 15);
}

#endif
