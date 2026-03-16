/*
 * The idea of this program is: to compare the efficiency of different code architectures (OOP and ESC).
 * Problem: the program draws 100, 1k, 10k, 100k, 1m of objects and simulates how the ball (circle) falls and then bounces back.
 */

#include <SDL2/SDL.h>
#include <SDL.h>
#include <iostream>

using namespace std;

const static uint32_t WINDOW_HEIGHT = 800, WINDOW_WIDTH = 1000;
const uint32_t FRAMES_COUNT = 1000;

const static size_t objects_amount = 100000;


// custom elipse/circle draw function 
SDL_Point *compute_circle(int32_t centreX, int32_t centreY, int32_t radius, size_t points_size) {
	SDL_Point points[points_size];

    int32_t x = (radius - 1);
    int32_t y = 0;
    int32_t tx = 1;
    int32_t ty = 1;
    int32_t error = (tx - (radius << 1));

	int counter = 0;
    while (x >= y) {
        // Each of the following renders an octant of the circle
		
		points[counter] = {centreX + x, centreY - y};	
		points[counter + 1] = {centreX + x, centreY + y};	

		points[counter + 2] = {centreX - x, centreY - y};	
		points[counter + 3] = {centreX - x, centreY + y};	

		points[counter + 4] = {centreX + y, centreY - x};	
		points[counter + 5] = {centreX + y, centreY + x};	

		points[counter + 6] = {centreX - y, centreY - x};	
		points[counter + 7] = {centreX - y, centreY + x};	

        if (error <= 0) {
            ++y;
            error += ty;
            ty += 2;
        }

        if (error > 0) {
            --x;
            tx += 2;
            error += (tx - (radius << 1));
        }
		counter += 8;
    }
	return points;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/*
 * OOP implementation
 */


class Speed {
public:
	float x_speed, y_speed;
	
	Speed() : x_speed(0), y_speed(0) {}

	Speed(float x_speed, float y_speed) : x_speed(x_speed), y_speed(y_speed) {}
};

class Shape {
public:
    float x, y;
    tuple<int, int, int> color;
	Speed speed;

	Shape() : x(0), y(0), color(0, 0, 0), speed() {}

	Shape(float x, float y) : x(x), y(y), color(0, 0, 0), speed() {}
	Shape(float x, float y, const Speed& speed) : x(x), y(y), color(0, 0, 0), speed(speed) {}

    virtual void draw(SDL_Renderer* renderer) = 0;

    void change_color(int color_r, int color_g, int color_b)
    {
        this->color = { color_r, color_g, color_b };
    }
};

class Point : public Shape {
public:
	Point() : Shape() {}
	Point(float x, float y) : Shape(x, y) {}
	
	void draw(SDL_Renderer* renderer) override {
		return;
	}
};

class Elipse : public Shape {
public:
	uint32_t w, h;

	Elipse() : Shape(), w(0), h(0) {}
	Elipse(float x, float y, uint32_t w, uint32_t h) : Shape(x, y), w(w), h(h) {}
	Elipse(float x, float y, uint32_t w, uint32_t h, Speed speed) : Shape(x, y, speed), w(w), h(h) {}

    void draw(SDL_Renderer* renderer) override
    {
		return;
    }
};

void DrawCircleOOP(SDL_Renderer* renderer, Elipse elipse)
{
	const uint32_t points_size = (elipse.h * 8 * 35 / 49 + (8 - 1)) & -8;
	//vector<Point> points = compute_circle_oop(elipse.x, elipse.y, elipse.h, points_size);
	SDL_Point *points = compute_circle(elipse.x, elipse.y, elipse.h, points_size);
	SDL_RenderDrawPoints(renderer, points, points_size);
}


bool draw_objects(SDL_Window* sdl_window, SDL_Renderer* renderer, size_t capacity, vector<Elipse>& elipses)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return false;
        }
    }

	// redrawing the whole screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

	const auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < elipses.size(); ++i) {
		Elipse elipse = elipses[i];
        float_t cur_y_speed = elipse.speed.y_speed * 0.99;
        float_t cur_x_speed = elipse.speed.x_speed * 0.99;

        if ((cur_y_speed > 0 && cur_y_speed + elipse.y + elipse.h / 2 >= WINDOW_HEIGHT)
            || (cur_y_speed < 0 && elipse.y - cur_y_speed - elipse.h / 2 <= 0)) {
            cur_y_speed = -cur_y_speed;
            elipse.speed.y_speed = cur_y_speed;
        }

        if ((cur_x_speed > 0 && cur_x_speed + elipse.x + elipse.w / 2 >= WINDOW_WIDTH) 
			|| (cur_x_speed < 0 && elipse.x - cur_x_speed - elipse.w / 2 <= 0)) {
            cur_x_speed = -cur_x_speed;
            elipse.speed.x_speed = cur_x_speed;
        }

        elipse.y += cur_y_speed;
        elipse.x += cur_x_speed;

		elipses[i] = elipse;
		DrawCircleOOP(renderer, elipses[i]);
    }
	//const auto end = std::chrono::high_resolution_clock::now();
	//auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	//cout << "The generation of each frame lasted = " << duration.count() << " microseconds" << endl;

    SDL_RenderPresent(renderer);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// main function
//
//

int main(int argc, char* argv[])
{
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not be initialized!\nSDL_Error:", SDL_GetError());
        return 1;
    }

    static SDL_Window* sdl_window = SDL_CreateWindow("Test Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (sdl_window == NULL) {
        fprintf(stderr, "SDL Window could not be created!\nSDL_Error:", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL Renderer could not be created!\nSDL_Error:", SDL_GetError());
        SDL_Quit();
        return 1;
    }

	vector<Elipse> elipses(objects_amount);

    int y = 10;
    for (int i = 0; i < objects_amount; ++i) {
        Speed sp = { (float) (i + 1), (float) 9.81 / 60 * 5 * (i + 1)};
		elipses[i] = Elipse(100, i * 50 + y, 10, 10, sp);
        y += 10;
        //add(sset, sp);
    }

	const auto start = std::chrono::high_resolution_clock::now();
	int i = 0;
    while (draw_objects(sdl_window, renderer, 10, elipses) && i < FRAMES_COUNT) {
        //SDL_Delay(16);
		i++;
    }
	const auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	cout << "The generation of " << FRAMES_COUNT << " frames lasted = " << duration.count() / 10e5 << " seconds" << endl;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 0;
}
