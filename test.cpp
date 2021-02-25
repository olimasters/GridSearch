#include <iostream>
#include <cmath>
#include <numeric>
#include <vector>
#include <algorithm>

#include "gridsearch.hpp"

double func(double x, double y)
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    return -(std::pow((x - 1.2),2.) + std::pow((y + 0.3),2.));
}

double funcHorrible(const std::vector<double>& pts)
{
    return func(pts[0], pts[1]);
}

int main(void)
{
    std::vector<double> mins = {-2,-2};
    std::vector<double> maxes = {2,2};
    auto res  = gridsearch::search(funcHorrible, mins, maxes, 10, 5);
    std::cout << "best score: " << res.score << std::endl;
    std::cout << "achieve with args: ";
    printVec(res.args);
    return 0;
}
