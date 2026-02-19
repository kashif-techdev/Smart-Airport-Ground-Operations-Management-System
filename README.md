# Smart Airport Ground Operations Management System

A comprehensive simulation of airport ground operations using C++ and Operating System concepts including threads, semaphores, mutexes, and priority scheduling.

## Features

- **Multi-threaded Flight Management**: Each flight runs as a separate thread with parallel sub-tasks
- **Resource Management**: Limited resources (runways, gates, fuel trucks, baggage crews, maintenance teams) with semaphore-based allocation
- **Priority Scheduling**: High/Medium/Low priority system for emergency and international flights
- **Dynamic Resource Management**: Resources can be added/removed during simulation
- **Skill-based Crew Assignment**: Specialized crews for heavy aircraft and emergency response
- **Real-time UI**: Beautiful ncurses-based terminal interface showing live airport status
- **Event Simulation**: Random events like weather delays, breakdowns, and emergencies
- **Error Handling**: Deadlock prevention and graceful resource management

## Requirements

- Ubuntu Linux (tested on Ubuntu 20.04+)
- GCC compiler with C++11 support
- pthread library
- ncurses library

## Installation

### Install Dependencies

```bash
make install-deps
```

Or manually:
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses5-dev
```

## Building

```bash
make
```

This will create the `airport_simulator` executable.

## Running

```bash
./airport_simulator
```

Press `Ctrl+C` to exit the simulation.

## System Architecture

### Components

1. **ResourceManager**: Manages all airport resources with semaphores and mutexes
2. **Flight**: Represents each flight as a thread with multiple workflow stages
3. **Airport**: Main controller managing flights and events
4. **UI**: Real-time display using ncurses

### Flight Workflow

Each flight goes through these stages:
1. Request landing clearance
2. Land on runway
3. Park at gate
4. Parallel tasks:
   - Passenger boarding
   - Refueling
   - Baggage handling
   - Maintenance check
5. Request departure clearance
6. Depart from runway

### Resource Types

- **Runways**: 2 (default)
- **Gates**: 6 (default)
- **Fuel Trucks**: 3 (default, 1 specialized for heavy aircraft)
- **Baggage Crews**: 4 (default)
- **Maintenance Teams**: 3 (default, 1 specialized for quick response)

### Priority Levels

- **HIGH (3)**: Emergency flights (low fuel, bird strike, mechanical issues)
- **MEDIUM (2)**: International flights
- **LOW (1)**: Domestic/non-urgent flights

## Project Structure

```
.
├── main.cpp              # Entry point and main simulation loop
├── Airport.h/cpp          # Airport management and event simulation
├── Flight.h/cpp           # Flight thread and workflow
├── ResourceManager.h/cpp  # Resource allocation and management
├── UI.h/cpp              # User interface using ncurses
├── Makefile              # Build configuration
└── README.md             # This file
```

## Key OS Concepts Used

1. **Threads**: pthread library for concurrent flight operations
2. **Semaphores**: Resource counting and synchronization
3. **Mutexes**: Critical section protection
4. **Shared Memory**: Flight status and resource state
5. **Priority Scheduling**: ATC-like priority-based resource allocation
6. **Deadlock Prevention**: Ordered resource acquisition

## Simulation Events

The system simulates various real-world events:
- Weather delays (runway closures)
- Fuel truck breakdowns
- Emergency flight situations
- Dynamic resource addition during peak hours
- Maintenance team rest periods

## Cleanup

```bash
make clean
```

## Notes

- The simulation runs indefinitely until interrupted
- Flights are automatically generated and processed
- Resource availability is displayed in real-time
- Emergency flights get immediate priority access
- Specialized crews are automatically assigned when available

## Troubleshooting

If you encounter compilation errors:
1. Ensure all dependencies are installed: `make install-deps`
2. Check that you're using GCC 4.8+ or Clang 3.3+
3. Verify ncurses is installed: `dpkg -l | grep ncurses`

If the UI doesn't display correctly:
- Ensure your terminal supports colors
- Try resizing your terminal window
- Minimum terminal size: 80x24 characters

