#include <cassert>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>

template <typename Node = int, typename Weight = int>
class adjacency_list {
public:
    adjacency_list() = default;

    adjacency_list(const adjacency_list &)            = delete;
    adjacency_list(adjacency_list &&)                 = delete;
    adjacency_list &operator=(const adjacency_list &) = delete;
    adjacency_list &operator=(adjacency_list &&)      = delete;

    void add_edge(const Node &u, const Node &v, const Weight &weight) {
        adj[u].emplace_back(v, weight);
    }

    const std::vector<std::pair<Node, Weight>> &get_neighbors(const Node &u) const {
        static const std::vector<std::pair<Node, Weight>> empty;
        auto iter = adj.find(u);
        if (iter != adj.end()) { return iter->second; }
        return empty;
    }

private:
    std::unordered_map<Node, std::vector<std::pair<Node, Weight>>> adj;
};

template <typename Node = int, typename Weight = int>
using Dist = std::unordered_map<Node, Weight>;

template <typename Node = int, typename Weight = int>
auto dijkstra(const Node &start, const adjacency_list<Node, Weight> &graph) {
    Dist<Node, Weight> dist;
    using pq_element = std::pair<Weight, Node>;
    std::priority_queue<pq_element, std::vector<pq_element>, std::greater<pq_element>> pq;
    pq.emplace(0, start);
    dist[start] = 0;
    while (!pq.empty()) {
        auto [current_dist, u] = pq.top();
        pq.pop();
        if (current_dist > dist[u]) { continue; }
        for (const auto &[v, weight] : graph.get_neighbors(u)) {
            Weight new_dist = current_dist + weight;
            if (!dist.count(v) || new_dist < dist[v]) {
                dist[v] = new_dist;
                pq.emplace(new_dist, v);
            }
        }
    }
    return dist;
}

template <typename Node = int, typename Weight = int>
using edge = std::tuple<Node, Node, Weight>;

template <typename Node      = int,
          typename Weight    = int,
          typename Container = std::vector<edge<Node, Weight>>>
auto bellman_ford(const Node &start, const Container &edges, size_t node_num) {
    std::unordered_map<Node, Weight> dist;
    dist[start]        = 0;
    bool has_neg_cycle = false;
    for (size_t i = 1; i <= node_num; ++i) {
        bool updated = false;
        for (const auto &[u, v, w] : edges) {
            if (!dist.count(u)) { continue; }
            Weight new_dist = dist[u] + w;
            if (!dist.count(v) || new_dist < dist[v]) {
                dist[v] = new_dist;
                updated = true;
            }
        }
        if (!updated) { break; }
        if (i == node_num && updated) { has_neg_cycle = true; }
    }
    return std::make_pair(dist, has_neg_cycle);
}

int main() {
    {
        adjacency_list<int, int> graph;
        graph.add_edge(1, 2, 1);
        graph.add_edge(1, 3, 4);
        graph.add_edge(2, 3, 2);

        const auto dist = dijkstra(1, graph);

        assert(dist.at(1) == 0);
        assert(dist.at(2) == 1);
        assert(dist.at(3) == 3);
        assert(dist.size() == 3);
    }
    {
        adjacency_list<int, int> graph;
        graph.add_edge(1, 2, 1);
        graph.add_edge(3, 4, 2);

        const auto dist = dijkstra(1, graph);

        assert(dist.at(1) == 0);
        assert(dist.at(2) == 1);
        assert(!dist.count(3));
        assert(!dist.count(4));
    }
    {
        std::vector<edge<char, int>> edges = {{'A', 'B', 2}, {'B', 'C', 3}, {'A', 'C', 6}};

        const auto [dist, has_neg_cycle] = bellman_ford('A', edges, 3);

        assert(!has_neg_cycle);
        assert(dist.at('A') == 0);
        assert(dist.at('B') == 2);
        assert(dist.at('C') == 5);
    }
    {
        std::vector<edge<>> edges = {{0, 1, 1}, {1, 2, -2}, {2, 0, -1}};

        const auto [dist, has_neg_cycle] = bellman_ford(0, edges, 3);

        assert(has_neg_cycle);
    }
    {
        std::vector<edge<>> edges = {{0, 1, 3}, {1, 2, -1}, {0, 2, 5}};

        const auto [dist, has_neg_cycle] = bellman_ford(0, edges, 3);

        assert(!has_neg_cycle);
        assert(dist.at(2) == 2);
    }
    std::cout << "ALL TESTS PASSED!" << std::endl;
    return 0;
}
