#include <cassert>
#include <iostream>
#include <string>
#include <vector>

void manachar(const std::string &s, std::vector<int> &p) {
    std::string t = "^";
    for (char c : s) { t += "#" + std::string(1, c); }
    t += "#$";
    int n = t.size();
    p.clear();
    p.resize(n, 0);
    int center = 0, right = 0;
    for (int i = 1; i < n - 1; ++i) {
        int mirror = 2 * center - i;
        if (i < right) { p[i] = std::min(right - i, p[mirror]); }
        while (t[i + (1 + p[i])] == t[i - (1 + p[i])]) { ++p[i]; }
        if (i + p[i] > right) {
            center = i;
            right  = i + p[i];
        }
    }
}

int main() {
    std::string s = "babad";
    std::vector<int> p;
    manachar(s, p);
    int max_len = 0, center_index = 0;
    for (std::size_t i = 1; i < p.size() - 1; ++i) {
        if (p[i] > max_len) {
            max_len      = p[i];
            center_index = i;
        }
    }
    int start = (center_index - max_len) / 2;

    std::string longest_palindrome = s.substr(start, max_len);
    assert(longest_palindrome == "bab" || longest_palindrome == "aba");
    std::cout << "Longest palindromic substring: " << longest_palindrome << std::endl;
    std::cout << "ALL TESTS PASSED!" << std::endl;
    return 0;
}
