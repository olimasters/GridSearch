#pragma once

#include <cmath>
#include <vector>

#ifdef NO_EXECUTION_POLICY
#include <thread>
#else
#include <execution>
#endif

namespace gridsearch
{

namespace detail
{

template <typename F, typename T, typename I>
inline auto invoke_impl(F, T, I);

template <typename F, typename T, std::size_t... Is>
inline auto invoke_impl(F fun, T tup, std::index_sequence<Is...>)
{
    return fun(std::get<Is>(tup)...);
}

template <typename F, typename T>
inline auto tuple_invoke(F fun, T tup)
{
    return invoke_impl(fun, tup,
            std::make_index_sequence<std::tuple_size<T>::value>{}
            );
}

template <int I, class... Ts>
inline decltype(auto) get(Ts&&... ts) 
{
      return std::get<I>(std::forward_as_tuple(ts...));
}

template <typename T, typename... Vec>
inline void cartesian_product_rec(std::vector<std::tuple<typename Vec::value_type...>> & result,
        T current,
        const Vec&... factors)
{
    if constexpr(std::tuple_size<T>::value == sizeof...(Vec))
        return (void)result.emplace_back(current);
    else
        for(auto&& x : get<std::tuple_size<T>::value>(factors...))
            cartesian_product_rec(result, std::tuple_cat(current, std::make_tuple(x)), factors...);
}

template <typename... Vec>
inline auto cartesian_product(const Vec&... factors)
{
    std::vector<std::tuple<typename Vec::value_type...>> res;
    cartesian_product_rec(res, std::tuple<>{}, factors...);
    return res;
}

template <typename T>
inline std::vector<T> subdivide_range(T lo, T hi, int subdivisions, double prop)
{
    std::vector<T> result;
    for(std::size_t i = 0; i < subdivisions; ++i)
        result.emplace_back(std::lerp(lo, hi, i * prop));
    return result;
}

template <typename T, typename I>
inline auto subdivide_impl(const T& mins, const T& maxes, int subdivisions, I);

template <typename T, std::size_t ...Is>
inline auto subdivide_impl(const T& mins, const T& maxes, int subdivisions, std::index_sequence<Is...>)
{
    double prop = 1./(subdivisions - 1);
    return cartesian_product(subdivide_range(std::get<Is>(mins), std::get<Is>(maxes), subdivisions, prop)...);
}

template <typename T>
inline auto subdivide(const T& mins, const T& maxes, int subdivisions)
{
    return subdivide_impl(mins, maxes, subdivisions, std::make_index_sequence<std::tuple_size<T>::value>{});
}

#ifdef NO_EXECUTION_POLICY
template <typename It, typename F>
inline void parallel_for(int concurrency, It begin, It end, F f)
{
    int dist = std::distance(begin, end);
    int tasks_per_thread = dist/concurrency;
    int extras = dist % concurrency;
    std::vector<std::thread> threads;
    while(concurrency--)
    {
        end = begin + tasks_per_thread;
        if(extras > 0)
            end += extras--;
        threads.emplace_back([begin, end, f](){std::for_each(begin, end, f);});
        begin = end;
    }
    for(auto& th : threads)
        th.join();
}
#endif

}

// Performs a grid search to maximise fun.
// Let n := size of mins (= size of maxes).  Search is performed in the box in R^n defined by
// [mins[0], maxes[0]] × [mins[1], maxes[1]] × ... × [mins[n-1], maxes[n-1]].
// concurrency is the number of threads used.
// subdivisions must be >=2, and is the granularity of th grid. For example,
// if subdivisions = 4, the interval [0,6] would be tested at the points {0,2,4,6}.
template <typename F, typename T>
auto search(F fun, const T& mins, const T& maxes, int subdivisions, int concurrency)
{
    if(subdivisions < 2)
        throw std::invalid_argument("subdivisions must be at least 2");
    // TODO: default concurrency to hardware concurrency or something
    if(concurrency < 1)
        throw std::invalid_argument("concurrency must be at least 1");

    const auto trial_args = detail::subdivide(mins, maxes, subdivisions);
    using It = decltype(trial_args.begin());
    std::vector<std::pair<It, double>> arg_value_pairs;
    for(auto it = trial_args.begin(); it != trial_args.end(); ++it)
        arg_value_pairs.emplace_back(it, std::numeric_limits<double>::min());

#ifdef NO_EXECUTION_POLICY
    detail::parallel_for(concurrency,
            arg_value_pairs.begin(),
            arg_value_pairs.end(),
            [&](auto& arg_val){arg_val.second = detail::tuple_invoke(fun, *arg_val.first);});
#else
    std::for_each(std::execution::par,
            arg_value_pairs.begin(),
            arg_value_pairs.end(),
            [&](auto& arg_val){arg_val.second = detail::tuple_invoke(fun, *arg_val.first);});
#endif
    

    auto best = std::max_element(arg_value_pairs.begin(), arg_value_pairs.end(), [](auto&& lhs, auto&& rhs){return lhs.second < rhs.second;});
    struct { T args; double score;} result = { .args = *(best->first), .score = best->second };
    return result;
}

}
