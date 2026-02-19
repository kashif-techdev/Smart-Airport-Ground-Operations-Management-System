#include "ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <unistd.h>

ResourceManager::ResourceManager(int rwys, int gts, int ft, 
                                 int bc, int mt)
    : numRunways(rwys), numGates(gts), numFuelTrucks(ft),
      numBaggageCrews(bc), numMaintenanceTeams(mt) {
    
    // Initialize semaphores
    sem_init(&sem_runway, 0, rwys);
    sem_init(&sem_gate, 0, gts);
    sem_init(&sem_fuelTruck, 0, ft);
    sem_init(&sem_baggageCrew, 0, bc);
    sem_init(&sem_maintenanceTeam, 0, mt);
    
    // Initialize mutexes
    pthread_mutex_init(&mutex_runway, nullptr);
    pthread_mutex_init(&mutex_gate, nullptr);
    pthread_mutex_init(&mutex_fuelTruck, nullptr);
    pthread_mutex_init(&mutex_baggageCrew, nullptr);
    pthread_mutex_init(&mutex_maintenanceTeam, nullptr);
    
    // Initialize runways
    for (int i = 0; i < rwys; i++) {
        runways.push_back({i, true, 0, CrewSkill::GENERAL, 0});
    }
    
    // Initialize gates
    for (int i = 0; i < gts; i++) {
        gates.push_back({i, true, 0, CrewSkill::GENERAL, 0});
    }
    
    // Initialize fuel trucks (mix of general and specialized)
    for (int i = 0; i < ft; i++) {
        CrewSkill skill = (i == 0) ? CrewSkill::HEAVY_AIRCRAFT : CrewSkill::GENERAL;
        fuelTrucks.push_back({i, true, 0, skill, 0});
    }
    
    // Initialize baggage crews
    for (int i = 0; i < bc; i++) {
        baggageCrews.push_back({i, true, 0, CrewSkill::GENERAL, 0});
    }
    
    // Initialize maintenance teams (mix of general and specialized)
    for (int i = 0; i < mt; i++) {
        CrewSkill skill = (i == 0) ? CrewSkill::QUICK_RESPONSE : CrewSkill::GENERAL;
        maintenanceTeams.push_back({i, true, 0, skill, 0});
    }
}

ResourceManager::~ResourceManager() {
    sem_destroy(&sem_runway);
    sem_destroy(&sem_gate);
    sem_destroy(&sem_fuelTruck);
    sem_destroy(&sem_baggageCrew);
    sem_destroy(&sem_maintenanceTeam);
    
    pthread_mutex_destroy(&mutex_runway);
    pthread_mutex_destroy(&mutex_gate);
    pthread_mutex_destroy(&mutex_fuelTruck);
    pthread_mutex_destroy(&mutex_baggageCrew);
    pthread_mutex_destroy(&mutex_maintenanceTeam);
}

ResourceStatus* ResourceManager::findAvailableResource(std::vector<ResourceStatus>& pool, CrewSkill requiredSkill) {
    time_t now = time(nullptr);
    
    // First, try to find specialized resource
    if (requiredSkill != CrewSkill::GENERAL) {
        for (auto& resource : pool) {
            if (resource.available && resource.skill == requiredSkill && 
                resource.unavailableUntil <= now) {
                return &resource;
            }
        }
    }
    
    // Fallback to any available resource
    for (auto& resource : pool) {
        if (resource.available && resource.unavailableUntil <= now) {
            return &resource;
        }
    }
    
    return nullptr;
}

ResourceStatus* ResourceManager::findLRUResource(std::vector<ResourceStatus>& pool) {
    ResourceStatus* lru = nullptr;
    time_t oldestTime = time(nullptr);
    
    for (auto& resource : pool) {
        if (resource.available && resource.lastUsed < oldestTime) {
            oldestTime = resource.lastUsed;
            lru = &resource;
        }
    }
    
    return lru;
}

void ResourceManager::markResourceUnavailable(ResourceStatus* resource, int seconds) {
    if (resource) {
        resource->unavailableUntil = time(nullptr) + seconds;
    }
}

int ResourceManager::acquireRunway(int flightId, int priority) {
    sem_wait(&sem_runway);
    pthread_mutex_lock(&mutex_runway);
    
    time_t now = time(nullptr);
    for (auto& runway : runways) {
        if (runway.available && runway.unavailableUntil <= now) {
            runway.available = false;
            runway.lastUsed = now;
            pthread_mutex_unlock(&mutex_runway);
            return runway.id;
        }
    }
    
    pthread_mutex_unlock(&mutex_runway);
    return -1; // Should not happen if semaphore works correctly
}

void ResourceManager::releaseRunway(int resourceId) {
    pthread_mutex_lock(&mutex_runway);
    if (resourceId >= 0 && resourceId < (int)runways.size()) {
        runways[resourceId].available = true;
        runways[resourceId].lastUsed = time(nullptr);
    }
    pthread_mutex_unlock(&mutex_runway);
    sem_post(&sem_runway);
}

int ResourceManager::acquireGate(int flightId, int priority) {
    sem_wait(&sem_gate);
    pthread_mutex_lock(&mutex_gate);
    
    time_t now = time(nullptr);
    for (auto& gate : gates) {
        if (gate.available && gate.unavailableUntil <= now) {
            gate.available = false;
            gate.lastUsed = now;
            pthread_mutex_unlock(&mutex_gate);
            return gate.id;
        }
    }
    
    pthread_mutex_unlock(&mutex_gate);
    return -1;
}

void ResourceManager::releaseGate(int resourceId) {
    pthread_mutex_lock(&mutex_gate);
    if (resourceId >= 0 && resourceId < (int)gates.size()) {
        gates[resourceId].available = true;
        gates[resourceId].lastUsed = time(nullptr);
    }
    pthread_mutex_unlock(&mutex_gate);
    sem_post(&sem_gate);
}

int ResourceManager::acquireFuelTruck(int flightId, int priority, CrewSkill skill) {
    sem_wait(&sem_fuelTruck);
    pthread_mutex_lock(&mutex_fuelTruck);
    
    ResourceStatus* truck = findAvailableResource(fuelTrucks, skill);
    if (!truck && skill != CrewSkill::GENERAL) {
        truck = findLRUResource(fuelTrucks);
    }
    
    if (truck) {
        truck->available = false;
        truck->lastUsed = time(nullptr);
        int id = truck->id;
        pthread_mutex_unlock(&mutex_fuelTruck);
        return id;
    }
    
    pthread_mutex_unlock(&mutex_fuelTruck);
    sem_post(&sem_fuelTruck);
    return -1;
}

void ResourceManager::releaseFuelTruck(int resourceId) {
    pthread_mutex_lock(&mutex_fuelTruck);
    if (resourceId >= 0 && resourceId < (int)fuelTrucks.size()) {
        fuelTrucks[resourceId].available = true;
        fuelTrucks[resourceId].lastUsed = time(nullptr);
    }
    pthread_mutex_unlock(&mutex_fuelTruck);
    sem_post(&sem_fuelTruck);
}

int ResourceManager::acquireBaggageCrew(int flightId, int priority) {
    sem_wait(&sem_baggageCrew);
    pthread_mutex_lock(&mutex_baggageCrew);
    
    ResourceStatus* crew = findAvailableResource(baggageCrews);
    if (crew) {
        crew->available = false;
        crew->lastUsed = time(nullptr);
        int id = crew->id;
        pthread_mutex_unlock(&mutex_baggageCrew);
        return id;
    }
    
    pthread_mutex_unlock(&mutex_baggageCrew);
    sem_post(&sem_baggageCrew);
    return -1;
}

void ResourceManager::releaseBaggageCrew(int resourceId) {
    pthread_mutex_lock(&mutex_baggageCrew);
    if (resourceId >= 0 && resourceId < (int)baggageCrews.size()) {
        baggageCrews[resourceId].available = true;
        baggageCrews[resourceId].lastUsed = time(nullptr);
    }
    pthread_mutex_unlock(&mutex_baggageCrew);
    sem_post(&sem_baggageCrew);
}

int ResourceManager::acquireMaintenanceTeam(int flightId, int priority, CrewSkill skill) {
    sem_wait(&sem_maintenanceTeam);
    pthread_mutex_lock(&mutex_maintenanceTeam);
    
    ResourceStatus* team = findAvailableResource(maintenanceTeams, skill);
    if (!team && skill != CrewSkill::GENERAL) {
        team = findLRUResource(maintenanceTeams);
    }
    
    if (team) {
        team->available = false;
        team->lastUsed = time(nullptr);
        int id = team->id;
        pthread_mutex_unlock(&mutex_maintenanceTeam);
        return id;
    }
    
    pthread_mutex_unlock(&mutex_maintenanceTeam);
    sem_post(&sem_maintenanceTeam);
    return -1;
}

void ResourceManager::releaseMaintenanceTeam(int resourceId) {
    pthread_mutex_lock(&mutex_maintenanceTeam);
    if (resourceId >= 0 && resourceId < (int)maintenanceTeams.size()) {
        maintenanceTeams[resourceId].available = true;
        maintenanceTeams[resourceId].lastUsed = time(nullptr);
    }
    pthread_mutex_unlock(&mutex_maintenanceTeam);
    sem_post(&sem_maintenanceTeam);
}

void ResourceManager::addRunway() {
    pthread_mutex_lock(&mutex_runway);
    int newId = runways.size();
    runways.push_back({newId, true, time(nullptr), CrewSkill::GENERAL, 0});
    numRunways++;
    sem_post(&sem_runway);
    pthread_mutex_unlock(&mutex_runway);
}

void ResourceManager::addGate() {
    pthread_mutex_lock(&mutex_gate);
    int newId = gates.size();
    gates.push_back({newId, true, time(nullptr), CrewSkill::GENERAL, 0});
    numGates++;
    sem_post(&sem_gate);
    pthread_mutex_unlock(&mutex_gate);
}

void ResourceManager::addFuelTruck() {
    pthread_mutex_lock(&mutex_fuelTruck);
    int newId = fuelTrucks.size();
    fuelTrucks.push_back({newId, true, time(nullptr), CrewSkill::GENERAL, 0});
    numFuelTrucks++;
    sem_post(&sem_fuelTruck);
    pthread_mutex_unlock(&mutex_fuelTruck);
}

void ResourceManager::addBaggageCrew() {
    pthread_mutex_lock(&mutex_baggageCrew);
    int newId = baggageCrews.size();
    baggageCrews.push_back({newId, true, time(nullptr), CrewSkill::GENERAL, 0});
    numBaggageCrews++;
    sem_post(&sem_baggageCrew);
    pthread_mutex_unlock(&mutex_baggageCrew);
}

void ResourceManager::addMaintenanceTeam() {
    pthread_mutex_lock(&mutex_maintenanceTeam);
    int newId = maintenanceTeams.size();
    maintenanceTeams.push_back({newId, true, time(nullptr), CrewSkill::GENERAL, 0});
    numMaintenanceTeams++;
    sem_post(&sem_maintenanceTeam);
    pthread_mutex_unlock(&mutex_maintenanceTeam);
}

void ResourceManager::removeRunway(int id) {
    pthread_mutex_lock(&mutex_runway);
    if (id >= 0 && id < (int)runways.size() && runways[id].available) {
        runways[id].available = false;
        runways[id].unavailableUntil = time(nullptr) + 86400; // Mark as unavailable for 24 hours
        numRunways--;
    }
    pthread_mutex_unlock(&mutex_runway);
}

void ResourceManager::removeGate(int id) {
    pthread_mutex_lock(&mutex_gate);
    if (id >= 0 && id < (int)gates.size() && gates[id].available) {
        gates[id].available = false;
        gates[id].unavailableUntil = time(nullptr) + 86400;
        numGates--;
    }
    pthread_mutex_unlock(&mutex_gate);
}

void ResourceManager::removeFuelTruck(int id) {
    pthread_mutex_lock(&mutex_fuelTruck);
    if (id >= 0 && id < (int)fuelTrucks.size() && fuelTrucks[id].available) {
        fuelTrucks[id].available = false;
        fuelTrucks[id].unavailableUntil = time(nullptr) + 86400;
        numFuelTrucks--;
    }
    pthread_mutex_unlock(&mutex_fuelTruck);
}

void ResourceManager::removeBaggageCrew(int id) {
    pthread_mutex_lock(&mutex_baggageCrew);
    if (id >= 0 && id < (int)baggageCrews.size() && baggageCrews[id].available) {
        baggageCrews[id].available = false;
        baggageCrews[id].unavailableUntil = time(nullptr) + 86400;
        numBaggageCrews--;
    }
    pthread_mutex_unlock(&mutex_baggageCrew);
}

void ResourceManager::removeMaintenanceTeam(int id) {
    pthread_mutex_lock(&mutex_maintenanceTeam);
    if (id >= 0 && id < (int)maintenanceTeams.size() && maintenanceTeams[id].available) {
        maintenanceTeams[id].available = false;
        maintenanceTeams[id].unavailableUntil = time(nullptr) + 86400;
        numMaintenanceTeams--;
    }
    pthread_mutex_unlock(&mutex_maintenanceTeam);
}

void ResourceManager::simulateBreakdown(ResourceType type, int resourceId, int durationSeconds) {
    time_t now = time(nullptr);
    
    switch (type) {
        case ResourceType::RUNWAY:
            pthread_mutex_lock(&mutex_runway);
            if (resourceId >= 0 && resourceId < (int)runways.size()) {
                markResourceUnavailable(&runways[resourceId], durationSeconds);
            }
            pthread_mutex_unlock(&mutex_runway);
            break;
        case ResourceType::GATE:
            pthread_mutex_lock(&mutex_gate);
            if (resourceId >= 0 && resourceId < (int)gates.size()) {
                markResourceUnavailable(&gates[resourceId], durationSeconds);
            }
            pthread_mutex_unlock(&mutex_gate);
            break;
        case ResourceType::FUEL_TRUCK:
            pthread_mutex_lock(&mutex_fuelTruck);
            if (resourceId >= 0 && resourceId < (int)fuelTrucks.size()) {
                markResourceUnavailable(&fuelTrucks[resourceId], durationSeconds);
            }
            pthread_mutex_unlock(&mutex_fuelTruck);
            break;
        default:
            break;
    }
}

int ResourceManager::getAvailableRunways() const {
    pthread_mutex_lock(&mutex_runway);
    int count = 0;
    time_t now = time(nullptr);
    for (const auto& runway : runways) {
        if (runway.available && runway.unavailableUntil <= now) count++;
    }
    pthread_mutex_unlock(&mutex_runway);
    return count;
}

int ResourceManager::getAvailableGates() const {
    pthread_mutex_lock(&mutex_gate);
    int count = 0;
    time_t now = time(nullptr);
    for (const auto& gate : gates) {
        if (gate.available && gate.unavailableUntil <= now) count++;
    }
    pthread_mutex_unlock(&mutex_gate);
    return count;
}

int ResourceManager::getAvailableFuelTrucks() const {
    pthread_mutex_lock(&mutex_fuelTruck);
    int count = 0;
    time_t now = time(nullptr);
    for (const auto& truck : fuelTrucks) {
        if (truck.available && truck.unavailableUntil <= now) count++;
    }
    pthread_mutex_unlock(&mutex_fuelTruck);
    return count;
}

int ResourceManager::getAvailableBaggageCrews() const {
    pthread_mutex_lock(&mutex_baggageCrew);
    int count = 0;
    time_t now = time(nullptr);
    for (const auto& crew : baggageCrews) {
        if (crew.available && crew.unavailableUntil <= now) count++;
    }
    pthread_mutex_unlock(&mutex_baggageCrew);
    return count;
}

int ResourceManager::getAvailableMaintenanceTeams() const {
    pthread_mutex_lock(&mutex_maintenanceTeam);
    int count = 0;
    time_t now = time(nullptr);
    for (const auto& team : maintenanceTeams) {
        if (team.available && team.unavailableUntil <= now) count++;
    }
    pthread_mutex_unlock(&mutex_maintenanceTeam);
    return count;
}

int ResourceManager::getTotalRunways() const {
    pthread_mutex_lock(&mutex_runway);
    int total = runways.size();
    pthread_mutex_unlock(&mutex_runway);
    return total;
}

int ResourceManager::getTotalGates() const {
    pthread_mutex_lock(&mutex_gate);
    int total = gates.size();
    pthread_mutex_unlock(&mutex_gate);
    return total;
}

int ResourceManager::getTotalFuelTrucks() const {
    pthread_mutex_lock(&mutex_fuelTruck);
    int total = fuelTrucks.size();
    pthread_mutex_unlock(&mutex_fuelTruck);
    return total;
}

int ResourceManager::getTotalBaggageCrews() const {
    pthread_mutex_lock(&mutex_baggageCrew);
    int total = baggageCrews.size();
    pthread_mutex_unlock(&mutex_baggageCrew);
    return total;
}

int ResourceManager::getTotalMaintenanceTeams() const {
    pthread_mutex_lock(&mutex_maintenanceTeam);
    int total = maintenanceTeams.size();
    pthread_mutex_unlock(&mutex_maintenanceTeam);
    return total;
}

std::vector<ResourceStatus> ResourceManager::getRunwayStatus() const {
    pthread_mutex_lock(&mutex_runway);
    std::vector<ResourceStatus> result = runways;
    pthread_mutex_unlock(&mutex_runway);
    return result;
}

std::vector<ResourceStatus> ResourceManager::getGateStatus() const {
    pthread_mutex_lock(&mutex_gate);
    std::vector<ResourceStatus> result = gates;
    pthread_mutex_unlock(&mutex_gate);
    return result;
}

std::vector<ResourceStatus> ResourceManager::getFuelTruckStatus() const {
    pthread_mutex_lock(&mutex_fuelTruck);
    std::vector<ResourceStatus> result = fuelTrucks;
    pthread_mutex_unlock(&mutex_fuelTruck);
    return result;
}

std::vector<ResourceStatus> ResourceManager::getBaggageCrewStatus() const {
    pthread_mutex_lock(&mutex_baggageCrew);
    std::vector<ResourceStatus> result = baggageCrews;
    pthread_mutex_unlock(&mutex_baggageCrew);
    return result;
}

std::vector<ResourceStatus> ResourceManager::getMaintenanceTeamStatus() const {
    pthread_mutex_lock(&mutex_maintenanceTeam);
    std::vector<ResourceStatus> result = maintenanceTeams;
    pthread_mutex_unlock(&mutex_maintenanceTeam);
    return result;
}

