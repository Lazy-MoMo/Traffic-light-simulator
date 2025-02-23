#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

std::mutex lightMutex;

class TrafficLight {
public:
  sf::RectangleShape shape;
  sf::Color colors[2] = {sf::Color::Red, sf::Color::Green};
  int state = 0; // 0 = Red, 1 = Green

  TrafficLight(float x, float y, float width, float height) {
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(colors[state]);
  }

  void updateState() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      std::lock_guard<std::mutex> lock(lightMutex);
      state = (state + 1) % 2; // Cycle through states (Red and Green)
      shape.setFillColor(colors[state]);
      std::cout << "Light changed to: " << (state == 0 ? "Red" : "Green")
                << std::endl;
    }
  }

  bool isRed() { return state == 0; }
};

class Road {
public:
  sf::RectangleShape shape;
  sf::Text label;

  Road(float x, float y, float width, float height, const std::string &name,
       sf::Font &font) {
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(sf::Color(50, 50, 50)); // Dark gray road color

    label.setFont(font);
    label.setString(name);
    label.setCharacterSize(24);
    label.setFillColor(sf::Color::White); // Set text color to white
    label.setPosition(x + width / 2 - label.getGlobalBounds().width / 2,
                      y + height / 2 - label.getGlobalBounds().height / 2);
  }
};

class Car {
public:
  sf::RectangleShape shape;
  float speedX;
  float speedY;

  Car(float x, float y, float width, float height, float speedX, float speedY) {
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(sf::Color::Blue); // Default car color
    this->speedX = speedX;
    this->speedY = speedY;
  }

  void move() {
    shape.move(speedX, speedY); // Move the car
  }

  bool isOutOfBounds(float windowWidth, float windowHeight) {
    sf::Vector2f pos = shape.getPosition();
    return (pos.x < 0 || pos.x > windowWidth || pos.y < 0 ||
            pos.y > windowHeight);
  }
};

class Lane {
public:
  sf::RectangleShape shape;
  sf::Color color;
  TrafficLight *trafficLight;
  std::vector<Car> cars;

  Lane(float x, float y, float width, float height, sf::Color color,
       TrafficLight *trafficLight) {
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(color);
    this->trafficLight = trafficLight;
  }

  void addCar(Car car) { cars.push_back(car); }

  void updateCars() {
    for (auto it = cars.begin(); it != cars.end();) {
      bool shouldMove = true;
      sf::Vector2f carPos = it->shape.getPosition();

      // Check if the car is within the specified rectangular area
      bool inRectangularArea = (carPos.x >= 350 && carPos.x <= 450 &&
                                carPos.y >= 250 && carPos.y <= 350);

      if (trafficLight->isRed() && !inRectangularArea) {
        if (shape.getSize().x > shape.getSize().y) { // Horizontal lane
          if (it->speedX > 0 &&
              carPos.x <= trafficLight->shape.getPosition().x) {
            shouldMove = false;
          } else if (it->speedX < 0 &&
                     carPos.x >= trafficLight->shape.getPosition().x) {
            shouldMove = false;
          }
        } else { // Vertical lane
          if (it->speedY > 0 &&
              carPos.y <= trafficLight->shape.getPosition().y) {
            shouldMove = false;
          } else if (it->speedY < 0 &&
                     carPos.y >= trafficLight->shape.getPosition().y) {
            shouldMove = false;
          }
        }
      }

      if (shouldMove) {
        it->move();
      }

      if (it->isOutOfBounds(800, 600)) {
        it = cars.erase(it); // Remove car if it is out of bounds
      } else {
        ++it;
      }
    }
  }

  void drawCars(sf::RenderWindow &window) {
    for (auto &car : cars) {
      window.draw(car.shape);
    }
  }
};

int main() {
  sf::RenderWindow window(sf::VideoMode(800, 600), "Traffic Light Simulator",
                          sf::Style::Fullscreen);
  window.setFramerateLimit(60);

  sf::Font font;
  if (!font.loadFromFile(
          "/usr/share/fonts/abattis-cantarell-fonts/Cantarell-Light.otf")) {
    std::cerr << "Error loading font\n";
    return -1;
  }

  // Roads
  Road roadA(200, 250, 400, 20, "Road A", font);
  Road roadB(200, 280, 400, 20, "Road B", font);
  Road roadC(350, 100, 20, 400, "Road C", font);
  Road roadD(380, 100, 20, 400, "Road D", font);

  // Traffic lights
  TrafficLight trafficLight1(450, 250, 25, 100);
  TrafficLight trafficLight2(325, 250, 25, 100);
  TrafficLight trafficLight3(350, 225, 100, 25);
  TrafficLight trafficLight4(350, 350, 100, 25);

  // Lanes for horizontal road
  Lane lane1(200, 260, 400, 20, sf::Color::White, &trafficLight1);
  Lane lane2(200, 290, 400, 20, sf::Color::White, &trafficLight1);
  Lane lane3(200, 320, 400, 20, sf::Color::White, &trafficLight1);

  // Lanes for vertical road
  Lane lane4(360, 100, 20, 400, sf::Color::White, &trafficLight3);
  Lane lane5(390, 100, 20, 400, sf::Color::White, &trafficLight3);
  Lane lane6(420, 100, 20, 400, sf::Color::White, &trafficLight3);

  // Threads for traffic lights
  std::thread lightThread1(&TrafficLight::updateState, &trafficLight1);
  std::thread lightThread2(&TrafficLight::updateState, &trafficLight2);
  std::thread lightThread3(&TrafficLight::updateState, &trafficLight3);
  std::thread lightThread4(&TrafficLight::updateState, &trafficLight4);

  std::srand(std::time(nullptr));

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed)
        window.close();
      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Q)
        window.close();
    }

    // Spawn cars randomly
    if (std::rand() % 100 < 2) {  // 2% chance to spawn a car each frame
      int side = std::rand() % 4; // Randomly choose one of the four sides
      if (side == 0) {            // Left side
        float laneY =
            260 + (std::rand() % 3) *
                      30; // Randomly choose one of the horizontal lanes
        lane1.addCar(Car(200, laneY, 40, 20, 0.5f,
                         0.0f)); // Add a new car moving horizontally
      } else if (side == 1) {    // Right side
        float laneY =
            260 + (std::rand() % 3) *
                      30; // Randomly choose one of the horizontal lanes
        lane1.addCar(Car(600, laneY, 40, 20, -0.5f,
                         0.0f)); // Add a new car moving horizontally
      } else if (side == 2) {    // Top side
        float laneX = 360 + (std::rand() % 3) *
                                30; // Randomly choose one of the vertical lanes
        lane4.addCar(Car(laneX, 100, 20, 40, 0.0f,
                         0.5f)); // Add a new car moving vertically
      } else if (side == 3) {    // Bottom side
        float laneX = 360 + (std::rand() % 3) *
                                30; // Randomly choose one of the vertical lanes
        lane4.addCar(Car(laneX, 500, 20, 40, 0.0f,
                         -0.5f)); // Add a new car moving vertically
      }
    }

    // Update cars in lanes
    lane1.updateCars();
    lane2.updateCars();
    lane3.updateCars();
    lane4.updateCars();
    lane5.updateCars();
    lane6.updateCars();

    {
      std::lock_guard<std::mutex> lock(lightMutex);
      window.clear();
      window.draw(roadA.shape);
      window.draw(roadB.shape);
      window.draw(roadC.shape);
      window.draw(roadD.shape);
      window.draw(roadA.label);
      window.draw(roadB.label);
      window.draw(roadC.label);
      window.draw(roadD.label);
      window.draw(lane1.shape);
      window.draw(lane2.shape);
      window.draw(lane3.shape);
      window.draw(lane4.shape);
      window.draw(lane5.shape);
      window.draw(lane6.shape);
      window.draw(trafficLight1.shape);
      window.draw(trafficLight2.shape);
      window.draw(trafficLight3.shape);
      window.draw(trafficLight4.shape);

      lane1.drawCars(window);
      lane2.drawCars(window);
      lane3.drawCars(window);
      lane4.drawCars(window);
      lane5.drawCars(window);
      lane6.drawCars(window);

      window.display();
    }
  }

  lightThread1.detach();
  lightThread2.detach();
  lightThread3.detach();
  lightThread4.detach();
  return 0;
}
