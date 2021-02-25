#pragma once

#include <cmath>
#include <vector>
#include <thread>
#include <execution>

// TODO: use tuples instead of vectors

template <typename T>
void printVec(const std::vector<T>& vec)
{
    for(const auto& x : vec)
        std::cout << x << ',';
    std::cout << std::endl;
}


namespace gridsearch
{

namespace detail
{

inline void cartesianProductRec(std::vector<std::vector<double>>& result, std::vector<double> current, const std::vector<std::vector<double>>& factors)
{
    if(current.size() == factors.size())
        return (void)result.emplace_back(std::move(current));
    for(double x : factors[current.size()])
    {
        std::vector<double> neighbour = current;
        neighbour.push_back(x);
        cartesianProductRec(result, std::move(neighbour), factors);
    }
}

inline std::vector<std::vector<double>> cartesianProduct(const std::vector<std::vector<double>>& factors)
{
    std::vector<std::vector<double>> res;
    cartesianProductRec(res, {}, factors);
    return res;
}

inline std::vector<std::vector<double>> subdivideDimensions(const std::vector<double>& mins, const std::vector<double>& maxes, int subdivisions)
{
    std::vector<std::vector<double>> result;
    double prop = 1./(subdivisions - 1);
    for(std::size_t i = 0; i < mins.size(); ++i)
    {
        std::vector<double> dimension_subdivision(subdivisions);
        for(std::size_t j = 0; j < subdivisions; ++j)
            dimension_subdivision[j] = std::lerp(mins[i], maxes[i], j * prop);
        result.emplace_back(std::move(dimension_subdivision));
    }
    return result;
}

inline std::vector<std::vector<double>> subdivide(const std::vector<double>& mins, const std::vector<double>& maxes, int subdivisions)
{
    std::vector<std::vector<double>> subdivision;

    std::vector<std::vector<double>> dimension_subdivision = subdivideDimensions(mins, maxes, subdivisions);
    // For example, if looking at the box [0,10] × [100,200] with subdivisions=3, this would
    // now equal {{0,5,10},{100,150,200}}.

    subdivision = cartesianProduct(std::move(dimension_subdivision));
    // We now have the argument sets we want to run with - in the example above, arguments.front() would be {0,100}.
    return subdivision;
}
}

// Performs a grid search to maximise objFun.
// Let n := mins.size() = maxes.size().  Search is performed in the box in R^n defined by
// [mins[0], maxes[0]] × [mins[1], maxes[1]] × ... × [mins[n-1], maxes[n-1]].
// concurrency is the number of threads used.
// subdivisions must be >=2, and is the granularity of th grid. For example,
// if subdivisions = 4, the interval [0,6] would be tested at the points {0,2,4,6}.
template <typename F>
auto search(F&& objFun, const std::vector<double>& mins, const std::vector<double>& maxes, int subdivisions, int concurrency)
{
    if(subdivisions < 2)
        throw std::invalid_argument("subdivisions must be at least 2");
    if(concurrency < 1)
        throw std::invalid_argument("concurrency must be at least 1");
    if(mins.size() != maxes.size())
        throw std::invalid_argument("mins and maxes must have equal size (" + std::to_string(mins.size()) + " vs " + std::to_string(maxes.size())  + ')');
    const std::vector<std::vector<double>> trial_args = detail::subdivide(mins, maxes, subdivisions);

    using It = decltype(trial_args.begin());
    std::vector<std::pair<It, double>> arg_value_pairs;

    for(auto it = trial_args.begin(); it != trial_args.end(); ++it)
        arg_value_pairs.emplace_back(it, std::numeric_limits<double>::min());

    std::for_each(std::execution::par, arg_value_pairs.begin(), arg_value_pairs.end(), [&](auto& arg_val){arg_val.second = objFun(*arg_val.first);});
    auto best = std::max_element(arg_value_pairs.begin(), arg_value_pairs.end(), [](auto&& lhs, auto&& rhs){return lhs.second < rhs.second;});
    struct { std::vector<double> args; double score;} result;
    result.args = *(best->first);
    result.score = best->second;
    return result;
}

}
