#include <iostream>
#include <vector>
#include <cstdlib>
#include <utility>
#include <stack>
#include <chrono>
#include <stdexcept>

typedef std::vector<std::vector<unsigned>> Board;

unsigned long long sqrtLong(unsigned long long n)
{
    unsigned long long l = 0, r = 1LL<<32;
    while(l < r)
    {
        unsigned long long mid = l+(r-l)/2, sqr = mid*mid;
        if(sqr == n) return mid;
        if(sqr < n) l = mid+1;
        else r = mid;
    }
    return l-1;
}

enum Direction
{
    LEFT, RIGHT, UP, DOWN, CNT, NONE
};
Direction oposite(Direction d)
{
    switch(d)
    {
    case Direction::LEFT:
        return Direction::RIGHT;
    case Direction::RIGHT:
        return Direction::LEFT;
    case Direction::UP:
        return Direction::DOWN;
    case Direction::DOWN:
        return Direction::UP;
    case Direction::NONE:
        return d;
    default:
        throw std::logic_error("bad direction");
    }
}
constexpr const char* strings[] = {"left", "right", "up", "down", "???", "none"};
struct Position
{
    int row, col;
};

class State
{
    Board board;
    Position currentEmptyPos;
    unsigned emptyAtTheEnd, manhattan;
    std::vector<Position> m_finalPositions;
    void recalcTotalManhattan()
    {
        manhattan = 0;
        int n = board.size();
        for(int i=0; i<n; i++)
            for(int j=0; j<n; j++)
                if(board[i][j])
                    manhattan += manhattanDist(board[i][j], i, j);
                else currentEmptyPos = {i, j};
    }
    unsigned manhattanDist(unsigned tileNum, int currentRow, int currentCol) const
    {
        return std::abs(currentRow-m_finalPositions[tileNum-1].row) + std::abs(currentCol-m_finalPositions[tileNum-1].col);
    }
    unsigned getTargetPosInd(unsigned tileNum) const
    {
        return tileNum - (tileNum <= emptyAtTheEnd);
    }
    unsigned countInversions() const
    {
        unsigned inversions = 0;
        std::size_t cells = board.size()*board.size();
        for(std::size_t i=0; i<cells; i++)
            for(std::size_t j=i+1; j<cells; j++)
            {
                unsigned tile1 = at(i), tile2 = at(j);
                if(tile1>tile2 && tile1 && tile2)
                    inversions++;
            }
        return inversions;
    }
    Position getPosition(std::size_t ind) const
    {
        auto res = std::ldiv(ind, board.size());
        return {(int)res.quot, (int)res.rem};
    }
    unsigned& at(std::size_t ind)
    {
        Position pos = getPosition(ind);
        return board[pos.row][pos.col];
    }
    unsigned at(std::size_t ind) const
    {
        Position pos = getPosition(ind);
        return board[pos.row][pos.col];
    }
    std::size_t row(std::size_t ind) const
    {
        return ind/board.size();
    }
    std::size_t col(std::size_t ind) const
    {
        return ind%board.size();
    }
    std::pair<unsigned, bool> search(std::vector<Direction>& st, unsigned g, unsigned bound)
    {
        unsigned f = g + manhattan;
        if(isSolved()) return {f, true};
        if(f > bound) return {f, false};
        unsigned min = -1;
        for(int d=Direction::CNT-1; d>=0; d--)
        //for(int d=0; d<Direction::CNT; d++)
        {
            Direction dir = static_cast<Direction>(d);
            if(!st.empty() && dir == oposite(st.back()) || !makeMove(dir)) continue;
            st.push_back(dir);
            auto t = search(st, g+1, bound);
            if(t.second) return {f, true};
            if(t.first < min) min = t.first;
            st.pop_back();
            makeMove(oposite(dir));
        }
        return {min, false};
    }
    void memoizeFinalPositions()
    {
        std::size_t k = board.size()*board.size();
        m_finalPositions.reserve(k-1);
        for(std::size_t tileNum=1; tileNum<k; tileNum++)
            m_finalPositions.push_back(getPosition(getTargetPosInd(tileNum)));
    }
public:
    State(Board board, unsigned emptyAtTheEnd): board(std::move(board)), emptyAtTheEnd(emptyAtTheEnd)
    {
        memoizeFinalPositions();
        recalcTotalManhattan();
    }
    bool isSolvable() const
    {
        unsigned inversions = countInversions();
        return board.size()%2 && !(inversions%2)
            || !(board.size()%2) && (inversions+currentEmptyPos.row)%2 == row(emptyAtTheEnd)%2;
    }
    bool isSolved() const
    {
        return !manhattan;
    }
    bool makeMove(Direction d)
    {
        if(d == Direction::LEFT && currentEmptyPos.col == board.size()-1
        || d == Direction::RIGHT && !currentEmptyPos.col
        || d == Direction::UP && currentEmptyPos.row == board.size()-1
        || d == Direction::DOWN && !currentEmptyPos.row
          ) return false;
        Position newEmptyPos = currentEmptyPos;
        switch(d)
        {
        case Direction::LEFT:
            newEmptyPos.col++;
            break;
        case Direction::RIGHT:
            newEmptyPos.col--;
            break;
        case Direction::UP:
            newEmptyPos.row++;
            break;
        case Direction::DOWN:
            newEmptyPos.row--;
            break;
        default:
            throw std::logic_error("bad direction");
        }
        std::swap(board[currentEmptyPos.row][currentEmptyPos.col], board[newEmptyPos.row][newEmptyPos.col]);
        manhattan -= manhattanDist(board[currentEmptyPos.row][currentEmptyPos.col], newEmptyPos.row, newEmptyPos.col);
        manhattan += manhattanDist(board[currentEmptyPos.row][currentEmptyPos.col], currentEmptyPos.row, currentEmptyPos.col);
        currentEmptyPos = newEmptyPos;
        return true;
    }
    std::vector<Direction> solve()
    {
        /*if(!isSolvable())
            throw std::logic_error("No solution");*/
        std::vector<Direction> st;
        unsigned bound = manhattan;
        for(;;)
        {
            auto [newBound, found] = search(st, 0, bound);
            if(found) break;
            bound = newBound;
        }
        return st;
    }
};

Board readBoard(std::istream& is, std::size_t k)
{
    const unsigned maxTileNum = k*k-1;
    Board b(k);
    for(std::size_t i=0; i<k; i++)
    {
        b[i].reserve(k);
        for(std::size_t j=0; j<k; j++)
        {
            unsigned t;
            is >> t;
            if(t > maxTileNum)
                throw std::logic_error("bad tile number");
            b[i].push_back(t);
        }
    }
    return b;
}

int main()
{
    std::size_t n;
    std::cin >> n;
    std::size_t k = sqrtLong(n+1);
    try
    {
        if(n != k*k-1)
            throw std::logic_error("n != k*k - 1");
        int emptyPos;
        std::cin >> emptyPos;
        if(emptyPos == -1) emptyPos = n;
        if(emptyPos < 0 || static_cast<std::size_t>(emptyPos) > n)
            throw std::logic_error("index out of the board");
        Board b(readBoard(std::cin, k));
        auto start = std::chrono::steady_clock::now();
        State initial(std::move(b), emptyPos);
        if(!initial.isSolvable())
        {
            std::cout << "No solution\n";
            return 0;
        }
        std::vector<Direction> path = initial.solve();
        auto end = std::chrono::steady_clock::now();
        std::cout << path.size() << '\n';
        for(Direction d: path)
            std::cout << strings[d] << '\n';
        std::cerr.precision(6);
        std::cerr << "elapsed time: " << std::fixed << std::chrono::duration<double>(end-start).count() << " s\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}


/*
8 5
1 2 3
4 5 6
0 7 8

8 -1
6 5 1
2 0 8
3 4 7

15 -1
7 1 3 8
2 6 4 12
5 10 0 11
9 13 14 15

24 8
8 20 7 1 9
22 18 2 19 15
11 0 14 3 13
21 10 16 4 23
17 6 12 5 24

15 4
15 14 7 13
5 9 2 4
6 10 11 8
3 1 0 12

*/
