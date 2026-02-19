#include "Airport.h"
#include "UI.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <random>
#include <sstream>
#include <signal.h>

Airport* g_airport = nullptr;
UI* g_ui = nullptr;

void signalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\nShutting down airport simulation...\n";
        if (g_ui) {
            g_ui->stop();
            g_ui->join();
        }
        if (g_airport) {
            g_airport->stopEventSimulation();
        }
        exit(0);
    }
}

int main() {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Seed random number generator
    std::srand(std::time(nullptr));
    
    std::cout << "Initializing Smart Airport Ground Operations Management System...\n";
    std::cout << "Press Ctrl+C to exit\n\n";
    
    // Create airport with initial resources
    g_airport = new Airport(2, 6, 3, 4, 3);
    
    // Create UI
    g_ui = new UI(g_airport);
    g_ui->start();
    
    // Start event simulation
    g_airport->startEventSimulation();
    
    g_ui->log("Airport system initialized");
    g_ui->log("Resources: 2 Runways, 6 Gates, 3 Fuel Trucks, 4 Baggage Crews, 3 Maintenance Teams");
    
    // Generate initial flights
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> typeDis(0, 3);
    std::uniform_int_distribution<> priorityDis(1, 3);
    
    // Add some initial flights
    for (int i = 0; i < 5; i++) {
        FlightType type = static_cast<FlightType>(typeDis(gen));
        FlightPriority priority = static_cast<FlightPriority>(priorityDis(gen));
        
        int flightId = g_airport->addFlight("", type, priority);
        g_airport->startFlight(flightId);
        
        std::ostringstream oss;
        oss << "Flight " << flightId << " added (Type: " << (int)type << ", Priority: " << (int)priority << ")";
        g_ui->log(oss.str());
        
        usleep(500000); // Small delay between flights
    }
    
    // Continuously add new flights
    int flightCounter = 5;
    while (true) {
        sleep(8); // Add new flight every 8 seconds
        
        FlightType type = static_cast<FlightType>(typeDis(gen));
        FlightPriority priority = static_cast<FlightPriority>(priorityDis(gen));
        
        int flightId = g_airport->addFlight("", type, priority);
        g_airport->startFlight(flightId);
        
        std::ostringstream oss;
        oss << "New flight " << flightId << " arrived";
        g_ui->log(oss.str());
        
        flightCounter++;
        
        // Clean up departed flights periodically
        if (flightCounter % 10 == 0) {
            // This would be handled by the airport's flight management
        }
    }
    
    // Cleanup (should not reach here due to signal handler)
    g_ui->stop();
    g_ui->join();
    g_airport->stopEventSimulation();
    
    delete g_ui;
    delete g_airport;
    
    return 0;
}

