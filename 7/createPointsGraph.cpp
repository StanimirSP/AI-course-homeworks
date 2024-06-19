#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstddef>
#include "Point.h"

std::vector<Point> readData(std::istream& is)
{
    std::vector<Point> data;
    Point p;
    while(is >> p.cluster >> p.x >> p.y)
        data.push_back(p);
    return data;
}

constexpr const char *HEADER = R"([Graph]
Version = 4.4.2.543
MinVersion = 2.5
OS = Windows NT 6.2)";
constexpr const char *COLORS[] = {
    "0x00FF0000",
    "0x0000FF00",
    "0x000000FF",
    "0x00FF0066",
    "0x00000000",
    "0x001F5FFF",
    "0x00005E34",
    "0x008E8E8E",
};
constexpr std::size_t COLORS_CNT = sizeof COLORS / sizeof *COLORS;

std::ostream& printPointSeriesMetaData(std::ostream& os, unsigned index, const char* fillColor)
{
    return os << "[PointSeries" << index << "]\n"
              << "FillColor = " << fillColor << "\n"
              << "Size = 5\n"
              << "Style = 6\n"
              << "Points = ";
}

std::ostream& printPointSeries(std::ostream& os, const std::vector<Point>& points)
{
    unsigned lastCluster = 0;
    for(const Point& p: points)
    {
        if(lastCluster != p.cluster)
            printPointSeriesMetaData(os << '\n', p.cluster, COLORS[lastCluster++%COLORS_CNT]);
        os << p.x << ',' << p.y << ';';
    }
    return os;
}

int main() try
{
    std::vector<Point> points = readData(std::cin);
    std::sort(points.begin(), points.end());
    printPointSeries(std::cout << HEADER, points) << '\n';
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
