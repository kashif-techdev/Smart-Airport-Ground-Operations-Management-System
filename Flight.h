#ifndef FLIGHT_H
#define FLIGHT_H

#include <pthread.h>
#include <string>
#include <ctime>
#include "ResourceManager.h"

// Flight priority levels
enum class FlightPriority {
    HIGH = 3,      // Emergency (low fuel, bird strike, mechanical)
    MEDIUM = 2,    // International flights
    LOW = 1        // Domestic/non-urgent
};

// Flight status
enum class FlightStatus {
    INCOMING,
    REQUESTING_LANDING,
    LANDING,
    PARKING,
    REFUELING,
    MAINTENANCE,
    BOARDING,
    BAGGAGE_HANDLING,
    READY_FOR_DEPARTURE,
    DEPARTING,
    DEPARTED
};

// Flight type
enum class FlightType {
    DOMESTIC,
    INTERNATIONAL,
    CARGO,
    EMERGENCY
};

struct FlightInfo {
    int flightId;
    std::string flightNumber;
    FlightType type;
    FlightPriority priority;
    FlightStatus status;
    time_t arrivalTime;
    time_t departureTime;
    int assignedGate;
    int assignedRunway;
    std::string currentActivity;
    bool emergency;
    std::string emergencyReason;
};

class Flight {
private:
    FlightInfo info;
    ResourceManager* resourceManager;
    pthread_t flightThread;
    pthread_t boardingThread;
    pthread_t fuelingThread;
    pthread_t baggageThread;
    pthread_t maintenanceThread;
    
    bool threadsRunning;
    bool shouldTerminate;
    
    // Thread functions
    static void* flightWorkflow(void* arg);
    static void* boardingWorkflow(void* arg);
    static void* fuelingWorkflow(void* arg);
    static void* baggageWorkflow(void* arg);
    static void* maintenanceWorkflow(void* arg);
    
    // Workflow steps
    void requestLanding();
    void land();
    void park();
    void startParallelTasks();
    void waitForParallelTasks();
    void requestDeparture();
    void depart();
    
    // Helper functions
    void updateStatus(FlightStatus newStatus, const std::string& activity);
    void simulateDelay(int seconds);
    int getPriorityValue() const;
    
public:
    Flight(int id, const std::string& flightNumber, FlightType type, 
           FlightPriority priority, ResourceManager* rm);
    ~Flight();
    
    // Start flight thread
    bool start();
    void stop();
    void join();
    
    // Getters
    FlightInfo getInfo() const;
    int getFlightId() const;
    FlightStatus getStatus() const;
    FlightPriority getPriority() const;
    
    // Emergency handling
    void setEmergency(const std::string& reason);
    void clearEmergency();
    
    // Status updates
    void setStatus(FlightStatus status);
};

#endif // FLIGHT_H

