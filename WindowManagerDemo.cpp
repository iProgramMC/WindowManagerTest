// Define things so that we don't get compiler errors later on.
#define OLC_PGE_APPLICATION
#define _CRT_SECURE_NO_WARNINGS
#include "olcPixelGameEngine.h"

// Helper defines and macros
#define r currentWindow
#define window_loop() for (int i = 0; i < WINDOW_COUNT; i++) if ((r = &windows[i])->exists)

// Theming is customized here.
#define WindowBarHeight 12
#define WindowBarColorActive 0xFFAA0000
#define WindowBarColorInactive 0xFF8D8985
#define WindowBackgroundColor 0xFFCAC6C2
#define GameBackgroundColor 0xFF838300

// Number of points for chart.
#define NUM_CHART_POINTS 20

// Chart data struct.
struct chart_data {
	float points[NUM_CHART_POINTS];
};

typedef void* window_data; // can be anything

struct window {
	// Is this window in a state where it exists?
	bool exists = false;

	// Position in x/y.
	int x = 0, y = 0;

	// Width and height in pixels.
	int width = 0, height = 0;

	// Window title. Must be 200 characters or less.
	char title[200] = { 0, };

	// User defined data. Must be malloc'd when spawning window
	// and freed when killing it.
	window_data wd = NULL;
};

// Maximum window count.
const int WINDOW_COUNT = 1024;

// Rectangle bounds and point checking code.
struct rectangle {
	int x, y, w, h;
};
// This code was recycled from a game I wrote, thus the naming of the variables.
bool RectIntersect(rectangle* r1, rectangle* r2)
{
	int playerX1 = r1->x, playerX2 = r1->x + r1->w, playerY1 = r1->y, playerY2 = r1->y + r1->w;
	int tileRectX1 = r2->x, tileRectX2 = r2->x + r2->w, tileRectY1 = r2->y, tileRectY2 = r2->y + r2->w;
	bool noOverlap = tileRectX1 > playerX2 ||
		playerX1 > tileRectX2 ||
		tileRectY1 > playerY2 ||
		playerY1 > tileRectY2;
	return !noOverlap;
}
bool RectContains(rectangle* rect, unsigned x, unsigned y) {
	int x2 = rect->x + rect->w;
	int y2 = rect->y + rect->h;
	return
		!(x < (unsigned)rect->x ||
			y < (unsigned)rect->y ||
			x >= (unsigned)x2 ||
			y >= (unsigned)y2);
}

class WndMgr : public olc::PixelGameEngine
{
public:
	int selected_window = -1;
	window windows[WINDOW_COUNT];
	window* currentWindow = NULL;
public:
	WndMgr()
	{
		sAppName = "Window Manager Demo";
	}

	void DrawWindow(window* wnd, int i) {
		// Draw the window border.
		auto pixel = olc::Pixel(WindowBackgroundColor);
		FillRect({ wnd->x, wnd->y }, { wnd->width, wnd->height }, pixel);
		DrawLine(wnd->x, wnd->y, wnd->x + wnd->width, wnd->y, olc::WHITE);
		DrawLine(wnd->x + wnd->width, wnd->y, wnd->x + wnd->width, wnd->y + wnd->height, olc::Pixel(0xFF8D8985));
		DrawLine(wnd->x + wnd->width, wnd->y + wnd->height, wnd->x, wnd->y + wnd->height, olc::Pixel(0xFF8D8985));
		DrawLine(wnd->x, wnd->y + wnd->height, wnd->x, wnd->y, olc::WHITE);
		FillRect(wnd->x + 2, wnd->y + 2, wnd->width - 3, WindowBarHeight, olc::Pixel(selected_window == i ? WindowBarColorActive : WindowBarColorInactive));
		DrawString(olc::vi2d( (int)wnd->x + 4, (int)wnd->y + 4 ), std::string(wnd->title));

		// Draw chart window contents.
		float size_of = 1.f / (NUM_CHART_POINTS-1);
		int width = (int)((wnd->width - 8) * size_of);
		for (int i = 0; i < NUM_CHART_POINTS - 1; i++) {
			float p0 = ((chart_data*)wnd->wd)->points[i], p1 = ((chart_data*)wnd->wd)->points[1 + i];
			int yb = wnd->height - WindowBarHeight - 10;
			int y0 = (wnd->height - 4) - (yb * p0);
			int y1 = (wnd->height - 4) - (yb * p1);
			DrawLine(olc::vi2d(wnd->x + 4 + width * i, wnd->y + y0), olc::vi2d(wnd->x + 4 + width * (i + 1), wnd->y + y1), olc::BLUE);
		}
	}

	void RedrawAllEvent() {
		window_loop() {
			// simple window draw
			DrawWindow(r, i);
		}
	}

	// Returns the window's ID or -1 if unavailable.
	int SpawnWindow(int x, int y, int width, int height, int color) {
		// find free window slot
		for (int i = 0; i < WINDOW_COUNT; i++) {
			if (!windows[i].exists) {
				r = &windows[i];
				r->exists = true;
				r->width = width;
				r->height = height;
				r->x = x;
				r->y = y;
				strcpy(r->title, "Chart Window");
				r->wd = new chart_data();
				for (int i = 0; i < NUM_CHART_POINTS; i++) {
					((chart_data*)r->wd)->points[i] = (rand() / (double)RAND_MAX); 
				}
				//r->color = color;
				DrawWindow(r, i);
				return i;
			}
		}
		return -1;
	}

public:
	bool OnUserCreate() override
	{
		srand(time(NULL)); // initialize RNG based on time

		// Called once at the start, so create things here
		FillRect({ 0,0 }, { ScreenWidth(),ScreenHeight() }, olc::Pixel(GameBackgroundColor));
		// add three test windows
		SpawnWindow(10, 10, 120, 80, 0xff00ffff);
		SpawnWindow(90, 20, 120, 80, 0xffff00ff);
		SpawnWindow(45, 50, 120, 80, 0xffffff00);
		RedrawAllEvent();
		return true;
	}
	int a = 0;
	int lastMousePosX = 0, lastMousePosY = 0;
	bool OnUserUpdate(float fElapsedTime) override
	{
		if (GetMouse(0).bPressed) {
			// find which window was clicked
			// This should be put inside an interrupt instead

			int window_max_i = -1, last_window = -1;
			window_loop() {
				rectangle re = { r->x, r->y, r->width, r->height };
				if (RectContains(&re, GetMouseX(), GetMouseY())) {
					window_max_i = std::max(window_max_i, i);
				}
				if (r->exists) {
					last_window = i;
				}
			}
			// After finding the window, get the last one, and swap this and the last window.
			if (window_max_i != -1) {
				selected_window = last_window;
				
				window aux;
				aux = windows[window_max_i];
				windows[window_max_i] = windows[last_window];
				windows[last_window] = aux;
				//std::swap(windows[window_max_i], windows[last_window]);
				// Then, rerender the windows. This could be done easier, but who cares?
				RedrawAllEvent();
			}
			else {
				// Rerender the active window.
				if (selected_window != -1) DrawWindow(&windows[selected_window], selected_window);
				selected_window = -1;
			}
		}
		if (GetMouse(0).bHeld) {
			if (selected_window != -1) {
				int deltaX = GetMouseX() - lastMousePosX;
				int deltaY = GetMouseY() - lastMousePosY;
				// draw a background color square here
				FillRect({ windows[selected_window].x,windows[selected_window].y }, { windows[selected_window].width+1, windows[selected_window].height + 1 }, olc::Pixel(GameBackgroundColor));
				windows[selected_window].x += deltaX;
				windows[selected_window].y += deltaY;
				RedrawAllEvent(); // could be done much more efficiently but doesnt matter
			}
		}
		lastMousePosX = GetMouseX();
		lastMousePosY = GetMouseY();
		return true;
	}
};

// Main function.
int main()
{
	WndMgr demo;
	if (demo.Construct(480, 360, 2, 2))
		demo.Start();
	return 0;
}
