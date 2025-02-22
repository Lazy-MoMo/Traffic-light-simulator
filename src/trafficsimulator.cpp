#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <ctime>

std::mutex lightMutex;

class TrafficLight {
public:
    sf::RectangleShape shape;
    sf::Color colors[3] = { sf::Color::Red, sf::Color::Yellow, sf::Color::Green };
    int state = 0; // 0 = Red, 1 = Yellow, 2 = Green

    TrafficLight(float x, float y, float width, float height) {
        shape.setSize(sf::Vector2f(width, height));
        shape.setPosition(x, y);
        shape.setFillColor(colors[state]);
    }

    void updateState() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::lock_guard<std::mutex> lock(lightMutex);
            state = (state + 1) % 3; // Cycle through states
            shape.setFillColor(colors[state]);
            std::cout << "Light changed to: " << (state == 0 ? "Red" : state == 1 ? "Yellow" : "Green") << std::endl;
        }
    }
};

class Road {
public:
    sf::RectangleShape shape;
    sf::Text label;

    Road(float x, float y, float width, float height, const std::string& name, sf::Font& font) {
        shape.setSize(sf::Vector2f(width, height));
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(50, 50, 50)); // Dark gray road color

        label.setFont(font);
        label.setString(name);
        label.setCharacterSize(24);
        label.setFillColor(sf::Color::White); // Set text color to white
        label.setPosition(x + width / 2 - label.getGlobalBounds().width / 2, y + height / 2 - label.getGlobalBounds().height / 2);
    }
};

class Lane {
public:
    sf::RectangleShape shape;

    Lane(float x, float y, float width, float height, sf::Color color) {
        shape.setSize(sf::Vector2f(width, height));
        shape.setPosition(x, y);
        shape.setFillColor(color); // Lane color
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
        shape.setFillColor(sf::Color::Blue); // Car color
        this->speedX = speedX;
        this->speedY = speedY;
    }

    void move() {
        shape.move(speedX, speedY);
    }

    bool isOutOfBounds(float roadWidth, float roadHeight) {
        sf::Vector2f pos = shape.getPosition();
        return (pos.x < 0 || pos.x > roadWidth || pos.y < 0 || pos.y > roadHeight);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Traffic Light Simulator", sf::Style::Fullscreen);

    sf::Font font;
    if (!font.loadFromFile("/usr/share/fonts/abattis-cantarell-fonts/Cantarell-Light.otf")) {
        std::cerr << "Error loading font\n";
        return -1;
    }

    // Traffic lights
    TrafficLight trafficLight1(450, 250, 25, 100); // Existing traffic light
    TrafficLight trafficLight2(325, 250, 25, 100); // New traffic light for horizontal road
    TrafficLight trafficLight3(350, 225, 100, 25); // New traffic light for vertical road
    TrafficLight trafficLight4(350, 350, 100, 25); // New traffic light for vertical road

    // Roads
    Road roadA(200, 250, 400, 100, "A", font); // Horizontal road x,y,width,height
    Road roadB(350, 100, 100, 400, "B", font); // Vertical road
    Road roadC(200, 250, 400, 100, "C", font); // Horizontal road x,y,width,height
    Road roadD(350, 100, 100, 400, "D", font); // Vertical road

    // Lanes for horizontal road
    Lane lane1(200, 260, 400, 20, sf::Color::White); // First lane
    Lane lane2(200, 290, 400, 20, sf::Color::Yellow); // Second lane
    Lane lane3(200, 320, 400, 20, sf::Color::White); // Third lane

    // Lanes for vertical road
    Lane lane4(360, 100, 20, 400, sf::Color::White); // First lane
    Lane lane5(390, 100, 20, 400, sf::Color::Yellow); // Second lane
    Lane lane6(420, 100, 20, 400, sf::Color::White); // Third lane

    // Threads for traffic lights
    std::thread lightThread1(&TrafficLight::updateState, &trafficLight1);
    std::thread lightThread2(&TrafficLight::updateState, &trafficLight2);
    std::thread lightThread3(&TrafficLight::updateState, &trafficLight3);
    std::thread lightThread4(&TrafficLight::updateState, &trafficLight4);

    // Cars
    std::vector<Car> cars;
    std::srand(std::time(nullptr));

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Q)
                window.close();
        }

        // Spawn cars randomly
        if (std::rand() % 100 < 2) { // 2% chance to spawn a car each frame
            int side = std::rand() % 4; // Randomly choose one of the four sides
            if (side == 0) { // Left side
                float laneY = 260 + (std::rand() % 3) * 30; // Randomly choose one of the horizontal lanes
                cars.emplace_back(200, laneY, 40, 20, 0.5f, 0.0f); // Add a new car moving horizontally
            } else if (side == 1) { // Right side
                float laneY = 260 + (std::rand() % 3) * 30; // Randomly choose one of the horizontal lanes
                cars.emplace_back(600, laneY, 40, 20, -0.5f, 0.0f); // Add a new car moving horizontally
            } else if (side == 2) { // Top side
                float laneX = 360 + (std::rand() % 3) * 30; // Randomly choose one of the vertical lanes
                cars.emplace_back(laneX, 100, 20, 40, 0.0f, 0.5f); // Add a new car moving vertically
            } else if (side == 3) { // Bottom side
                float laneX = 360 + (std::rand() % 3) * 30; // Randomly choose one of the vertical lanes
                cars.emplace_back(laneX, 500, 20, 40, 0.0f, -0.5f); // Add a new car moving vertically
            }
        }

        // Move cars and remove those that are out of bounds
        for (auto it = cars.begin(); it != cars.end();) {
            it->move();
            if (it->isOutOfBounds(800, 600)) {
                it = cars.erase(it);
            } else {
                ++it;
            }
        }

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

            for (auto& car : cars) {
                window.draw(car.shape);
            }

            window.display();
        }
    }

    lightThread1.detach();
    lightThread2.detach();
    lightThread3.detach();
    lightThread4.detach();
    return 0;
}
