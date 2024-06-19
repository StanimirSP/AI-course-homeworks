#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <stdexcept>
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <utility>
#include <iomanip>

constexpr double      MIN_COORDINATE = 0,
                      MAX_COORDINATE = 1000,
                      MUTATION_PROBABILITY = 0.1,
                      KEEP_PARENT_PROBABILITY = 0.3;
constexpr std::size_t POPULATION_SIZE = 20000,
                      ITERATIONS = 2000,
                      MAX_POINTS = 1000;

double sqr(double n)
{
    return n*n;
}
struct Point
{
    double x, y;
    double distance(const Point& b) const
    {
        return std::sqrt(sqr(x-b.x)+sqr(y-b.y));
    }
};

template<class Generator>
std::vector<Point> randPoints(std::size_t cnt, double min, double max, Generator&& gen)
{
    std::uniform_real_distribution<> dist(min, max);
    std::vector<Point> res;
    res.reserve(cnt);
    while(cnt--)
        res.push_back({dist(gen), dist(gen)});
    return res;
}

class TSP_Map
{
    std::vector<std::vector<double>> distsSqr;
    void initDistances(const std::vector<Point>& cities)
    {
        for(std::size_t i=0; i<cities.size(); i++)
        {
            distsSqr.reserve(i+1);
            for(std::size_t j=0; j<i; j++)
                distsSqr[i].push_back(cities[i].distance(cities[j]));
            distsSqr[i].push_back(0);
        }
    }
public:
    TSP_Map(const std::vector<Point>& cities): distsSqr(cities.size())
    {
        initDistances(cities);
    }
    double distance(int i, int j) const
    {
        return j<=i? distsSqr[i][j]: distsSqr[j][i];
    }
    std::size_t cities() const
    {
        return distsSqr.size();
    }
};

class Path
{
    const TSP_Map& map;
    std::vector<int> path;
    double len;
    double calcLength() const
    {
        double len = 0;
        for(std::size_t i=1; i<path.size(); i++)
            len += map.distance(path[i-1], path[i]);
        return len;
    }
    Path createChild(const Path& parent2, unsigned start, unsigned end) const
    {
        std::vector<int> newPath(path.size());
        std::vector<bool> usedCities(path.size());
        for(unsigned i=start; i<=end; i++)
            usedCities[newPath[i] = path[i]] = true;
        auto copyFrom = parent2.path.begin()+end+1;
        auto updateCopyFrom = [&copyFrom, &parent2, &usedCities] () -> decltype((copyFrom)) {
            if(copyFrom == parent2.path.end()) copyFrom = parent2.path.begin();
            while(usedCities[*copyFrom])
                if(++copyFrom == parent2.path.end()) copyFrom = parent2.path.begin();
            return copyFrom;
        };
        for(unsigned i=end+1; i<newPath.size(); i++)
            newPath[i] = *updateCopyFrom()++;
        for(unsigned i=0; i<start; i++)
            newPath[i] = *updateCopyFrom()++;
        return {map, newPath};
    }
public:
    Path(const TSP_Map& map, const std::vector<int>& path): map(map), path(path), len(calcLength()) {}

    double length() const
    {
        return len;
    }
    std::size_t cities() const
    {
        return path.size();
    }
    template<class Generator>
    void mutate(Generator&& gen)
    {
        std::uniform_int_distribution<unsigned> dist(0, path.size()-1);
        std::swap(path[dist(gen)], path[dist(gen)]);
        len = calcLength();
    }
    template<class Generator>
    std::pair<Path, Path> crossover(const Path& parent2, Generator&& gen) const
    {
        if(&map != &parent2.map)
            throw std::logic_error("cannot crossover paths over different maps");
        std::uniform_int_distribution<unsigned> dist(0, path.size()-1);
        unsigned i = dist(gen), j = dist(gen);
        if(i>j) std::swap(i, j);
        return { createChild(parent2, i, j), parent2.createChild(*this, i, j) };
    }
};


class Population
{
    std::vector<Path> members;
    std::vector<double> totalFitnessUntil;
    Path* best;
    template<class Generator>
    Path& chooseMember(Generator&& gen)
    {
        return const_cast<Path&>(const_cast<const Population*>(this)->chooseMember(gen));
    }
    template<class Generator>
    const Path& chooseMember(Generator&& gen) const
    {
        std::uniform_real_distribution<> naturalSelection(0, totalFitnessUntil.back());
        return members[std::lower_bound(totalFitnessUntil.begin(), totalFitnessUntil.end(), naturalSelection(gen)) - totalFitnessUntil.begin() - 1];
    }
public:
    template<class Generator>
    static Population createInitial(std::size_t size, const TSP_Map& map, Generator&& gen)
    {
        std::vector<Path> members;
        std::vector<int> path(map.cities());
        std::iota(path.begin(), path.end(), 0);
        members.reserve(size);
        while(size--)
        {
            std::shuffle(path.begin(), path.end(), gen);
            members.emplace_back(map, path);
        }
        return members;
    }
    Population(std::vector<Path> members): members(std::move(members)), best(&this->members.at(0))
    {
        totalFitnessUntil.reserve(size()+1);
        totalFitnessUntil.push_back(-0.0);
        for(Path& path: this->members)
        {
            totalFitnessUntil.push_back(totalFitnessUntil.back()+path.length());
            if(path.length() < best->length())
                best = &path;
        }
    }
    template<class Generator>
    Population children(Generator&& gen) const
    {
        std::bernoulli_distribution hasMutation(MUTATION_PROBABILITY);
        std::vector<Path> children;
        while(children.size() < size())
        {
            auto [child1, child2] = chooseMember(gen).crossover(chooseMember(gen), gen);
            if(hasMutation(gen)) child1.mutate(gen);
            if(hasMutation(gen)) child2.mutate(gen);
            children.push_back(std::move(child1));
            children.push_back(std::move(child2));
        }
        return children;
    }
    template<class Generator>
    Population select(Population&& children, Generator&& gen)
    {
        std::bernoulli_distribution keepParent(KEEP_PARENT_PROBABILITY);
        std::vector<Path> newMembers;
        newMembers.reserve(size());
        newMembers.push_back(std::move(*best));
        newMembers.push_back(std::move(*children.best));
        while(newMembers.size() < size())
        {
            Path& chosenMember = keepParent(gen)? chooseMember(gen): children.chooseMember(gen);
            if(chosenMember.cities()) // if member has not been chosen yet
                newMembers.push_back(std::move(chosenMember));
        }
        return newMembers;
    }
    const Path& bestMember() const
    {
        return *best;
    }
    std::size_t size() const
    {
        return members.size();
    }
};

const std::vector<Point> testPoints {
    { 0.000190032,-0.000285946 },
    { 383.458,-0.000608756 },
    { -27.0206,-282.758 },
    { 335.751,-269.577 },
    { 69.4331,-246.780 },
    { 168.521,31.4012 },
    { 320.350,-160.900 },
    { 179.933,-318.031 },
    { 492.671,-131.563 },
    { 112.198,-110.561 },
    { 306.320,-108.090 },
    { 217.343,-447.089 },
};

int main() try
{
    //std::size_t n;
    //std::cin >> n;
    auto start = std::chrono::steady_clock::now();

    std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
    const TSP_Map map(/*randPoints(n, MIN_COORDINATE, MAX_COORDINATE, mt)*/testPoints);
    Population p = Population::createInitial(POPULATION_SIZE, map, mt);
    std::cout << std::fixed;
    std::cout.precision(3);
    for(std::size_t i=0; i<ITERATIONS; i++)
    {
        if(!(i%(ITERATIONS/20)))
            std::cout << "after iteration " << std::setw(8) << i << ": " << std::setw(10) << p.bestMember().length() << '\n';
        p = p.select(p.children(mt), mt);
    }

    auto end = std::chrono::steady_clock::now();
    std::cout << "after iteration " << std::setw(8) << ITERATIONS << ": " << std::setw(10) << p.bestMember().length() << '\n';
    std::cerr.precision(6);
    std::cerr << "elapsed time: " << std::fixed << std::chrono::duration<double>(end-start).count() << " s\n";
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
