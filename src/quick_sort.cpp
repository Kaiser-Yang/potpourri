#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <random>

template <typename T, typename C = std::less<typename std::iterator_traits<T>::value_type>>
auto partition(T begin, T end, T pivot, const C &comp = C()) {
    assert(begin != end);
    std::swap(*begin, *pivot);
    auto left = begin, right = end;
    auto iter = std::next(begin);
    while (iter != right) {
        short res = comp(*iter, *std::prev(iter)) ? -1 : (comp(*std::prev(iter), *iter) ? 1 : 0);
        if (res > 0) {
            right = std::prev(right);
            std::swap(*right, *iter);
        } else if (res == 0) {
            iter = std::next(iter);
        } else {
            std::swap(*left, *iter);
            left = std::next(left);
            iter = std::next(iter);
        }
    }
    return std::make_pair(left, right);
}

static std::random_device rd;
static std::mt19937 gen(rd());
template <typename T, typename C = std::less<typename std::iterator_traits<T>::value_type>>
void quick_sort(T begin, T end, const C &comp = C()) {
    if (begin == end || std::next(begin) == end) { return; }
    using DiffType = typename std::iterator_traits<T>::difference_type;
    std::uniform_int_distribution<DiffType> dis(0, std::distance(begin, end) - 1);  // [0, n-1]
    const auto pivot   = std::next(begin, dis(gen));
    auto [left, right] = partition(begin, end, pivot, comp);
    quick_sort(begin, left, comp);
    quick_sort(right, end, comp);
}

int main() {
    std::vector<int> arr = {3, 1, 4, 1, 5, 9, 2, 6};
    quick_sort(arr.begin(), arr.end());
    std::cout << "Sorted arr: ";
    for (int num : arr) { std::cout << num << " "; }
    std::cout << "\n";
    assert(std::is_sorted(arr.begin(), arr.end()));

    std::vector<int> arr2 = {5, 4, 3, 2, 1};
    quick_sort(arr2.begin(), arr2.end());
    std::cout << "Sorted arr2: ";
    for (int num : arr2) { std::cout << num << " "; }
    std::cout << "\n";
    assert(std::is_sorted(arr2.begin(), arr2.end()));

    std::vector<int> arr3 = {2, 1};
    quick_sort(arr3.begin(), arr3.end());
    std::cout << "Sorted arr3: ";
    for (int num : arr3) { std::cout << num << " "; }
    std::cout << "\n";
    assert(std::is_sorted(arr3.begin(), arr3.end()));

    std::vector<int> arr4 = {3, 3, 3, 3};
    quick_sort(arr4.begin(), arr4.end());
    std::cout << "Sorted arr4: ";
    for (int num : arr4) { std::cout << num << " "; }
    std::cout << "\n";
    assert(std::is_sorted(arr4.begin(), arr4.end()));

    return 0;
}
