#include "../include/simulator.h"
#include "../include/traffic_generator.h"
#include <SDL2/SDL.h>
#include <stdio.h>

int main() {
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;

  if (initializeSDL(&window, &renderer) == 0) {
    fprintf(stderr, "Failed to initialize SDL\n");
    return 1;
  }

  printf("Traffic light simulator starting...\n");

  if (!run_simulator(window, renderer)) {
    fprintf(stderr, "Simulation failed\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // Clean up and quit SDL
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
