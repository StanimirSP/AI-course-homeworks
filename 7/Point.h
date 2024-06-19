#ifndef POINT_H
#define POINT_H

inline double sqr(double n)
{
    return n*n;
}

struct Point
{
    double x, y;
    unsigned long cluster;
    double distanceSquared(const Point& p) const
    {
        return sqr(x-p.x)+sqr(y-p.y);
    }
    Point& operator+=(const Point& p)
    {
        x += p.x;
        y += p.y;
        return *this;
    }
    Point operator/(double d) const
    {
        return {x/d, y/d};
    }
    bool operator<(const Point& p) const
    {
        return cluster < p.cluster;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Point& p)
{
    return os << p.cluster << ' ' << p.x << ' ' << p.y;
}

#endif // POINT_H
