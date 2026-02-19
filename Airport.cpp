#include "Airport.h"
#include <iostream>
#include <random>
#include <sstream>
#include <algorithm>
#include <unistd.h>

Airport::Airport(int runways, int gates, int fuelTrucks, 
                 int baggageCrews, int maintenanceTeams)
    : resourceManager(new ResourceManager(runways, gates, fuelTrucks, baggageCrews, maintenanceTeams)),
      eventsRunning(false), shouldTerminate(false),
      totalFlights(0), completedFlights(0) {
    
    pthread_mutex_init(&mutex_flights, nullptr);
    pthread_mutex_init(&mutex_status, nullptr);
    simulationStartTime = time(nullptr);
    
    // Start cleanup thread
    pthread_create(&cleanupThread, nullptr, cleanupWorker, this);
}

Airport::~Airport() {
    stopEventSimulation();
    shouldTerminate = true;
    
    // Wait for cleanup thread
    pthread_join(cleanupThread, nullptr);
    
    // Stop all flights
    pthread_mutex_lock(&mutex_flights);
    for (auto* flight : flights) {
        flight->stop();
        flight->join();
        delete flight;
    }
    flights.clear();
    pthread_mutex_unlock(&mutex_flights);
    
    delete resourceManager;
    pthread_mutex_destroy(&mutex_flights);
    pthread_mutex_destroy(&mutex_status);
}

std::string Airport::generateFlightNumber() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> airlineDis(1, 9);
    std::uniform_int_distribution<> numberDis(100, 9999);
    
    char airline = 'A' + airlineDis(gen) - 1;
    int number = numberDis(gen);
    
    std::ostringstream oss;
    oss << airline << number;
    return oss.str();
}

int Airport::addFlight(const std::string& flightNumber, FlightType type, FlightPriority priority) {
    pthread_mutex_lock(&mutex_flights);
    
    int flightId = totalFlights++;
    std::string fNumber = flightNumber.empty() ? generateFlightNumber() : flightNumber;
    
    Flight* flight = new Flight(flightId, fNumber, type, priority, resourceManager);
    flights.push_back(flight);
    
    // Initialize status
    FlightInfo info = flight->getInfo();
    updateFlightStatus(flightId, info);
    
    pthread_mutex_unlock(&mutex_flights);
    
    return flightId;
}

void Airport::startFlight(int flightId) {
    pthread_mutex_lock(&mutex_flights);
    
    for (auto* flight : flights) {
        if (flight->getFlightId() == flightId) {
            flight->start();
            break;
        }
    }
    
    pthread_mutex_unlock(&mutex_flights);
}

void Airport::removeFlight(int flightId) {
    pthread_mutex_lock(&mutex_flights);
    
    auto it = std::remove_if(flights.begin(), flights.end(),
        [flightId](Flight* f) {
            if (f->getFlightId() == flightId) {
                f->stop();
                f->join();
                delete f;
                return true;
            }
            return false;
        });
    
    flights.erase(it, flights.end());
    
    pthread_mutex_unlock(&mutex_flights);
    
    pthread_mutex_lock(&mutex_status);
    flightStatuses.erase(flightId);
    completedFlights++;
    pthread_mutex_unlock(&mutex_status);
}

void Airport::updateFlightStatus(int flightId, const FlightInfo& info) {
    pthread_mutex_lock(&mutex_status);
    flightStatuses[flightId] = info;
    
    // Check if flight has departed
    if (info.status == FlightStatus::DEPARTED && info.departureTime > 0) {
        // Flight will be removed in next cleanup cycle
    }
    
    pthread_mutex_unlock(&mutex_status);
}

std::vector<FlightInfo> Airport::getAllFlightStatuses() const {
    pthread_mutex_lock(&mutex_status);
    
    // Update statuses from active flights
    std::vector<FlightInfo> statuses;
    for (const auto& pair : flightStatuses) {
        statuses.push_back(pair.second);
    }
    
    pthread_mutex_unlock(&mutex_status);
    
    // Also get current status from active flights
    pthread_mutex_lock(&mutex_flights);
    for (auto* flight : flights) {
        FlightInfo info = flight->getInfo();
        bool found = false;
        for (auto& status : statuses) {
            if (status.flightId == info.flightId) {
                status = info;
                found = true;
                break;
            }
        }
        if (!found) {
            statuses.push_back(info);
        }
    }
    pthread_mutex_unlock(&mutex_flights);
    
    return statuses;
}

FlightInfo Airport::getFlightStatus(int flightId) const {
    pthread_mutex_lock(&mutex_status);
    auto it = flightStatuses.find(flightId);
    FlightInfo info;
    if (it != flightStatuses.end()) {
        info = it->second;
    }
    pthread_mutex_unlock(&mutex_status);
    
    // Also check active flights
    pthread_mutex_lock(&mutex_flights);
    for (auto* flight : flights) {
        if (flight->getFlightId() == flightId) {
            info = flight->getInfo();
            break;
        }
    }
    pthread_mutex_unlock(&mutex_flights);
    
    return info;
}

ResourceManager* Airport::getResourceManager() const {
    return resourceManager;
}

void* Airport::eventSimulator(void* arg) {
    Airport* airport = static_cast<Airport*>(arg);
    airport->simulateEvents();
    return nullptr;
}

void Airport::simulateEvents() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> eventDis(1, 100);
    std::uniform_int_distribution<> delayDis(5, 15);
    std::uniform_int_distribution<> resourceDis(0, 4);
    
    while (!shouldTerminate) {
        sleep(10); // Check for events every 10 seconds
        
        if (shouldTerminate) break;
        
        int event = eventDis(gen);
        
        // 10% chance of weather delay (runway closure)
        if (event <= 10) {
            int runwayId = resourceDis(gen) % 2;
            int delay = delayDis(gen);
            resourceManager->simulateBreakdown(ResourceType::RUNWAY, runwayId, delay);
        }
        // 5% chance of fuel truck breakdown
        else if (event <= 15) {
            int truckId = resourceDis(gen) % 3;
            int delay = delayDis(gen);
            resourceManager->simulateBreakdown(ResourceType::FUEL_TRUCK, truckId, delay);
        }
        // 5% chance of emergency flight
        else if (event <= 20) {
            pthread_mutex_lock(&mutex_flights);
            if (!flights.empty()) {
                std::uniform_int_distribution<> flightDis(0, flights.size() - 1);
                int flightIdx = flightDis(gen);
                std::vector<std::string> reasons = {
                    "Low fuel emergency",
                    "Bird strike",
                    "Mechanical issue",
                    "Medical emergency"
                };
                std::uniform_int_distribution<> reasonDis(0, reasons.size() - 1);
                flights[flightIdx]->setEmergency(reasons[reasonDis(gen)]);
            }
            pthread_mutex_unlock(&mutex_flights);
        }
        // 3% chance of adding resource during peak hours
        else if (event <= 23) {
            int resourceType = resourceDis(gen);
            switch (resourceType) {
                case 0: addRunway(); break;
                case 1: addGate(); break;
                case 2: addFuelTruck(); break;
                case 3: addBaggageCrew(); break;
                case 4: addMaintenanceTeam(); break;
            }
        }
    }
}

void Airport::startEventSimulation() {
    if (eventsRunning) return;
    
    eventsRunning = true;
    shouldTerminate = false;
    pthread_create(&eventThread, nullptr, eventSimulator, this);
}

void Airport::stopEventSimulation() {
    if (!eventsRunning) return;
    
    shouldTerminate = true;
    if (eventsRunning) {
        pthread_join(eventThread, nullptr);
        eventsRunning = false;
    }
}

int Airport::getTotalFlights() const {
    return totalFlights;
}

int Airport::getCompletedFlights() const {
    return completedFlights;
}

int Airport::getActiveFlights() const {
    pthread_mutex_lock(&mutex_flights);
    int count = flights.size();
    pthread_mutex_unlock(&mutex_flights);
    return count;
}

time_t Airport::getSimulationTime() const {
    return time(nullptr) - simulationStartTime;
}

void Airport::addRunway() {
    resourceManager->addRunway();
}

void Airport::addGate() {
    resourceManager->addGate();
}

void Airport::addFuelTruck() {
    resourceManager->addFuelTruck();
}

void Airport::addBaggageCrew() {
    resourceManager->addBaggageCrew();
}

void Airport::addMaintenanceTeam() {
    resourceManager->addMaintenanceTeam();
}

void* Airport::cleanupWorker(void* arg) {
    Airport* airport = static_cast<Airport*>(arg);
    
    while (!airport->shouldTerminate) {
        sleep(5); // Cleanup every 5 seconds
        airport->cleanupDepartedFlights();
    }
    
    return nullptr;
}

void Airport::cleanupDepartedFlights() {
    pthread_mutex_lock(&mutex_flights);
    
    std::vector<int> toRemove;
    for (auto* flight : flights) {
        FlightInfo info = flight->getInfo();
        if (info.status == FlightStatus::DEPARTED && info.departureTime > 0) {
            // Give it a few seconds before removing
            if (time(nullptr) - info.departureTime > 3) {
                toRemove.push_back(flight->getFlightId());
            }
        }
    }
    
    pthread_mutex_unlock(&mutex_flights);
    
    // Remove departed flights
    for (int flightId : toRemove) {
        removeFlight(flightId);
    }
}

