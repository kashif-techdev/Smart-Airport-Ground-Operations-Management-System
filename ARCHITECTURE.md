# System Architecture Documentation

## Overview

The Smart Airport Ground Operations Management System is a multi-threaded simulation that demonstrates core Operating System concepts including process/thread management, synchronization, resource allocation, priority scheduling, and memory management.

## System Components

### 1. ResourceManager (ResourceManager.h/cpp)

**Purpose**: Manages all limited airport resources with thread-safe access control.

**Key Features**:
- Semaphore-based resource counting
- Mutex-protected resource pools
- Skill-based crew assignment with LRU fallback
- Dynamic resource addition/removal
- Resource degradation simulation

**Data Structures**:
- `std::vector<ResourceStatus>` for each resource type
- Semaphores for each resource type (sem_runway, sem_gate, etc.)
- Mutexes for critical sections (mutex_runway, mutex_gate, etc.)

**Synchronization**:
- Semaphores track available resource count
- Mutexes protect resource status updates
- Time-based availability tracking for degraded resources

### 2. Flight (Flight.h/cpp)

**Purpose**: Represents each flight as a thread with a complete workflow.

**Thread Structure**:
- Main flight thread: Orchestrates the entire flight lifecycle
- Boarding thread: Simulates passenger boarding
- Fueling thread: Handles refueling operations
- Baggage thread: Manages baggage handling
- Maintenance thread: Performs maintenance checks

**Workflow Stages**:
1. **INCOMING**: Flight approaching airport
2. **REQUESTING_LANDING**: Waiting for runway clearance
3. **LANDING**: Using runway to land
4. **PARKING**: Moving to assigned gate
5. **Parallel Operations** (run concurrently):
   - **BOARDING**: Passenger boarding
   - **REFUELING**: Aircraft refueling
   - **BAGGAGE_HANDLING**: Loading/unloading baggage
   - **MAINTENANCE**: Safety checks
6. **READY_FOR_DEPARTURE**: All operations complete
7. **DEPARTING**: Using runway to take off
8. **DEPARTED**: Flight has left

**Priority Handling**:
- Emergency flights automatically get HIGH priority
- Priority affects resource acquisition order
- Semaphores naturally implement priority through waiting queues

### 3. Airport (Airport.h/cpp)

**Purpose**: Main controller managing flights, events, and system state.

**Threads**:
- **Event Simulation Thread**: Generates random events (weather, breakdowns, emergencies)
- **Cleanup Thread**: Removes departed flights from the system

**Event Types**:
- Weather delays (runway closures for 5-15 seconds)
- Fuel truck breakdowns
- Emergency flight situations
- Dynamic resource addition during peak hours

**Memory Management**:
- Shared memory within process for all flight statuses
- Mutex-protected status map
- Automatic cleanup of completed flights

### 4. UI (UI.h/cpp)

**Purpose**: Real-time terminal-based user interface using ncurses.

**Display Sections**:
- **Header**: System title, simulation time, flight statistics
- **Resource Status**: Available resources for each type
- **Flight Status**: Current status of all active flights
- **System Log**: Timestamped event log

**Features**:
- Color-coded priority display (Red=High, Yellow=Medium, Green=Low)
- Real-time updates (0.5 second refresh rate)
- Scrollable log window
- Responsive layout

## Synchronization Mechanisms

### Semaphores
Used for resource counting:
- `sem_runway`: Tracks available runways
- `sem_gate`: Tracks available gates
- `sem_fuelTruck`: Tracks available fuel trucks
- `sem_baggageCrew`: Tracks available baggage crews
- `sem_maintenanceTeam`: Tracks available maintenance teams

### Mutexes
Protect critical sections:
- `mutex_runway`: Protects runway status updates
- `mutex_gate`: Protects gate assignments
- `mutex_fuelTruck`: Protects fuel truck assignments
- `mutex_baggageCrew`: Protects crew assignments
- `mutex_maintenanceTeam`: Protects team assignments
- `mutex_flights`: Protects flight list in Airport
- `mutex_status`: Protects flight status map
- `mutex_log`: Protects UI log messages

### Deadlock Prevention
- Ordered resource acquisition (runway → gate → services)
- Timeout mechanisms for resource waiting
- No circular dependencies in resource requests

## Priority Scheduling Algorithm

### Priority Levels
1. **HIGH (3)**: Emergency situations
   - Low fuel emergency
   - Bird strike
   - Mechanical issues
   - Medical emergencies

2. **MEDIUM (2)**: International flights

3. **LOW (1)**: Domestic/non-urgent flights

### Implementation
- Priority stored in FlightInfo structure
- Semaphore wait operations naturally queue by arrival time
- Emergency flights can interrupt normal operations
- First-Come-First-Served for same priority

## Resource Allocation Strategy

### Skill-Based Assignment
- **Heavy Aircraft Crew**: Preferred for international flights
- **Quick Response Team**: Assigned to emergency maintenance
- **LRU Fallback**: If specialized crew unavailable, use least recently used general crew

### Dynamic Resource Management
- Resources can be added during simulation (peak hours)
- Resources can become unavailable (breakdowns, rest periods)
- System adapts automatically to resource changes

## Memory Management

### Shared Memory Structure
All data structures exist in the same process:
- Flight status map: `std::map<int, FlightInfo>`
- Resource pools: `std::vector<ResourceStatus>`
- Flight list: `std::vector<Flight*>`

### Memory Efficiency
- Automatic cleanup of departed flights
- Bounded log message buffer (100 messages)
- Efficient status updates without copying

## Error Handling

### Resource Unavailability
- Flights wait using semaphores (blocking wait)
- No busy-waiting
- Graceful degradation when resources unavailable

### Thread Safety
- All shared data protected by mutexes
- Atomic operations for status updates
- Proper cleanup on termination

### Crash Prevention
- Signal handlers for graceful shutdown
- Resource release on flight completion
- No memory leaks (RAII principles)

## Thread Flow Diagram

```
Main Thread
├── Airport Constructor
│   ├── ResourceManager initialization
│   ├── Event simulation thread start
│   └── Cleanup thread start
├── UI Thread start
└── Flight generation loop
    └── For each flight:
        └── Flight Thread
            ├── Landing (acquire runway)
            ├── Parking (acquire gate)
            ├── Parallel Tasks:
            │   ├── Boarding Thread
            │   ├── Fueling Thread (acquire truck)
            │   ├── Baggage Thread (acquire crew)
            │   └── Maintenance Thread (acquire team)
            ├── Departure (acquire runway)
            └── Cleanup
```

## Performance Considerations

- Non-blocking UI updates (separate thread)
- Efficient resource lookup (O(n) for small resource pools)
- Minimal locking overhead
- Real-time simulation suitable for demonstration

## Testing Scenarios

1. **Normal Operations**: Multiple flights with different priorities
2. **Emergency Handling**: Emergency flight interrupts normal operations
3. **Resource Contention**: All gates occupied, flights wait
4. **Dynamic Events**: Weather delay, resource breakdown
5. **Peak Hours**: Resource addition during high traffic
6. **Specialized Crews**: International flight gets heavy aircraft crew

## Future Enhancements

- Network-based multi-airport simulation
- Database persistence for flight history
- Web-based UI alternative
- Machine learning for optimal resource allocation
- Real-time weather data integration

