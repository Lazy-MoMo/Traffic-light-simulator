#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <vector>

std::mutex lightMutex;

enum class Side { NONE, LEFT, RIGHT, TOP, BOTTOM };

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

  bool isRed() { return state == 0; }
};

class Road {
public:
  sf::RectangleShape shape;

  Road(float x, float y, float width, float height) {
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(sf::Color(50, 50, 50));
  }
};

class Car {
public:
  sf::RectangleShape shape;
  float speedX, speedY;
  bool isStraight;
  bool isRight;
  bool hasTurned;
  bool stopped;

  Car(float x, float y, float width, float height, float speedX, float speedY,
      bool isStraight, bool isRight, bool hasTurned = false)
      : speedX(speedX), speedY(speedY), isStraight(isStraight),
        isRight(isRight), hasTurned(hasTurned), stopped(false) {
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(sf::Color::Blue);
  }

  void move() { shape.move(speedX, speedY); }

  bool isOutOfBounds(float windowWidth, float windowHeight) {
    sf::Vector2f pos = shape.getPosition();
    return (pos.x < 0 || pos.x > windowWidth || pos.y < 0 ||
            pos.y > windowHeight);
  }

  sf::FloatRect getCollisionBounds() const {
    // Get the car's bounding box
    return shape.getGlobalBounds();
  }

  bool isColliding(const Car &other) const {
    return getCollisionBounds().intersects(other.getCollisionBounds());
  }
};

class Lane {
public:
  sf::RectangleShape shape;
  sf::Color color;
  sf::Color carColor;
  TrafficLight *trafficLight;
  std::vector<Car> cars;
  bool ignoreTrafficLight;
  bool isPriority;
  int waitingVehicles;

  Lane(float x, float y, float width, float height, sf::Color color,
       sf::Color carColor, TrafficLight *trafficLight,
       bool ignoreTrafficLight = false, bool isPriority = false)
      : ignoreTrafficLight(ignoreTrafficLight), isPriority(isPriority),
        waitingVehicles(0) {
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(x, y);
    shape.setFillColor(color);
    this->carColor = carColor;
    this->trafficLight = trafficLight;
  }

  void addCar(const Car &car) {
    // Check if there is enough space to add the car without collision
    Car newCar = car;
    newCar.shape.setFillColor(carColor);

    // Check for collisions with existing cars
    for (const auto &existingCar : cars) {
      if (newCar.isColliding(existingCar)) {
        // Don't add car if it would collide
        return;
      }
    }

    cars.push_back(newCar);
  }

  void updateCars() {
    const float stopThreshold = 10.0f;  // Distance threshold before stopping
    const float collisionBuffer = 8.0f; // Minimum distance between cars

    for (auto it = cars.begin(); it != cars.end();) {
      bool shouldMove = true;
      sf::Vector2f carPos = it->shape.getPosition();
      bool inRectangularArea = (carPos.x >= 350 && carPos.x <= 450 &&
                                carPos.y >= 250 && carPos.y <= 350);

      // Check collision with other cars in the same lane
      for (auto &otherCar : cars) {
        // Skip self comparison
        if (&otherCar == &(*it)) {
          continue;
        }

        // Calculate future position
        sf::Vector2f futurePos = carPos;
        futurePos.x += it->speedX;
        futurePos.y += it->speedY;

        // Temporary car at future position to check collision
        Car futureCar = *it;
        futureCar.shape.setPosition(futurePos);

        // Check if future position would cause collision
        if (futureCar.isColliding(otherCar)) {
          shouldMove = false;
          it->stopped = true;
          break;
        }

        // Check for safe distance between cars (for cars going in the same
        // direction)
        if ((it->speedX * otherCar.speedX > 0 ||
             it->speedY * otherCar.speedY > 0)) {
          sf::Vector2f otherPos = otherCar.shape.getPosition();
          float distance = 0.0f;

          // Calculate distance in the direction of movement
          if (std::fabs(it->speedX) > 0) { // Horizontal movement
            if ((it->speedX > 0 && otherPos.x > carPos.x) ||
                (it->speedX < 0 && otherPos.x < carPos.x)) {
              distance =
                  std::fabs(otherPos.x - carPos.x) - it->shape.getSize().x;
            }
          } else if (std::fabs(it->speedY) > 0) { // Vertical movement
            if ((it->speedY > 0 && otherPos.y > carPos.y) ||
                (it->speedY < 0 && otherPos.y < carPos.y)) {
              distance =
                  std::fabs(otherPos.y - carPos.y) - it->shape.getSize().y;
            }
          }

          // If cars are too close in the direction of movement, stop
          if (distance > 0 && distance < collisionBuffer) {
            shouldMove = false;
            it->stopped = true;
            break;
          }
        }
      }

      // Traffic light check (only if collision check passed)
      if (shouldMove && !ignoreTrafficLight && trafficLight->isRed() &&
          !inRectangularArea) {
        if (shape.getSize().x > shape.getSize().y) { // Horizontal lane
          if (it->speedX > 0) {                      // Car moving right
            float carRight = carPos.x + it->shape.getSize().x;
            float lightLeft = trafficLight->shape.getPosition().x;
            float gap = lightLeft - carRight;
            if (gap > 0 && gap < stopThreshold)
              shouldMove = false;
          } else if (it->speedX < 0) { // Car moving left
            float carLeft = carPos.x;
            float lightRight = trafficLight->shape.getPosition().x +
                               trafficLight->shape.getSize().x;
            float gap = carLeft - lightRight;
            if (gap > 0 && gap < stopThreshold)
              shouldMove = false;
          }
        } else {                // Vertical lane
          if (it->speedY > 0) { // Car moving down
            float carBottom = carPos.y + it->shape.getSize().y;
            float lightTop = trafficLight->shape.getPosition().y;
            float gap = lightTop - carBottom;
            if (gap > 0 && gap < stopThreshold)
              shouldMove = false;
          } else if (it->speedY < 0) { // Car moving up
            float carTop = carPos.y;
            float lightBottom = trafficLight->shape.getPosition().y +
                                trafficLight->shape.getSize().y;
            float gap = carTop - lightBottom;
            if (gap > 0 && gap < stopThreshold)
              shouldMove = false;
          }
        }
      }

      if (shouldMove) {
        it->move();
        it->stopped = false;
      } else {
        it->stopped = true;
      }

      if (it->isOutOfBounds(720, 600)) {
        it = cars.erase(it);
      } else if (carPos.x >= 360 && carPos.x <= 380 && carPos.y >= 260 &&
                 carPos.y <= 280) {
        it->speedX = 0.0f;
        it->speedY = -0.5f;
        ++it; // Advance the iterator
      } else if (carPos.x >= 420 && carPos.x <= 440 && carPos.y >= 260 &&
                 carPos.y <= 280) {
        it->speedY = 0.0f;
        it->speedX = 0.5f;
        ++it; // Advance the iterator
      } else if (carPos.x == 441 && carPos.y >= 320 && carPos.y <= 340) {
        it->speedX = 0.0f;
        it->speedY = 0.5f;
        ++it; // Advance the iterator
      } else if (carPos.y == 341 && carPos.x >= 360 && carPos.x <= 380) {
        it->speedY = 0.0f;
        it->speedX = -0.5f;
        ++it; // Advance the iterator
      } else if (it->isRight && !it->hasTurned && carPos.x >= 410 &&
                 carPos.x <= 430 && carPos.y >= 310 && carPos.y <= 330 &&
                 it->speedY > 0) {
        it->speedY = 0.0f;
        it->speedX = -0.5f;
        it->hasTurned = true;
        ++it; // Advance the iterator
      } else if (it->isRight && !it->hasTurned && carPos.x >= 390 &&
                 carPos.x <= 410 && carPos.y == 291 && it->speedY < 0) {
        it->speedY = 0.0f;
        it->speedX = 0.5f;
        it->hasTurned = true;
        ++it; // Advance the iterator
      } else if (it->isRight && !it->hasTurned && carPos.x == 410 &&
                 carPos.y >= 290 && carPos.y <= 310 && it->speedX > 0) {
        it->speedX = 0.0f;
        it->speedY = 0.5f;
        it->hasTurned = true;
        ++it; // Advance the iterator
      } else if (it->isRight && !it->hasTurned && carPos.x == 391 &&
                 carPos.y >= 310 && carPos.y <= 330 && it->speedX < 0) {
        it->speedX = 0.0f;
        it->speedY = -0.5f;
        it->hasTurned = true;
        ++it; // Advance the iterator
      } else {
        ++it;
      }
    }
  }

  void updateWaitingCount() {
    waitingVehicles = 0;
    for (auto &car : cars) {
      if (car.stopped) {
        waitingVehicles++;
      }
    }
  }

  void drawCars(sf::RenderWindow &window) {
    for (auto &car : cars) {
      window.draw(car.shape);
    }
  }
};

Lane *findLaneWithMostCars(const std::vector<Lane *> &lanes) {
  Lane *maxLane = nullptr;
  int maxCount = 0;
  for (auto lane : lanes) {
    int count = lane->cars.size();
    if (count > maxCount) {
      maxCount = count;
      maxLane = lane;
    }
  }
  return maxLane;
}

// Count how many cars in a group of lanes are stopped
int countStopped(const std::vector<Lane *> &lanes) {
  int count = 0;
  for (auto lane : lanes) {
    for (auto &car : lane->cars) {
      if (car.stopped)
        count++;
    }
  }
  return count;
}

// Calculate total waiting vehicles for a group of lanes
int calculateTotalWaiting(const std::vector<Lane *> &lanes) {
  int total = 0;
  for (auto lane : lanes) {
    if (!lane->isPriority) { // Only count normal lanes
      total += lane->waitingVehicles;
    }
  }
  return total;
}

// Calculate green light duration based on waiting vehicles
float calculateGreenDuration(const std::vector<Lane *> &allLanes,
                             const std::vector<Lane *> &activeLanes) {
  int totalNormalLanes = 0;
  int totalWaitingVehicles = 0;

  // Count normal lanes and their waiting vehicles
  for (auto lane : allLanes) {
    if (!lane->isPriority) {
      totalNormalLanes++;
      totalWaitingVehicles += lane->waitingVehicles;
    }
  }

  if (totalNormalLanes == 0)
    return 5.0f; // Default duration if no normal lanes

  // Calculate |V| according to the formula: |V| = 1/n * sum(Li)
  float vehiclesToServe = (float)totalWaitingVehicles / totalNormalLanes;

  // Calculate time: Total time = |V| * t
  // Assuming t = 1.0 second per vehicle
  const float timePerVehicle = 1.0f;
  return std::max(3.0f, vehiclesToServe * timePerVehicle); // Minimum 3 seconds
}

// Check if any car intersects a given region
bool anyCarInRegion(const std::vector<Lane *> &lanes,
                    const sf::FloatRect &region) {
  for (auto lane : lanes) {
    for (auto &car : lane->cars) {
      if (car.shape.getGlobalBounds().intersects(region))
        return true;
    }
  }
  return false;
}

int main() {
  sf::RenderWindow window(sf::VideoMode(800, 600), "Traffic Light Simulator");
  window.setFramerateLimit(300);

  sf::Font font;
  if (!font.loadFromFile(
          "/usr/share/fonts/abattis-cantarell-fonts/Cantarell-Light.otf")) {
    std::cerr << "Error loading font\n";
    return -1;
  }

  // Create roads
  Road roadA(100, 250, 400, 120);
  Road roadB(470, 250, 250, 120);
  Road roadC(350, 000, 120, 400);
  Road roadD(350, 370, 120, 400);

  // Create traffic lights (we control their states via priority check)
  TrafficLight trafficLight1(470, 250, 25, 120); // Right side
  TrafficLight trafficLight2(325, 250, 25, 120); // Left side
  TrafficLight trafficLight3(350, 225, 120, 25); // Top side
  TrafficLight trafficLight4(350, 370, 120, 25); // Bottom side

  // Create lanes
  // Horizontal lanes: left side (trafficLight2) and right side (trafficLight1)
  Lane lane1(100, 260, 250, 20, sf::Color::White, sf::Color(125, 5, 82),
             &trafficLight2, true, true); // left priority lane
  Lane lane2(100, 290, 250, 40, sf::Color::White, sf::Color(125, 5, 82),
             &trafficLight2);
  Lane lane3(100, 340, 250, 20, sf::Color::White, sf::Color(125, 5, 82),
             &trafficLight2);
  Lane lane7(470, 260, 250, 20, sf::Color::White, sf::Color(210, 145, 188),
             &trafficLight1); // right
  Lane lane8(470, 290, 250, 40, sf::Color::White, sf::Color(210, 145, 188),
             &trafficLight1);
  Lane lane9(470, 340, 250, 20, sf::Color::White, sf::Color(210, 145, 188),
             &trafficLight1, true);
  Lane lane4(360, 000, 20, 250, sf::Color::White, sf::Color::Blue,
             &trafficLight3); // top
  Lane lane5(390, 000, 40, 250, sf::Color::White, sf::Color::Blue,
             &trafficLight3);
  Lane lane6(440, 000, 20, 250, sf::Color::White, sf::Color::Blue,
             &trafficLight3, true);
  Lane lane10(440, 370, 20, 250, sf::Color::White, sf::Color::Black,
              &trafficLight4, true, true); // bottom priority lane
  Lane lane11(390, 370, 40, 250, sf::Color::White, sf::Color::Black,
              &trafficLight4);
  Lane lane12(360, 370, 20, 250, sf::Color::White, sf::Color::Black,
              &trafficLight4, true);

  // Group lanes by side for priority checking.
  std::vector<Lane *> leftLanes = {&lane1, &lane2, &lane3};
  std::vector<Lane *> rightLanes = {&lane7, &lane8, &lane9};
  std::vector<Lane *> topLanes = {&lane4, &lane5, &lane6};
  std::vector<Lane *> bottomLanes = {&lane10, &lane11, &lane12};

  // Create a vector of all lanes to simplify operations
  std::vector<Lane *> allLanes = {&lane1, &lane2,  &lane3,  &lane4,
                                  &lane5, &lane6,  &lane7,  &lane8,
                                  &lane9, &lane10, &lane11, &lane12};

  Side currentPriority = Side::NONE;
  Lane *currentLane = nullptr;

  // Timing variables
  float greenTimer = 0.0f;
  float greenDuration = 0.0f;
  const float frameTime = 1.0f / 300.0f; // Based on framerate limit

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

    // Spawn cars randomly (2% chance per frame)
    if (std::rand() % 100 < 2) {
      int side = std::rand() % 4;
      if (side == 0) { // Left side
        float laneY = 260 + (std::rand() % 3) * 30;
        if (laneY == 260)
          lane1.addCar(Car(100, laneY, 20, 20, 0.5f, 0.0f, false, false));
        else if (laneY == 290) {
          int whichLane = std::rand() % 2;
          if (whichLane == 0)
            lane2.addCar(Car(100, laneY, 20, 20, 0.5f, 0.0f, true, false));
          else
            lane2.addCar(Car(100, laneY, 20, 20, 0.5f, 0.0f, false, true));
        }
      } else if (side == 1) { // Right side
        float laneY = 280 + (std::rand() % 3) * 30;
        if (laneY == 340)
          lane9.addCar(Car(700, laneY, 20, 20, -0.5f, 0.0f, false, false));
        else if (laneY == 310) {
          int whichLane = std::rand() % 2;
          if (whichLane == 0)
            lane8.addCar(Car(700, laneY, 20, 20, -0.5f, 0.0f, true, false));
          else
            lane8.addCar(Car(700, laneY, 20, 20, -0.5f, 0.0f, false, true));
        }
      } else if (side == 2) { // Top side
        float laneX = 380 + (std::rand() % 3) * 30;
        if (laneX == 410) {
          int whichLane = std::rand() % 2;
          if (whichLane == 0)
            lane5.addCar(Car(laneX, 000, 20, 20, 0.0f, 0.5f, true, false));
          else
            lane5.addCar(Car(laneX, 000, 20, 20, 0.0f, 0.5f, false, true));
        } else if (laneX == 440) {
          lane6.addCar(Car(laneX, 000, 20, 20, 0.0f, 0.5f, false, false));
        }
      } else if (side == 3) { // Bottom side
        float laneX = 360 + (std::rand() % 3) * 30;
        if (laneX == 390) {
          int whichLane = std::rand() % 2;
          if (whichLane == 0)
            lane11.addCar(Car(laneX, 600, 20, 20, 0.0f, -0.5f, true, false));
          else
            lane11.addCar(Car(laneX, 600, 20, 20, 0.0f, -0.5f, false, true));
        } else if (laneX == 360) {
          lane12.addCar(Car(laneX, 600, 20, 20, 0.0f, -0.5f, false, false));
        }
      }
    }

    // Update waiting counts for all lanes
    for (auto lane : allLanes) {
      lane->updateWaitingCount();
    }

    // Update cars in all lanes
    for (auto lane : allLanes) {
      lane->updateCars();
    }

    // Timer logic for green light duration
    if (currentPriority != Side::NONE) {
      greenTimer += frameTime;
      if (greenTimer >= greenDuration) {
        currentPriority = Side::NONE;
        greenTimer = 0.0f;
      }
    }

    // === PRIORITY CHECK ===
    // Define the base intersection region where cars are considered waiting.
    sf::FloatRect intersectionRegion(350, 250, 100, 100);

    // Check for priority lanes with more than 5 waiting vehicles
    Lane *priorityLane = nullptr;
    for (auto lane : allLanes) {
      if (lane->isPriority && lane->waitingVehicles > 5) {
        priorityLane = lane;
        break;
      }
    }

    // Determine active traffic light and lanes based on current priority.
    TrafficLight *activeTrafficLight = nullptr;
    std::vector<Lane *> *activeLanes = nullptr;
    if (currentPriority == Side::LEFT) {
      activeTrafficLight = &trafficLight2;
      activeLanes = &leftLanes;
    } else if (currentPriority == Side::RIGHT) {
      activeTrafficLight = &trafficLight1;
      activeLanes = &rightLanes;
    } else if (currentPriority == Side::TOP) {
      activeTrafficLight = &trafficLight3;
      activeLanes = &topLanes;
    } else if (currentPriority == Side::BOTTOM) {
      activeTrafficLight = &trafficLight4;
      activeLanes = &bottomLanes;
    }

    // Compute the extended region as the union of the base intersection
    // region and the active traffic light's bounds.
    sf::FloatRect extendedRegion = intersectionRegion;
    if (activeTrafficLight != nullptr) {
      sf::FloatRect lightBounds = activeTrafficLight->shape.getGlobalBounds();
      extendedRegion.left = std::min(intersectionRegion.left, lightBounds.left);
      extendedRegion.top = std::min(intersectionRegion.top, lightBounds.top);
      extendedRegion.width =
          std::max(intersectionRegion.left + intersectionRegion.width,
                   lightBounds.left + lightBounds.width) -
          extendedRegion.left;
      extendedRegion.height =
          std::max(intersectionRegion.top + intersectionRegion.height,
                   lightBounds.top + lightBounds.height) -
          extendedRegion.top;
    }

    // If a priority lane needs service and no current priority, give it
    // priority
    if (currentPriority == Side::NONE && priorityLane != nullptr) {
      if (priorityLane == &lane1 || priorityLane == &lane2 ||
          priorityLane == &lane3) {
        currentPriority = Side::LEFT;
        greenDuration = calculateGreenDuration(allLanes, leftLanes);
      } else if (priorityLane == &lane7 || priorityLane == &lane8 ||
                 priorityLane == &lane9) {
        currentPriority = Side::RIGHT;
        greenDuration = calculateGreenDuration(allLanes, rightLanes);
      } else if (priorityLane == &lane4 || priorityLane == &lane5 ||
                 priorityLane == &lane6) {
        currentPriority = Side::TOP;
        greenDuration = calculateGreenDuration(allLanes, topLanes);
      } else if (priorityLane == &lane10 || priorityLane == &lane11 ||
                 priorityLane == &lane12) {
        currentPriority = Side::BOTTOM;
        greenDuration = calculateGreenDuration(allLanes, bottomLanes);
      }
      greenTimer = 0.0f;
    }

    // If no priority is set, find the lane group with the highest total waiting
    // vehicles
    if (currentPriority == Side::NONE) {
      int leftTotal = calculateTotalWaiting(leftLanes);
      int rightTotal = calculateTotalWaiting(rightLanes);
      int topTotal = calculateTotalWaiting(topLanes);
      int bottomTotal = calculateTotalWaiting(bottomLanes);

      // Find the direction with the highest waiting vehicles
      int maxTotal = std::max({leftTotal, rightTotal, topTotal, bottomTotal});

      if (maxTotal > 0) {
        if (maxTotal == leftTotal) {
          currentPriority = Side::LEFT;
          greenDuration = calculateGreenDuration(allLanes, leftLanes);
        } else if (maxTotal == rightTotal) {
          currentPriority = Side::RIGHT;
          greenDuration = calculateGreenDuration(allLanes, rightLanes);
        } else if (maxTotal == topTotal) {
          currentPriority = Side::TOP;
          greenDuration = calculateGreenDuration(allLanes, topLanes);
        } else {
          currentPriority = Side::BOTTOM;
          greenDuration = calculateGreenDuration(allLanes, bottomLanes);
        }
        greenTimer = 0.0f;
      }
    }

    // If still no priority is set, use the original method to find lane with
    // most cars
    if (currentPriority == Side::NONE) {
      if (currentLane == nullptr || currentLane->cars.size() == 0) {
        currentLane = findLaneWithMostCars(allLanes);
      }

      if (currentLane != nullptr) {
        if (std::find(leftLanes.begin(), leftLanes.end(), currentLane) !=
            leftLanes.end()) {
          currentPriority = Side::LEFT;
          greenDuration = 5.0f; // Default duration
        } else if (std::find(rightLanes.begin(), rightLanes.end(),
                             currentLane) != rightLanes.end()) {
          currentPriority = Side::RIGHT;
          greenDuration = 5.0f; // Default duration
        } else if (std::find(topLanes.begin(), topLanes.end(), currentLane) !=
                   topLanes.end()) {
          currentPriority = Side::TOP;
          greenDuration = 5.0f; // Default duration
        } else if (std::find(bottomLanes.begin(), bottomLanes.end(),
                             currentLane) != bottomLanes.end()) {
          currentPriority = Side::BOTTOM;
          greenDuration = 5.0f; // Default duration
        }
        greenTimer = 0.0f;
      }
    }

    // Force only one light green at a time according to current priority.
    {
      std::lock_guard<std::mutex> lock(lightMutex);
      if (currentPriority == Side::LEFT) {
        trafficLight2.state = 1;
        trafficLight1.state = 0;
        trafficLight3.state = 0;
        trafficLight4.state = 0;
      } else if (currentPriority == Side::RIGHT) {
        trafficLight1.state = 1;
        trafficLight2.state = 0;
        trafficLight3.state = 0;
        trafficLight4.state = 0;
      } else if (currentPriority == Side::TOP) {
        trafficLight3.state = 1;
        trafficLight1.state = 0;
        trafficLight2.state = 0;
        trafficLight4.state = 0;
      } else if (currentPriority == Side::BOTTOM) {
        trafficLight4.state = 1;
        trafficLight1.state = 0;
        trafficLight2.state = 0;
        trafficLight3.state = 0;
      } else {
        // Default: all red.
        trafficLight1.state = 0;
        trafficLight2.state = 0;
        trafficLight3.state = 0;
        trafficLight4.state = 0;
      }
      // Update the fill colors.
      trafficLight1.shape.setFillColor(
          trafficLight1.colors[trafficLight1.state]);
      trafficLight2.shape.setFillColor(
          trafficLight2.colors[trafficLight2.state]);
      trafficLight3.shape.setFillColor(
          trafficLight3.colors[trafficLight3.state]);
      trafficLight4.shape.setFillColor(
          trafficLight4.colors[trafficLight4.state]);
    }

    {
      std::lock_guard<std::mutex> lock(lightMutex);
      window.clear();
      window.draw(roadA.shape);
      window.draw(roadB.shape);
      window.draw(roadC.shape);
      window.draw(roadD.shape);
      window.draw(lane1.shape);
      window.draw(lane2.shape);
      window.draw(lane3.shape);
      window.draw(lane4.shape);
      window.draw(lane5.shape);
      window.draw(lane6.shape);
      window.draw(lane7.shape);
      window.draw(lane8.shape);
      window.draw(lane9.shape);
      window.draw(lane10.shape);
      window.draw(lane11.shape);
      window.draw(lane12.shape);
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
      lane7.drawCars(window);
      lane8.drawCars(window);
      lane9.drawCars(window);
      lane10.drawCars(window);
      lane11.drawCars(window);
      lane12.drawCars(window);
      window.display();
    }
  }

  return 0;
}
