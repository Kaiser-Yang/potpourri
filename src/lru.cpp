#include <cassert>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>

template <class K, class V>
class lru {
public:
    lru(size_t cap) : capacity(cap) {}
    lru(const lru &)            = delete;
    lru(lru &&)                 = delete;
    lru &operator=(const lru &) = delete;
    lru &operator=(lru &&)      = delete;

    V get(const K &key, const V &default_value = V()) {
        auto iter = cache_map.find(key);
        if (iter == cache_map.end()) { return default_value; }
        cache_list.splice(cache_list.begin(), cache_list, iter->second);
        return iter->second->second;
    }

    template <typename T, typename U>
    void put(T &&key, U &&value) {
        if (capacity == 0) { return; }
        const K &key_ref = key;
        auto iter        = cache_map.find(key_ref);
        if (iter == cache_map.end()) {
            if (cache_list.size() == capacity) {
                const auto &back = cache_list.back();
                cache_map.erase(back.first);
                cache_list.pop_back();
            }
            cache_list.emplace_front(std::piecewise_construct,
                                     std::forward_as_tuple(std::forward<T>(key)),
                                     std::forward_as_tuple(std::forward<U>(value)));
            cache_map.emplace(std::forward<T>(key), cache_list.begin());
            return;
        }
        cache_list.splice(cache_list.begin(), cache_list, iter->second);
        iter->second->second = value;
    }

private:
    size_t capacity;
    std::list<std::pair<K, V>> cache_list;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> cache_map;
};

int main() {
    {
        lru<int, std::string> cache(3);
        cache.put(1, "A");
        cache.put(2, "B");
        cache.put(3, "C");
        assert(cache.get(1) == "A");
        assert(cache.get(3) == "C");
        cache.put(4, "D");
        assert(cache.get(2) == "");
        assert(cache.get(1) == "A");
        assert(cache.get(3) == "C");
        assert(cache.get(4) == "D");
    }
    {
        lru<std::string, int> cache(2);
        cache.put("apple", 10);
        cache.put("banana", 20);
        assert(cache.get("apple") == 10);
        cache.put("apple", 100);
        cache.put("orange", 30);
        assert(cache.get("banana") == 0);
        assert(cache.get("apple") == 100);
        assert(cache.get("orange") == 30);
    }
    {
        lru<int, int> cache(1);
        cache.put(1, 100);
        assert(cache.get(1) == 100);
        cache.put(2, 200);
        assert(cache.get(1) == 0);
        assert(cache.get(2) == 200);
    }
    {
        lru<char, float> cache(2);
        cache.put('a', 1.0f);
        assert(cache.get('b') == 0.0f);
        assert(cache.get('a') == 1.0f);
    }
    std::cout << "ALL TESTS PASSED!" << std::endl;
    return 0;
}
