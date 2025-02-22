#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <mutex>

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

    Road(float x, float y, float width, float height) {
        shape.setSize(sf::Vector2f(width, height));
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(50, 50, 50)); // Dark gray road color
    
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

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Traffic Light Simulator",sf::Style::Fullscreen);

    // Traffic lights
    TrafficLight trafficLight1(450,250,25,100); // Existing traffic light
    TrafficLight trafficLight2(325,250,25,100); // New traffic light for horizontal road
    TrafficLight trafficLight3(350,225,100,25); // New traffic light for vertical road
    TrafficLight trafficLight4(350,350,100,25); // New traffic light for vertical road

    // Roads
    Road road1(200, 250, 400, 100); // Horizontal road x,y,width,height
    Road road2(350, 100, 100, 400); // Vertical road

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

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Q)
                window.close();
        }

        {
            std::lock_guard<std::mutex> lock(lightMutex);
            window.clear();
            window.draw(road1.shape);
            window.draw(road2.shape);
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
      
            window.display();
        }
    }

    lightThread1.detach();
    lightThread2.detach();
    lightThread3.detach();
    return 0;
}
