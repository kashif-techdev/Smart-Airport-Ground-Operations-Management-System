#ifndef AIRPORT_H
#define AIRPORT_H

#include "ResourceManager.h"
#include "Flight.h"
#include <vector>
#include <map>
#include <pthread.h>
#include <ctime>

class Airport {
private:
    ResourceManager* resourceManager;
    std::vector<Flight*> flights;
    std::map<int, FlightInfo> flightStatuses;
    mutable pthread_mutex_t mutex_flights;
    mutable pthread_mutex_t mutex_status;
    
    
    // Event simulation
    pthread_t eventThread;
    bool eventsRunning;
    mutable bool shouldTerminate;
    
    // Cleanup thread
    pthread_t cleanupThread;
    
    // Statistics
    int totalFlights;
    int completedFlights;
    time_t simulationStartTime;
    
    // Event simulation thread
    static void* eventSimulator(void* arg);
    void simulateEvents();
    
    // Cleanup thread
    static void* cleanupWorker(void* arg);
    
    // Helper functions
    void updateFlightStatus(int flightId, const FlightInfo& info);
    std::string generateFlightNumber();
    void cleanupDepartedFlights();
    
public:
    Airport(int runways = 2, int gates = 6, int fuelTrucks = 3, 
            int baggageCrews = 4, int maintenanceTeams = 3);
    ~Airport();
    
    // Flight management
    int addFlight(const std::string& flightNumber, FlightType type, FlightPriority priority);
    void removeFlight(int flightId);
    void startFlight(int flightId);
    
    // Status queries
    std::vector<FlightInfo> getAllFlightStatuses() const;
    FlightInfo getFlightStatus(int flightId) const;
    
    // Resource status
    ResourceManager* getResourceManager() const;
    
    // Event simulation
    void startEventSimulation();
    void stopEventSimulation();
    
    // Statistics
    int getTotalFlights() const;
    int getCompletedFlights() const;
    int getActiveFlights() const;
    time_t getSimulationTime() const;
    
    // Dynamic resource management
    void addRunway();
    void addGate();
    void addFuelTruck();
    void addBaggageCrew();
    void addMaintenanceTeam();
};

#endif // AIRPORT_H

