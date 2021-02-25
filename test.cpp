#include <iostream>
#include <cmath>
#include <numeric>
#include <vector>
#include <algorithm>
#include <thread>
#include <tuple>
#include <utility>

#include "gridsearch.hpp"

double func(double x, double y)
{
    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(10ms);
    return -(std::pow((x - 1.2),2.) + std::pow((y + 0.3),2.));
}

template <typename T, std::size_t... Is>
void printTupImpl(T tup, std::index_sequence<Is...>)
{
    using Expander = int[];
    (void)Expander{(std::cout << std::get<Is>(tup) << ',',0)...};
    std::cout << std::endl;
}

template <typename T>
void printTup(T tup)
{
    printTupImpl(tup, std::make_index_sequence<std::tuple_size<T>::value>{});
}

int main(void)
{
    std::tuple<double, double> mins = {-2,-2};
    std::tuple<double, double> maxes = {2,2};
    auto res  = gridsearch::search(func, mins, maxes, 100, 5);
    std::cout << "best score: " << res.score << std::endl;
    std::cout << "achieve with args: ";
    printTup(res.args);
    return 0;
}
