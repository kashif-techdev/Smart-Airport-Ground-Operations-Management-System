#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <pthread.h>
#include <semaphore.h>
#include <vector>
#include <queue>
#include <string>
#include <map>
#include <ctime>

// Resource types
enum class ResourceType {
    RUNWAY,
    GATE,
    FUEL_TRUCK,
    BAGGAGE_CREW,
    MAINTENANCE_TEAM
};

// Crew skill types
enum class CrewSkill {
    GENERAL,
    HEAVY_AIRCRAFT,
    QUICK_RESPONSE,
    INTERNATIONAL
};

// Resource status
struct ResourceStatus {
    int id;
    bool available;
    time_t lastUsed;
    CrewSkill skill;
    time_t unavailableUntil; // For degradation/breakdown
};

class ResourceManager {
private:
    // Semaphores for resource counting
    sem_t sem_runway;
    sem_t sem_gate;
    sem_t sem_fuelTruck;
    sem_t sem_baggageCrew;
    sem_t sem_maintenanceTeam;
    
    // Mutexes for resource access (mutable for const member functions)
    mutable pthread_mutex_t mutex_runway;
    mutable pthread_mutex_t mutex_gate;
    mutable pthread_mutex_t mutex_fuelTruck;
    mutable pthread_mutex_t mutex_baggageCrew;
    mutable pthread_mutex_t mutex_maintenanceTeam;
    
    // Resource pools
    std::vector<ResourceStatus> runways;
    std::vector<ResourceStatus> gates;
    std::vector<ResourceStatus> fuelTrucks;
    std::vector<ResourceStatus> baggageCrews;
    std::vector<ResourceStatus> maintenanceTeams;
    
    // Priority queues for resource requests
    std::map<ResourceType, std::queue<int>> resourceQueues;
    
    // Initial counts
    int numRunways;
    int numGates;
    int numFuelTrucks;
    int numBaggageCrews;
    int numMaintenanceTeams;
    
    // Helper functions
    ResourceStatus* findAvailableResource(std::vector<ResourceStatus>& pool, CrewSkill requiredSkill = CrewSkill::GENERAL);
    ResourceStatus* findLRUResource(std::vector<ResourceStatus>& pool);
    void markResourceUnavailable(ResourceStatus* resource, int seconds);
    
public:
    ResourceManager(int runways = 2, int gates = 6, int fuelTrucks = 3, 
                   int baggageCrews = 4, int maintenanceTeams = 3);
    ~ResourceManager();
    
    // Resource acquisition (returns resource ID, -1 if failed)
    int acquireRunway(int flightId, int priority);
    int acquireGate(int flightId, int priority);
    int acquireFuelTruck(int flightId, int priority, CrewSkill skill = CrewSkill::GENERAL);
    int acquireBaggageCrew(int flightId, int priority);
    int acquireMaintenanceTeam(int flightId, int priority, CrewSkill skill = CrewSkill::GENERAL);
    
    // Resource release
    void releaseRunway(int resourceId);
    void releaseGate(int resourceId);
    void releaseFuelTruck(int resourceId);
    void releaseBaggageCrew(int resourceId);
    void releaseMaintenanceTeam(int resourceId);
    
    // Dynamic resource management
    void addRunway();
    void addGate();
    void addFuelTruck();
    void addBaggageCrew();
    void addMaintenanceTeam();
    
    void removeRunway(int id);
    void removeGate(int id);
    void removeFuelTruck(int id);
    void removeBaggageCrew(int id);
    void removeMaintenanceTeam(int id);
    
    // Resource degradation simulation
    void simulateBreakdown(ResourceType type, int resourceId, int durationSeconds);
    
    // Status queries
    int getAvailableRunways() const;
    int getAvailableGates() const;
    int getAvailableFuelTrucks() const;
    int getAvailableBaggageCrews() const;
    int getAvailableMaintenanceTeams() const;
    
    // Total resource counts
    int getTotalRunways() const;
    int getTotalGates() const;
    int getTotalFuelTrucks() const;
    int getTotalBaggageCrews() const;
    int getTotalMaintenanceTeams() const;
    
    std::vector<ResourceStatus> getRunwayStatus() const;
    std::vector<ResourceStatus> getGateStatus() const;
    std::vector<ResourceStatus> getFuelTruckStatus() const;
    std::vector<ResourceStatus> getBaggageCrewStatus() const;
    std::vector<ResourceStatus> getMaintenanceTeamStatus() const;
};

#endif // RESOURCE_MANAGER_H

