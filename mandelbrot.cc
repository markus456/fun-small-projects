#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdio.h>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <deque>
#include <algorithm>
#include <thread>
#include <assert.h>
#include <mandelbrot_ispc.h>

using Clock = std::chrono::steady_clock;
using namespace std::literals::chrono_literals;

std::random_device seed;
std::default_random_engine rnd(seed());

//Screen dimension
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;

double re_low = -2.0;
double re_high = 1.0;
double im_low = -0.5;
double im_high = 0.5;
double re_offset = 0.0;
double im_offset = 0.0;
double zoom = 0.0;

std::deque<std::mutex> line_locks;

struct Point
{
    int x;
    int y;

    bool operator==(const Point& other) const
    {
        return x == other.x && y == other.y;
    }
};

int mandelbrot(double re0, double im0, int limit)
{
    int i;
    double re = re0;
    double im = im0;

    for (i = 0; i < limit; i++)
    {
        if (re * re + im * im > 4.0)
        {
            break;
        }

        double next_re = re * re - im * im;
        double next_im = 2.0 * re * im;
        re = re0 + next_re;
        im = im0 + next_im;
    }

    return i;
}

std::array funcs {
    mandelbrot
};

auto fn = funcs[0];


void render_thread(int y_start, int y_end, std::vector<uint32_t>& data)
{
    int p = y_start * SCREEN_WIDTH;
    int end = y_end * SCREEN_WIDTH;

    for (; p < end; p++)
    {
        int x = p % SCREEN_WIDTH;
        int y = p / SCREEN_WIDTH;
        double x_ratio = (double)x / (double)SCREEN_WIDTH;
        double y_ratio = (double)y / (double)SCREEN_HEIGHT;
        double re = (re_low + re_offset) + (re_high + re_offset) * x_ratio;
        double im = (im_low + im_offset) + (im_high + im_offset) * y_ratio;
        //std::lock_guard guard(line_locks[y]);
        data[p] = fn(re, im, 1000) == 1000 ? 0xff00ff00 : 0xff0000ff;
    }
}

int main(int argc, char** argv)
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        return 1;
    }

    SDL_DisplayMode dm;

    if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
    {
        printf("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
        return 1;
    }

    SCREEN_WIDTH = dm.w * 0.75;
    SCREEN_HEIGHT = dm.h * 0.75;

    // std::cout << "original x: " << dm.w << std::endl;
    // std::cout << "original y: " << dm.h << std::endl;
    // std::cout << "scaled x: " << SCREEN_WIDTH << std::endl;
    // std::cout << "scaled y: " << SCREEN_HEIGHT << std::endl;

    int delay = 10;

    if (argc > 1)
    {
        delay = atoi(argv[2]);
    }

    if (argc > 2)
    {
        fn = funcs[atoi(argv[2]) % funcs.size()];
    }

    SDL_Window* window = SDL_CreateWindow("Mandelbrot Set", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if(window)
    {
        SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, 0);

        // Update the surface
        SDL_UpdateWindowSurface(window);

        SDL_Event e;
        bool quit = false;
        auto prev = Clock::now();
        bool done = false;

        std::vector<int32_t> data(SCREEN_WIDTH * SCREEN_HEIGHT, 0xffffffff);
        SDL_Texture* buffer = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_RGBA32,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                SCREEN_WIDTH, SCREEN_HEIGHT);
        assert(buffer);
        std::vector<std::thread> threads;
        const size_t line_size = SCREEN_WIDTH * sizeof(uint32_t);
        const int y_lines = (SCREEN_WIDTH * SCREEN_HEIGHT) / SCREEN_WIDTH;
        line_locks.resize(y_lines);

        auto do_step = [&](){
            ispc::mandelbrot_ispc(re_low, im_low, re_high, im_high, SCREEN_WIDTH, SCREEN_HEIGHT, 1000, data.data());

            // if (threads.empty() && false)
            // {
            //     int max_threads = std::thread::hardware_concurrency();
            //     //std::cout << "Using " << max_threads << " threads" << std::endl;
            //     for (int i = 0; i < max_threads; i++)
            //     {
            //         int y_start = (y_lines / max_threads) * i;
            //         int y_end = (y_lines / max_threads) * (i + 1);
            //         //std::cout << "Thread " << i << " procesing y range: " << y_start << "-" << y_end << std::endl;
            //         threads.emplace_back(render_thread, y_start, y_end, std::ref(data));
            //     }
            // }
        };

        auto cleanup = [&](){
            for (auto& t : threads)
            {
                t.join();
            }
            threads.clear();
        };

        while (!quit)
        {
            while(SDL_PollEvent(&e))
            {
                switch (e.type)
                {
                case SDL_QUIT:
                    {
                        quit = true;
                        cleanup();
                    }
                    break;
                case SDL_KEYDOWN:
                    switch (e.key.keysym.sym)
                    {
                    case SDLK_PAGEUP:
                        cleanup();
                        im_low *= 0.9;
                        im_high *= 0.9;
                        re_high *= 0.9;
                        re_low *= 0.9;
                        break;

                    case SDLK_PAGEDOWN:
                        cleanup();
                        im_low *= 1.1;
                        im_high *= 1.1;
                        re_low *= 1.1;
                        re_high *= 1.1;
                        break;

                    case SDLK_UP:
                        cleanup();
                        im_low -= 0.1;
                        im_high -= 0.1;
                        break;

                    case SDLK_DOWN:
                        cleanup();
                        im_low += 0.1;
                        im_high += 0.1;
                        break;

                    case SDLK_LEFT:
                        cleanup();
                        re_low -= 0.1;
                        re_high -= 0.1;
                        break;

                    case SDLK_RIGHT:
                        cleanup();
                        re_high += 0.1;
                        re_low += 0.1;
                        break;

                    }
                    break;

                default:
                    break;
                }
            }

            do_step();

            void *pixels;
            int pitch;
            int rc = SDL_LockTexture(buffer, NULL, &pixels, &pitch);
            assert(rc == 0);
            uint8_t* ptr_out = (uint8_t*)pixels;
            uint8_t* ptr_in = (uint8_t*)data.data();
            assert(line_size == pitch);

            for (int y = 0; y < SCREEN_HEIGHT; y++)
            {
                std::lock_guard guard(line_locks[y]);
                size_t line_offset = y * line_size;
                memcpy(ptr_out + line_offset, ptr_in + line_offset, line_size);
            }

            SDL_UnlockTexture(buffer);
            SDL_RenderCopy(renderer, buffer, NULL, NULL);
            SDL_RenderPresent(renderer);
            SDL_UpdateWindowSurface(window);
        }

        SDL_DestroyWindow(window);
    }

    SDL_Quit();
    return 0;
}
