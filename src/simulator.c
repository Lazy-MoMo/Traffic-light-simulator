#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 20
#define MAIN_FONT "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define SCALE 1
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define ARROW_SIZE 15
#define CAR_WIDTH 30
#define CAR_HEIGHT 50

const char *VEHICLE_FILE = "vehicles.data";

typedef struct {
  int currentLight;
  int nextLight;
} SharedData;

typedef struct {
  int x, y;
  int speed;
} Car;

// Function declarations
bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font);
void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x,
                 int y);
void drawLightForB(SDL_Renderer *renderer, bool isRed);
void refreshLight(SDL_Renderer *renderer, SharedData *sharedData);
void *chequeQueue(void *arg);
void *readAndParseFile(void *arg);
void drawCar(SDL_Renderer *renderer, Car *car);
void updateCarPosition(Car *car);

void printMessageHelper(const char *message, int count) {
  for (int i = 0; i < count; i++)
    printf("%s\n", message);
}

int main() {
  pthread_t tQueue, tReadFile;
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Event event;

  if (!initializeSDL(&window, &renderer)) {
    return -1;
  }
  SDL_mutex *mutex = SDL_CreateMutex();
  SharedData sharedData = {0, 0}; // 0 => all red

  TTF_Font *font = TTF_OpenFont(MAIN_FONT, 24);
  if (!font)
    SDL_Log("Failed to load font: %s", TTF_GetError());

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
  drawRoadsAndLane(renderer, font);
  SDL_RenderPresent(renderer);

  pthread_create(&tQueue, NULL, chequeQueue, &sharedData);
  pthread_create(&tReadFile, NULL, readAndParseFile, NULL);

  Car car = {100, 100, 5}; // Initial position and speed

  bool running = true;
  while (running) {
    refreshLight(renderer, &sharedData);
    while (SDL_PollEvent(&event))
      if (event.type == SDL_QUIT)
        running = false;

    updateCarPosition(&car);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    drawRoadsAndLane(renderer, font);
    drawCar(renderer, &car);
    SDL_RenderPresent(renderer);
  }
  SDL_DestroyMutex(mutex);
  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (window)
    SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
    return false;
  }
  if (TTF_Init() < 0) {
    SDL_Log("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
    return false;
  }

  *window = SDL_CreateWindow("Junction Diagram", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH * SCALE,
                             WINDOW_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
  if (!*window) {
    SDL_Log("Failed to create window: %s", SDL_GetError());
    SDL_Quit();
    return false;
  }

  *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
  SDL_RenderSetScale(*renderer, SCALE, SCALE);

  if (!*renderer) {
    SDL_Log("Failed to create renderer: %s", SDL_GetError());
    SDL_DestroyWindow(*window);
    TTF_Quit();
    SDL_Quit();
    return false;
  }

  return true;
}

void drawCar(SDL_Renderer *renderer, Car *car) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the car
  SDL_Rect carRect = {car->x, car->y, CAR_WIDTH, CAR_HEIGHT};
  SDL_RenderFillRect(renderer, &carRect);
}

void updateCarPosition(Car *car) {
  car->y += car->speed; // Move the car downwards
}

void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

void drawArrwow(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int x3,
                int y3) {
  if (y1 > y2) {
    swap(&y1, &y2);
    swap(&x1, &x2);
  }
  if (y1 > y3) {
    swap(&y1, &y3);
    swap(&x1, &x3);
  }
  if (y2 > y3) {
    swap(&y2, &y3);
    swap(&x2, &x3);
  }

  float dx1 = (y2 - y1) ? (float)(x2 - x1) / (y2 - y1) : 0;
  float dx2 = (y3 - y1) ? (float)(x3 - x1) / (y3 - y1) : 0;
  float dx3 = (y3 - y2) ? (float)(x3 - x2) / (y3 - y2) : 0;

  float sx1 = x1, sx2 = x1;

  for (int y = y1; y < y2; y++) {
    SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
    sx1 += dx1;
    sx2 += dx2;
  }

  sx1 = x2;

  for (int y = y2; y <= y3; y++) {
    SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
    sx1 += dx3;
    sx2 += dx2;
  }
}

void drawLightForB(SDL_Renderer *renderer, bool isRed) {
  SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
  SDL_Rect lightBox = {400, 300, 50, 30};
  SDL_RenderFillRect(renderer, &lightBox);
  if (isRed)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  else
    SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
  SDL_Rect straight_Light = {405, 305, 20, 20};
  SDL_RenderFillRect(renderer, &straight_Light);
  drawArrwow(renderer, 435, 305, 435, 305 + 20, 435 + 10, 305 + 10);
}
