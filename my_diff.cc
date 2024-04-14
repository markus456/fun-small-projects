#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <ncurses.h>
#include <chrono>
#include <thread>
#include <cassert>

namespace fs = std::filesystem;
using namespace std::chrono_literals;
using namespace std::string_literals;

const char CHAR_TABLE[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.";

class CursesInit
{
public:
    CursesInit()
    {
        initscr();
        start_color();
        cbreak();
        noecho();
    }

    ~CursesInit()
    {
        endwin();
    }
};

std::string read_file(fs::path filename)
{
    std::ostringstream ss;
    ss << std::ifstream(filename).rdbuf();
    return ss.str();
}

void draw(const std::string& left, const std::string& right,
          std::vector<std::vector<int>> lcs, std::vector<std::vector<int>> attr,
          const std::string& output, const std::string& status = "")
{
    erase();

    for (int i = 0; i < left.size(); i++)
    {
        mvaddch(0, 1 + i, left[i]);
    }

    for (int i = 0; i < right.size(); i++)
    {
        mvaddch(1 + i, 0, right[i]);
    }

    for (size_t i = 0; i < left.size(); i++)
    {

        for (size_t j = 0; j < right.size(); j++)
        {
            int c = CHAR_TABLE[std::min(lcs[i][j], (int)(sizeof(CHAR_TABLE) - 2))];
            c |= attr[i][j];
            mvaddch(j + 1, i + 1, c);
        }
    }

    if (!output.empty())
    {
        int line = 0;
        for (std::string msg : {
                "LCS:    " + std::to_string(lcs[left.size() - 1][right.size() - 1]),
                "Left:   " + left,
                "Right:  " + right,
                "Out:    " + output,
                "Status: " + status,
            })
        {
            mvaddstr(right.size() + 5 + line++, 0, msg.c_str());
        }
    }

    refresh();
}

void replace_newlines(std::string& str)
{
    for (char& c : str)
    {
        if (isspace(c))
        {
            c = '^';
        }
    }
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " FILE1 FILE2" << std::endl;
        return 1;
    }

    std::vector<fs::path> filenames = {argv[1], argv[2]};

    for (const auto& f : filenames)
    {
        if (!fs::exists(f))
        {
            std::cout << "Error: File " << f << " not found." << std::endl;
            return 1;
        }
    }

    double sleep_time = 0;
    double sleep_time_backtrack = 0;

    if (argc > 3)
    {
        sleep_time = atof(argv[3]);
        sleep_time_backtrack = sleep_time;
    }

    if (argc > 4)
    {
        sleep_time_backtrack = atof(argv[4]);
    }

    CursesInit curses_init;
    std::string output;
    std::string left = read_file(filenames[0]);
    std::string right = read_file(filenames[1]);
    std::vector<std::vector<int>> lcs;
    lcs.resize(left.size());

    for (auto& l : lcs)
    {
        l.resize(right.size());
    }

    auto lcs_attr = lcs;
    draw(left, right, lcs, lcs_attr, output);
    //getch();

    for (size_t i = 1; i < left.size(); i++)
    {
        for (size_t j = 1; j < right.size(); j++)
        {
            if (left[i - 1] == right[j - 1])
            {
                lcs[i][j] = lcs[i - 1][j - 1] + 1;
            }
            else
            {
                lcs[i][j] = std::max(lcs[i - 1][j], lcs[i][j - 1]);
            }

            if (sleep_time > 0)
            {
                lcs_attr[i][j] = A_REVERSE;
                draw(left, right, lcs, lcs_attr, output);
                lcs_attr[i][j] = 0;
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
            }
        }
    }

    replace_newlines(left);
    replace_newlines(right);
    output.assign(std::max(left.size(), right.size()), ' ');
    draw(left, right, lcs, lcs_attr, output, "Press enter to backtrace");

    getch();

    int x = left.size() - 1;
    int y = right.size() - 1;

    while (x > 0 || y > 0)
    {
        std::ostringstream ss;
        ss << "x: " << x << " '" << left[std::max(x, 0)] << "' y: " << y << " '"<< right[std::max(y, 0)] << "'";
        lcs_attr[std::max(x, 0)][std::max(y, 0)] = A_REVERSE;

        if (x >= 0 && y >= 0 && left[x] == right[y])
        {
            if (y > x)
            {
                output[y] = right[y];
            }
            else
            {
                output[x] = left[x];
            }
            x--;
            y--;
        }
        else if (y > 0 && (x <= 0 || lcs[x - 1][y] < lcs[x][y - 1]))
        {
            output[y] = '+';
            y--;

            if (y == 0)
            {
                output[0] = '+';
            }
        }
        else if (x > 0 && (y <= 0 || lcs[x - 1][y] >= lcs[x][y - 1]))
        {
            output[x] = '-';
            x--;

            if (x == 0)
            {
                output[0] = '-';
            }
        }

        if (sleep_time_backtrack > 0)
        {
            draw(left, right, lcs, lcs_attr, output, ss.str());
            std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time_backtrack));
            std::this_thread::sleep_for(50ms);
        }
    }


    draw(left, right, lcs, lcs_attr, output, "Done");
    getch();



    return 0;
}
