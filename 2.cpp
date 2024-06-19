#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <cstddef>

template<class T, class Generator>
T randBetween(T a, T b, Generator&& g)
{
    return std::uniform_int_distribution<T>{a, b}(g);
}

class NQueens
{
    std::vector<unsigned> rowOfQueen, queensInRow, queensInMainDiag, queensInAntidiag;
    std::mt19937 mt;
    bool isSolved = false;
    void addQueen(unsigned row, unsigned col, int cnt = 1)
    {
        queensInRow[row] += cnt;
        queensInMainDiag[size()-1+row-col] += cnt;
        queensInAntidiag[row+col] += cnt;
    }
    unsigned getRowWithMinConflict(unsigned col)
    {
        unsigned min = -1;
        std::vector<unsigned> bestRows;
        for(unsigned row=0; row<size(); row++)
        {
            unsigned conflicts = conflictsCnt(row, col);
            if(conflicts < min)
            {
                min = conflicts;
                bestRows.clear();
            }
            if(conflicts == min)
                bestRows.push_back(row);
        }
        return bestRows[randBetween<unsigned>(0, bestRows.size()-1, mt)];
    }
    unsigned conflictsCnt(unsigned row, unsigned col) const
    {
        return queensInRow[row]+queensInMainDiag[size()-1+row-col]+queensInAntidiag[row+col]-3*hasQueen(row, col);
    }
    unsigned getColWithMaxConflict()
    {
        unsigned max = 0;
        std::vector<unsigned> worstCols;
        for(unsigned col=0; col<size(); col++)
        {
            unsigned conflicts = conflictsCnt(rowOfQueen[col], col);
            if(conflicts > max)
            {
                max = conflicts;
                worstCols.clear();
            }
            if(conflicts == max)
                worstCols.push_back(col);
        }
        if(!max) isSolved = true;
        return worstCols[randBetween<unsigned>(0, worstCols.size()-1, mt)];
    }
public:
    NQueens(std::size_t n, unsigned long seed = 1): queensInRow(n), queensInMainDiag(2*n-1), queensInAntidiag(2*n-1), mt(seed)
    {
        if(n==2 || n==3)
            throw std::logic_error("no solution");
        rowOfQueen.reserve(n);
        placeQueens();
    }
    unsigned size() const
    {
        return queensInRow.size();
    }
    bool hasQueen(unsigned row, unsigned col) const
    {
        return row == rowOfQueen[col];
    }
    void placeQueens()
    {
        std::fill(queensInRow.begin(), queensInRow.end(), 0);
        std::fill(queensInMainDiag.begin(), queensInMainDiag.end(), 0);
        std::fill(queensInAntidiag.begin(), queensInAntidiag.end(), 0);
        /*for(unsigned col=0; col<size(); col++)
            addQueen(rowOfQueen[col] = getRowWithMinConflict(col), col);*/
        unsigned col = 1;
        for(unsigned row=0; row<size(); row++)
        {
            rowOfQueen[col] = row;
            addQueen(row, col);
            col += 2;
            if(col >= size())
                col = 0;
        }
    }
    bool solve(unsigned maxSteps)
    {
        while(maxSteps--)
        {
            unsigned col = getColWithMaxConflict();
            moveQueen(col, rowOfQueen[col], getRowWithMinConflict(col));
            if(isSolved) return true;
        }
        return false;
    }
    void moveQueen(unsigned inCol, unsigned fromRow, unsigned toRow)
    {
        addQueen(fromRow, inCol, -1);
        rowOfQueen[inCol] = toRow;
        addQueen(toRow, inCol);
    }
    friend std::ostream& operator<<(std::ostream& os, const NQueens& obj)
    {
        for(unsigned i=0; i<obj.size(); i++)
        {
            for(unsigned j=0; j<obj.size(); j++)
                os << (obj.hasQueen(i, j)? '*': '_');
            os << '\n';
        }
        return os;
    }
};

int main() try
{
    std::size_t n;
    std::cin >> n;
    auto start = std::chrono::steady_clock::now();
    NQueens board(n, std::chrono::system_clock::now().time_since_epoch().count());
    while(!board.solve(2*n))
        board.placeQueens();
    auto end = std::chrono::steady_clock::now();
    //std::cout << board;
    std::cerr.precision(6);
    std::cerr << "elapsed time: " << std::fixed << std::chrono::duration<double>(end-start).count() << " s\n";
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
