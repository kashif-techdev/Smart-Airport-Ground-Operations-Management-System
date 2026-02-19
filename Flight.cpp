#include "Flight.h"
#include <iostream>
#include <unistd.h>
#include <random>
#include <sstream>

Flight::Flight(int id, const std::string& flightNumber, FlightType type, 
               FlightPriority priority, ResourceManager* rm)
    : resourceManager(rm), threadsRunning(false), shouldTerminate(false) {
    info.flightId = id;
    info.flightNumber = flightNumber;
    info.type = type;
    info.priority = priority;
    info.status = FlightStatus::INCOMING;
    info.arrivalTime = time(nullptr);
    info.departureTime = 0;
    info.assignedGate = -1;
    info.assignedRunway = -1;
    info.currentActivity = "Incoming";
    info.emergency = false;
    info.emergencyReason = "";
}

Flight::~Flight() {
    stop();
    join();
}

bool Flight::start() {
    if (threadsRunning) return false;
    
    shouldTerminate = false;
    threadsRunning = true;
    
    if (pthread_create(&flightThread, nullptr, flightWorkflow, this) != 0) {
        threadsRunning = false;
        return false;
    }
    
    return true;
}

void Flight::stop() {
    shouldTerminate = true;
}

void Flight::join() {
    if (threadsRunning) {
        pthread_join(flightThread, nullptr);
        threadsRunning = false;
    }
}

void* Flight::flightWorkflow(void* arg) {
    Flight* flight = static_cast<Flight*>(arg);
    
    // Step 1: Request runway and land
    flight->requestLanding();
    if (flight->shouldTerminate) return nullptr;
    
    flight->land();
    if (flight->shouldTerminate) return nullptr;
    
    // Step 2: Request gate and park
    flight->park();
    if (flight->shouldTerminate) return nullptr;
    
    // Step 3: Start parallel tasks
    flight->startParallelTasks();
    
    // Step 4: Wait for all parallel tasks to complete
    flight->waitForParallelTasks();
    
    // Step 5: Request runway and depart
    flight->requestDeparture();
    if (flight->shouldTerminate) return nullptr;
    
    flight->depart();
    
    return nullptr;
}

void* Flight::boardingWorkflow(void* arg) {
    Flight* flight = static_cast<Flight*>(arg);
    flight->updateStatus(FlightStatus::BOARDING, "Boarding passengers");
    
    // Simulate boarding time (3-8 seconds)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(3, 8);
    int boardingTime = dis(gen);
    
    for (int i = 0; i < boardingTime && !flight->shouldTerminate; i++) {
        sleep(1);
    }
    
    return nullptr;
}

void* Flight::fuelingWorkflow(void* arg) {
    Flight* flight = static_cast<Flight*>(arg);
    int priority = flight->getPriorityValue();
    
    // Determine if specialized crew needed (for international/large aircraft)
    CrewSkill requiredSkill = (flight->info.type == FlightType::INTERNATIONAL) 
        ? CrewSkill::HEAVY_AIRCRAFT : CrewSkill::GENERAL;
    
    int truckId = flight->resourceManager->acquireFuelTruck(flight->info.flightId, priority, requiredSkill);
    
    if (truckId >= 0) {
        flight->updateStatus(FlightStatus::REFUELING, "Refueling");
        
        // Simulate refueling time (2-6 seconds)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(2, 6);
        int fuelingTime = dis(gen);
        
        for (int i = 0; i < fuelingTime && !flight->shouldTerminate; i++) {
            sleep(1);
        }
        
        flight->resourceManager->releaseFuelTruck(truckId);
    }
    
    return nullptr;
}

void* Flight::baggageWorkflow(void* arg) {
    Flight* flight = static_cast<Flight*>(arg);
    int priority = flight->getPriorityValue();
    
    int crewId = flight->resourceManager->acquireBaggageCrew(flight->info.flightId, priority);
    
    if (crewId >= 0) {
        flight->updateStatus(FlightStatus::BAGGAGE_HANDLING, "Handling baggage");
        
        // Simulate baggage handling time (2-5 seconds)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(2, 5);
        int baggageTime = dis(gen);
        
        for (int i = 0; i < baggageTime && !flight->shouldTerminate; i++) {
            sleep(1);
        }
        
        flight->resourceManager->releaseBaggageCrew(crewId);
    }
    
    return nullptr;
}

void* Flight::maintenanceWorkflow(void* arg) {
    Flight* flight = static_cast<Flight*>(arg);
    int priority = flight->getPriorityValue();
    
    // Use specialized crew for emergencies
    CrewSkill requiredSkill = flight->info.emergency 
        ? CrewSkill::QUICK_RESPONSE : CrewSkill::GENERAL;
    
    int teamId = flight->resourceManager->acquireMaintenanceTeam(flight->info.flightId, priority, requiredSkill);
    
    if (teamId >= 0) {
        flight->updateStatus(FlightStatus::MAINTENANCE, "Maintenance check");
        
        // Simulate maintenance time (1-4 seconds, faster for emergencies)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(2, 4);
        int maintenanceTime = flight->info.emergency ? 1 : dis(gen);
        
        for (int i = 0; i < maintenanceTime && !flight->shouldTerminate; i++) {
            sleep(1);
        }
        
        flight->resourceManager->releaseMaintenanceTeam(teamId);
    }
    
    return nullptr;
}

void Flight::requestLanding() {
    updateStatus(FlightStatus::REQUESTING_LANDING, "Requesting landing clearance");
    // Priority-based waiting is handled by semaphore
}

void Flight::land() {
    int priority = getPriorityValue();
    int runwayId = resourceManager->acquireRunway(info.flightId, priority);
    
    if (runwayId >= 0) {
        info.assignedRunway = runwayId;
        updateStatus(FlightStatus::LANDING, "Landing on runway " + std::to_string(runwayId));
        
        // Simulate landing time (2-4 seconds)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(2, 4);
        int landingTime = dis(gen);
        
        simulateDelay(landingTime);
        
        resourceManager->releaseRunway(runwayId);
    }
}

void Flight::park() {
    updateStatus(FlightStatus::PARKING, "Requesting gate");
    
    int priority = getPriorityValue();
    int gateId = resourceManager->acquireGate(info.flightId, priority);
    
    if (gateId >= 0) {
        info.assignedGate = gateId;
        updateStatus(FlightStatus::PARKING, "Parked at gate " + std::to_string(gateId));
        simulateDelay(1); // Parking time
    }
}

void Flight::startParallelTasks() {
    updateStatus(FlightStatus::BOARDING, "Starting ground operations");
    
    // Create threads for parallel tasks
    pthread_create(&boardingThread, nullptr, boardingWorkflow, this);
    pthread_create(&fuelingThread, nullptr, fuelingWorkflow, this);
    pthread_create(&baggageThread, nullptr, baggageWorkflow, this);
    pthread_create(&maintenanceThread, nullptr, maintenanceWorkflow, this);
}

void Flight::waitForParallelTasks() {
    pthread_join(boardingThread, nullptr);
    pthread_join(fuelingThread, nullptr);
    pthread_join(baggageThread, nullptr);
    pthread_join(maintenanceThread, nullptr);
    
    updateStatus(FlightStatus::READY_FOR_DEPARTURE, "All operations complete");
    simulateDelay(1);
}

void Flight::requestDeparture() {
    updateStatus(FlightStatus::READY_FOR_DEPARTURE, "Requesting departure clearance");
}

void Flight::depart() {
    int priority = getPriorityValue();
    int runwayId = resourceManager->acquireRunway(info.flightId, priority);
    
    if (runwayId >= 0) {
        info.assignedRunway = runwayId;
        updateStatus(FlightStatus::DEPARTING, "Departing from runway " + std::to_string(runwayId));
        
        // Release gate before departure
        if (info.assignedGate >= 0) {
            resourceManager->releaseGate(info.assignedGate);
            info.assignedGate = -1;
        }
        
        // Simulate takeoff time
        simulateDelay(2);
        
        resourceManager->releaseRunway(runwayId);
        updateStatus(FlightStatus::DEPARTED, "Departed");
        info.departureTime = time(nullptr);
    }
}

void Flight::updateStatus(FlightStatus newStatus, const std::string& activity) {
    info.status = newStatus;
    info.currentActivity = activity;
}

void Flight::simulateDelay(int seconds) {
    for (int i = 0; i < seconds && !shouldTerminate; i++) {
        sleep(1);
    }
}

int Flight::getPriorityValue() const {
    if (info.emergency) {
        return static_cast<int>(FlightPriority::HIGH);
    }
    return static_cast<int>(info.priority);
}

FlightInfo Flight::getInfo() const {
    return info;
}

int Flight::getFlightId() const {
    return info.flightId;
}

FlightStatus Flight::getStatus() const {
    return info.status;
}

FlightPriority Flight::getPriority() const {
    return info.priority;
}

void Flight::setEmergency(const std::string& reason) {
    info.emergency = true;
    info.emergencyReason = reason;
    info.priority = FlightPriority::HIGH;
}

void Flight::clearEmergency() {
    info.emergency = false;
    info.emergencyReason = "";
}

void Flight::setStatus(FlightStatus status) {
    info.status = status;
}

