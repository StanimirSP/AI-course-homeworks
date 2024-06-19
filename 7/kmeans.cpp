#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cerrno>
#include <stdexcept>
#include <random>
#include <chrono>
#include <limits>
#include "Point.h"

#define kmeans_plusplus
constexpr unsigned RESTART_CNT = 100;

unsigned long strtoul_s(const char* str, int base = 10)
{
    char *end;
    errno = 0;
    unsigned long res = std::strtoul(str, &end, base);
    if(*end || errno)
        throw std::invalid_argument(std::string(str) + ": conversion to unsigned long failed");
    return res;
}

std::vector<Point> readData(std::istream& is)
{
    std::vector<Point> data;
    Point p{};
    while(is >> p.x >> p.y)
        data.push_back(p);
    return data;
}

template<class Generator>
class KMeans
{
    std::vector<Point> points, centroids;
    Generator& gen;
    void makeCentroid(Point& p)
    {
        centroids.push_back(p);
        p.cluster = centroids.size();
    }
    void initRandom(unsigned long K)
    {
        std::uniform_int_distribution<unsigned long> dist(0, points.size()-1);
        while(centroids.size() < K)
            if(unsigned long randIndex = dist(gen); !points[randIndex].cluster)
                makeCentroid(points[randIndex]);
    }
    std::size_t findClosestCentroid(const Point& p) const
    {
        std::size_t closestCentroidInd = 0;
            for(std::size_t i=1; i<centroids.size(); i++)
                if(p.distanceSquared(centroids[i]) < p.distanceSquared(centroids[closestCentroidInd]))
                    closestCentroidInd = i;
        return closestCentroidInd;
    }
    void initPlusPlus(unsigned long K)
    {
        std::uniform_int_distribution<unsigned long> dist(0, points.size()-1);
        makeCentroid(points[dist(gen)]);
        std::vector<double> weights;
        weights.reserve(points.size());
        while(centroids.size() < K)
        {
            for(const Point& p: points)
                weights.push_back(p.distanceSquared(centroids[findClosestCentroid(p)]));
            std::discrete_distribution<unsigned long> dist(weights.begin(), weights.end());
            makeCentroid(points[dist(gen)]);
            weights.clear();
        }
    }
    void repositionCentroids()
    {
        std::vector<std::pair<Point, unsigned long>> newCentroids(centroids.size());
        for(const Point& p: points)
        {
            newCentroids[p.cluster-1].first += p;
            newCentroids[p.cluster-1].second++;
        }
        for(std::size_t i=0; i<centroids.size(); i++)
            centroids[i] = newCentroids[i].first/newCentroids[i].second;
    }
    bool repartitionClusters()
    {
        bool clusterChanged = false;
        for(Point& p: points)
        {
            std::size_t closestCentroid = findClosestCentroid(p)+1;
            if(p.cluster != closestCentroid)
            {
                p.cluster = closestCentroid;
                clusterChanged = true;
            }
        }
        return clusterChanged;
    }
public:
    KMeans(const std::vector<Point>& points, unsigned long K, Generator& gen): points(points), gen(gen)
    {
        if(K <= 0 || K > points.size())
            throw std::invalid_argument("invalid K: should be greater than 0 and not greater that the number of points");
#       ifdef kmeans_plusplus
            initPlusPlus(K);
#       else
            initRandom(K);
#       endif // kmeans_plusplus
        repartitionClusters();
    }
    const std::vector<Point>& execute()
    {
        if(points.empty())
            throw std::logic_error("algorithm already executed");
        do
            repositionCentroids();
        while(repartitionClusters());
        return points;
    }
    double WCSS() const
    {
        double sum = 0;
        for(const Point& p: points)
            sum += p.distanceSquared(centroids[p.cluster-1]);
        return sum;
    }
};

std::ostream& print(std::ostream& os, const std::vector<Point>& points)
{
    for(const Point& p: points)
        os << p << '\n';
    return os;
}

std::vector<Point> minimizeWCSS(const std::vector<Point>& data, unsigned long K)
{
    std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
    std::vector<Point> best;
    double minSum = std::numeric_limits<double>::infinity();
    for(unsigned i=0; i<=RESTART_CNT; i++)
    {
        KMeans kmeans(data, K, mt);
        auto& tmp = kmeans.execute();
        if(double nextSum = kmeans.WCSS(); nextSum < minSum)
        {
            minSum = nextSum;
            best = tmp;
        }
    }
    return best;
}

int main(int argc, char** argv) try
{
    if(argc != 3)
        throw std::invalid_argument(std::string("Usage: ") + *argv + " <data file> <K>");
    unsigned long K = strtoul_s(argv[2]);
    std::vector<Point> data;
    {
        std::ifstream ifs(argv[1]);
        if(!ifs)
            throw std::runtime_error(std::string("could not open ") + argv[1] + " for reading");
        data = readData(ifs);
    }
    print(std::cout, minimizeWCSS(data, K));
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
