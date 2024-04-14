#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdio.h>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <thread>

using Clock = std::chrono::steady_clock;
using namespace std::literals::chrono_literals;

std::random_device seed;
std::default_random_engine rnd(seed());

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int CIRCLE_RADIUS = 3;
const int MARGIN_WIDTH = SCREEN_WIDTH / 10;
const int MARGIN_HEIGHT = SCREEN_HEIGHT / 10;

struct Point
{
    int x;
    int y;

    bool operator==(const Point& other) const
    {
        return x == other.x && y == other.y;
    }
};

using Edge = std::pair<Point, Point>;

std::vector<Point>::const_iterator find_leftmost_point(const std::vector<Point>& points)
{
    auto it = points.begin();
    auto best = it++;

    while (it != points.end())
    {
        if (it->x < best->x)
        {
            best = it;
        }

        ++it;
    }

    return best;
}

std::vector<Point> generate_random_points(int n)
{
    std::vector<Point> rval;

    for (int i = 0; i < n; i++)
    {
        std::uniform_int_distribution<> rnd_x(MARGIN_WIDTH, SCREEN_WIDTH - MARGIN_WIDTH);
        std::uniform_int_distribution<> rnd_y(MARGIN_HEIGHT, SCREEN_HEIGHT - MARGIN_HEIGHT);
        rval.emplace_back(rnd_x(rnd), rnd_y(rnd));
    }

    return rval;
}

double cross_product(Point a, Point b)
{
    return a.x * b.y - b.x * a.y;
}

bool is_lefter_than(Point p_prev, Point p_best, Point p_cand)
{
    Point v1 {p_best.x - p_prev.x, p_best.y - p_prev.y};
    Point v2 {p_cand.x - p_prev.x, p_cand.y - p_prev.y};
    return cross_product(v1, v2) >= 0;
}

enum State {
    INIT,
    FIND_START,
    FIND,
    FIND_END,
    DONE,
};

int main(int argc, char** argv)
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        return 1;
    }

    int num_points = 10;
    int delay = 250;

    if (argc > 1)
    {
        num_points = atoi(argv[1]);
    }

    if (argc > 2)
    {
        delay = atoi(argv[2]);
    }

    SDL_Window* window = SDL_CreateWindow("Delaunay Triangulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if(window)
    {
        SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, 0);

        //Update the surface
        SDL_UpdateWindowSurface(window);

        std::vector<Point> points = generate_random_points(num_points);
        std::vector<Point> hull;
        std::vector<Edge>  edges;

        SDL_Event e;
        bool quit = false;
        int x = 0, y = 0, incx = 1, incy = 1;
        auto prev = Clock::now();
        State state = INIT;
        auto it = points.begin();
        auto best = it;

        auto do_step = [&](){
            switch (state)
            {
            case INIT:
                {
                    hull.emplace_back(*find_leftmost_point(points));
                    state = FIND_START;
                }
                break;

            case FIND_START:
                {
                    it = points.begin();
                    best = it;
                    state = FIND;
                }
                [[fallthrough]];

            case FIND:
                {
                    Point p1 = hull.back();
                    Point p2 = *it;

                    if (*it != p1)
                    {
                        if (is_lefter_than(hull.back(), *it, *best))
                        {
                            best = it;
                        }
                    }

                    if (++it == points.end())
                    {
                        state = FIND_END;
                    }
                }
                break;

            case FIND_END:
                {
                    hull.emplace_back(*best);

                    if (*best == hull.front())
                    {
                        it = points.end();
                        best = points.end();
                        state = DONE;
                    }
                    else
                    {
                        it = points.begin();
                        best = points.begin();
                        state = FIND_START;
                    }
                }
                break;

            case DONE:
                break;
            }
        };



        while(!quit)
        {
            while(SDL_PollEvent(&e))
            {
                if(e.type == SDL_QUIT)
                {
                    quit = true;
                }
            }

            if (auto now = Clock::now(); now - prev > std::chrono::milliseconds(delay))
            {
                prev = now;
                do_step();
            }

            SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderClear(renderer);

            for (size_t i = 1; i < hull.size(); i++)
            {
                const auto& p1 = hull[i - 1];
                const auto& p2 = hull[i];
                lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, 0xff, 0x0, 0x0, 0xff);
            }

            if (it != points.end() && !hull.empty())
            {
                auto p1 = hull.back();
                auto p2 = *it;
                lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, 0x00, 0x00, 0x00, 0xff);
                p2 = *best;
                lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, 0x00, 0xff, 0x00, 0xff);
            }


            for (const auto& [p1, p2] : edges)
            {
                lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, 0xff, 0x0, 0x0, 0xff);
            }

            for (const auto& p : points)
            {
                uint32_t color = 0xff000000;

                if (!hull.empty() && p == hull.front())
                {
                    color = 0xffff0000;
                }

                filledCircleColor(renderer, p.x, p.y, CIRCLE_RADIUS, color);
            }

            SDL_RenderPresent(renderer);
        }

        SDL_DestroyWindow(window);
    }

    SDL_Quit();
    return 0;
}
