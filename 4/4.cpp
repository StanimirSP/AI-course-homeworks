#include <iostream>
#include <utility>
#include <algorithm>
#include <limits>

constexpr unsigned N = 3;

constexpr int infinity = std::numeric_limits<int>::max();
struct Offset
{
    int dr, dc;
    Offset operator-() const
    {
        return Offset{-dr, -dc};
    }
};
struct Position
{
    unsigned row, col;
};

class TicTacToe
{
    static constexpr char symbols[] = {'X', 'O'},
                          players = sizeof symbols;
    char board[N][N];
    bool playerFirst;
    unsigned short moves;
    short result;
    int utility() const
    {
        constexpr int MAX_DEPTH = N*N;
        if(result <= 0) return 0;
        return result == 1+playerFirst? (MAX_DEPTH+1)-moves: moves-(MAX_DEPTH+1);
    }
    std::pair<Position, int> alphabeta(int alpha, int beta)
    {
        Position p{N, N};
        if(result) return {p, utility()};
        int value;
        if(moves%players == playerFirst) // if it's computer's turn
        {
            value = -infinity;
            for(unsigned i=0; i<N; i++)
                for(unsigned j=0; j<N; j++)
                    if(!board[i][j])
                    {
                        makeMove({i, j});
                        auto [_, newValue] = alphabeta(alpha, beta);
                        if(newValue > value)
                        {
                            value = newValue;
                            p = {i, j};
                        }
                        undoMove({i, j});
                        if(value >= beta) return {p, value};
                        alpha = std::max(alpha, value);
                    }
        }
        else
        {
            value = infinity;
            for(unsigned i=0; i<N; i++)
                for(unsigned j=0; j<N; j++)
                    if(!board[i][j])
                    {
                        makeMove({i, j});
                        auto [_, newValue] = alphabeta(alpha, beta);
                        if(newValue < value)
                        {
                            value = newValue;
                            p = {i, j};
                        }
                        undoMove({i, j});
                        if(value <= alpha) return {p, value};
                        beta = std::min(beta, value);
                    }
        }
        return {p, value};
    }
    void makeBestMove()
    {
        makeMove(alphabeta(-infinity, infinity).first);
    }
    bool hasWinner(Position lastPlaced, Offset o) const
    {
        Offset ofs[] = {o, -o};
        for(unsigned dir=0; dir<sizeof ofs/sizeof *ofs; dir++)
            for(unsigned row=lastPlaced.row+ofs[dir].dr, col=lastPlaced.col+ofs[dir].dc; row<N && col<N; row+=ofs[dir].dr, col+=ofs[dir].dc)
                if(board[row][col] != board[lastPlaced.row][lastPlaced.col])
                    return false;
        return true;
    }
    bool hasWinner(Position lastPlaced) const
    {
        return                                         hasWinner(lastPlaced, {0,  1})   // check current column
            ||                                         hasWinner(lastPlaced, {1,  0})   // check current row
            || (lastPlaced.row == lastPlaced.col     && hasWinner(lastPlaced, {1,  1}))   // check main diagonal
            || (lastPlaced.row+lastPlaced.col == N-1 && hasWinner(lastPlaced, {1, -1}));  // check antidiagonal
    }
    void makeMove(Position p)
    {
        board[p.row][p.col] = symbols[moves%players];
        if(hasWinner(p)) result = 1+moves++%players;
        else if(++moves >= sizeof board) result = -1;
    }
    void undoMove(Position p)
    {
        board[p.row][p.col] = result = 0;
        moves--;
    }
public:
    TicTacToe(bool playerFirst = true): board{}, playerFirst(playerFirst), moves(0), result(0)
    {
        if(!playerFirst) makeBestMove();
    }
    bool play(Position p)
    {
        if(p.row >= N || p.col >= N || board[p.row][p.col])
            return false;
        makeMove(p);
        if(!result) makeBestMove();
        return true;
    }
    int winner() const
    {
        return result;
    }
    friend std::ostream& operator<<(std::ostream& os, const TicTacToe& game)
    {
        for(unsigned r=0; r<N; r++)
        {
            os << '|';
            for(unsigned c=0; c<N; c++)
                os << (game.board[r][c]? game.board[r][c]: ' ') << '|';
            os << '\n';
        }
        return os;
    }
};

int main()
{
    std::cout << "Do you want to play first? [y/n] ";
    char c;
    std::cin >> c;
    bool playerFirst;
    switch(c)
    {
    case 'y': case 'Y':
        playerFirst = true;
        break;
    default:
        std::cout << "Bad choice! You will play second!\n";
        [[fallthrough]];
    case 'n': case 'N':
        playerFirst = false;
    }
    TicTacToe game(playerFirst);
    while(!game.winner())
    {
        unsigned r, c;
        std::cout << game;
        std::cout << "Where do you want to play?\n";
        std::cin >> r >> c;
        if(!game.play({r-1, c-1}))
            std::cout << "Illegal move! Try again.\n";
    }
    std::cout << game;
    if(game.winner() < 0) std::cout << "Draw!\n";
    else std::cout << "You " << (game.winner() == 1+playerFirst? "lose": "win") << "!\n";
}
