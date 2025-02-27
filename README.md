# Traffic Light Simulator

A real-time traffic light simulator with moving traffic using **SDL2**, **SFML**, and **pthreads**. The simulator models an intersection with a queue-based traffic management system and priority-based lane control.

## Features

- **Four-way intersection** with roads A, B, C, and D.
- **Three lanes per road**, with lane-wise priority queuing.
- **Traffic light control** with synchronized signals for opposite lanes.
- **Queue-based traffic management** ensuring fair vehicle dispatch.
- **Priority lane handling:**
  - **AL2 priority activation** when vehicle count exceeds 10.
  - Reverts to normal when below 5.
- **Multi-threading** for smooth simulation using `pthreads`.

## Installation

### Prerequisites

Ensure you have the following installed:

- `SFML` (if using SFML for rendering)
- A C++ compiler supporting C++11 or later
- `Make` (for build configuration)

### Build Instructions

1. Clone the repository:

   ```bash
   git clone https://github.com/Lazy-MoMo/Traffic-light-simulator.git
   cd Traffic-light-simulator/src/
   ```

2. Compile the project:

   ```bash
   make
   ```

3. Run the simulator:

   ```bash
   ./traffic_simulator
   ```

## Usage

- The traffic lights switch automatically based on the queue management system.
- Vehicles move according to lane priority rules.
- The simulation dynamically adjusts priority based on real-time queue conditions.
- The interface visualizes the traffic flow and light changes in real time.

## Contribution

Contributions are welcome! Feel free to submit issues or pull requests.

### To Contribute

1. Fork the repository.
2. Create a feature branch: `git checkout -b feature-name`
3. Commit your changes: `git commit -m "Add feature"`
4. Push to your branch: `git push origin feature-name`
5. Open a pull request.

## License

This project is licensed under the [MIT License](LICENSE).

## Author

[Your Name](https://github.com/Lazy-MoMo)

---

Feel free to modify the repository link and author details accordingly!

