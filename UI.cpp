#include "UI.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <unistd.h>

UI::UI(Airport* apt) : airport(apt), mainWindow(nullptr), statusWindow(nullptr), 
      resourceWindow(nullptr), flightWindow(nullptr), logWindow(nullptr),
      uiRunning(false), shouldTerminate(false) {
    pthread_mutex_init(&mutex_log, nullptr);
}

UI::~UI() {
    stop();
    join();
    
    // Don't delete mainWindow as it's set to stdscr
    if (statusWindow) {
        delwin(statusWindow);
    }
    if (resourceWindow) {
        delwin(resourceWindow);
    }
    if (flightWindow) {
        delwin(flightWindow);
    }
    if (logWindow) {
        delwin(logWindow);
    }
    
    endwin();
    pthread_mutex_destroy(&mutex_log);
}

void UI::start() {
    if (uiRunning) return;
    
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();
    
    // Initialize color pairs
    init_pair(1, COLOR_RED, COLOR_BLACK);      // High priority/Emergency
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);   // Medium priority
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Low priority
    init_pair(4, COLOR_CYAN, COLOR_BLACK);    // Info
    init_pair(5, COLOR_WHITE, COLOR_BLUE);    // Header
    
    // Create windows
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    
    mainWindow = stdscr;
    statusWindow = newwin(3, maxX, 0, 0);
    resourceWindow = newwin(8, maxX / 2, 3, 0);
    int flightHeight = std::max(10, (maxY - 12) / 2);
    flightWindow = newwin(flightHeight, maxX / 2, 3, maxX / 2);
    int logHeight = std::max(5, maxY - 12 - flightHeight);
    logWindow = newwin(logHeight, maxX / 2, 3 + flightHeight, maxX / 2);
    
    uiRunning = true;
    shouldTerminate = false;
    pthread_create(&uiThread, nullptr, uiUpdateLoop, this);
}

void UI::stop() {
    shouldTerminate = true;
}

void UI::join() {
    if (uiRunning) {
        pthread_join(uiThread, nullptr);
        uiRunning = false;
    }
}

void* UI::uiUpdateLoop(void* arg) {
    UI* ui = static_cast<UI*>(arg);
    
    while (!ui->shouldTerminate) {
        ui->updateDisplay();
        usleep(500000); // Update every 0.5 seconds
    }
    
    return nullptr;
}

void UI::updateDisplay() {
    drawHeader();
    drawResourceStatus();
    drawFlightStatus();
    drawLog();
    refresh();
}

void UI::drawHeader() {
    wattron(statusWindow, COLOR_PAIR(5));
    wbkgd(statusWindow, COLOR_PAIR(5));
    werase(statusWindow);
    
    std::ostringstream oss;
    oss << "  SMART AIRPORT GROUND OPERATIONS MANAGEMENT SYSTEM  ";
    oss << "  Time: " << std::setw(6) << airport->getSimulationTime() << "s  ";
    oss << "Active: " << std::setw(3) << airport->getActiveFlights() << "  ";
    oss << "Total: " << std::setw(3) << airport->getTotalFlights();
    
    mvwprintw(statusWindow, 1, 0, "%s", oss.str().c_str());
    wattroff(statusWindow, COLOR_PAIR(5));
    wrefresh(statusWindow);
}

void UI::drawResourceStatus() {
    werase(resourceWindow);
    box(resourceWindow, 0, 0);
    
    ResourceManager* rm = airport->getResourceManager();
    
    // Get totals and available counts
    int totalRunways = rm->getTotalRunways();
    int totalGates = rm->getTotalGates();
    int totalFuelTrucks = rm->getTotalFuelTrucks();
    int totalBaggageCrews = rm->getTotalBaggageCrews();
    int totalMaintenanceTeams = rm->getTotalMaintenanceTeams();
    
    int availRunways = rm->getAvailableRunways();
    int availGates = rm->getAvailableGates();
    int availFuelTrucks = rm->getAvailableFuelTrucks();
    int availBaggageCrews = rm->getAvailableBaggageCrews();
    int availMaintenanceTeams = rm->getAvailableMaintenanceTeams();
    
    // Calculate in-use counts
    int inUseRunways = totalRunways - availRunways;
    int inUseGates = totalGates - availGates;
    int inUseFuelTrucks = totalFuelTrucks - availFuelTrucks;
    int inUseBaggageCrews = totalBaggageCrews - availBaggageCrews;
    int inUseMaintenanceTeams = totalMaintenanceTeams - availMaintenanceTeams;
    
    mvwprintw(resourceWindow, 1, 2, "RESOURCE STATUS");
    mvwprintw(resourceWindow, 2, 2, "Runways:        %d/%d", 
              inUseRunways, totalRunways);
    mvwprintw(resourceWindow, 3, 2, "Gates:          %d/%d", 
              inUseGates, totalGates);
    mvwprintw(resourceWindow, 4, 2, "Fuel Trucks:    %d/%d", 
              inUseFuelTrucks, totalFuelTrucks);
    mvwprintw(resourceWindow, 5, 2, "Baggage Crews:  %d/%d", 
              inUseBaggageCrews, totalBaggageCrews);
    mvwprintw(resourceWindow, 6, 2, "Maintenance:    %d/%d", 
              inUseMaintenanceTeams, totalMaintenanceTeams);
    
    wrefresh(resourceWindow);
}

void UI::drawFlightStatus() {
    werase(flightWindow);
    box(flightWindow, 0, 0);
    
    mvwprintw(flightWindow, 1, 2, "FLIGHT STATUS");
    
    std::vector<FlightInfo> flights = airport->getAllFlightStatuses();
    
    int row = 2;
    int maxRows = getmaxy(flightWindow) - 2;
    
    for (size_t i = 0; i < flights.size() && row < maxRows; i++) {
        const FlightInfo& info = flights[i];
        
        // Determine color based on priority
        int color = 3; // Default green
        if (info.emergency || info.priority == FlightPriority::HIGH) {
            color = 1; // Red
        } else if (info.priority == FlightPriority::MEDIUM) {
            color = 2; // Yellow
        }
        
        wattron(flightWindow, COLOR_PAIR(color));
        
        std::ostringstream oss;
        oss << info.flightNumber;
        if (info.emergency) {
            oss << " [EMERGENCY]";
        }
        oss << " " << getStatusString(info.status);
        
        std::string line = oss.str();
        if (line.length() > getmaxx(flightWindow) - 4) {
            line = line.substr(0, getmaxx(flightWindow) - 4);
        }
        
        mvwprintw(flightWindow, row, 2, "%s", line.c_str());
        wattroff(flightWindow, COLOR_PAIR(color));
        
        row++;
        
        // Show activity on next line if space
        if (row < maxRows && !info.currentActivity.empty()) {
            wattron(flightWindow, COLOR_PAIR(4));
            std::string activity = "  -> " + info.currentActivity;
            if (activity.length() > getmaxx(flightWindow) - 4) {
                activity = activity.substr(0, getmaxx(flightWindow) - 4);
            }
            mvwprintw(flightWindow, row, 2, "%s", activity.c_str());
            wattroff(flightWindow, COLOR_PAIR(4));
            row++;
        }
    }
    
    wrefresh(flightWindow);
}

void UI::drawLog() {
    werase(logWindow);
    box(logWindow, 0, 0);
    
    mvwprintw(logWindow, 1, 2, "SYSTEM LOG");
    
    pthread_mutex_lock(&mutex_log);
    std::vector<std::string> logs = logMessages;
    pthread_mutex_unlock(&mutex_log);
    
    int maxRows = getmaxy(logWindow) - 2;
    int startIdx = (logs.size() > maxRows) ? logs.size() - maxRows : 0;
    
    int row = 2;
    for (int i = startIdx; i < (int)logs.size() && row < maxRows + 2; i++) {
        std::string log = logs[i];
        if (log.length() > getmaxx(logWindow) - 4) {
            log = log.substr(0, getmaxx(logWindow) - 4);
        }
        mvwprintw(logWindow, row, 2, "%s", log.c_str());
        row++;
    }
    
    wrefresh(logWindow);
}

void UI::addLogMessage(const std::string& message) {
    pthread_mutex_lock(&mutex_log);
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
    
    std::ostringstream oss;
    oss << "[" << timeStr << "] " << message;
    logMessages.push_back(oss.str());
    
    // Keep only last 100 messages
    if (logMessages.size() > 100) {
        logMessages.erase(logMessages.begin());
    }
    
    pthread_mutex_unlock(&mutex_log);
}

void UI::log(const std::string& message) {
    addLogMessage(message);
}

std::string UI::getStatusString(FlightStatus status) {
    switch (status) {
        case FlightStatus::INCOMING: return "INCOMING";
        case FlightStatus::REQUESTING_LANDING: return "REQ_LAND";
        case FlightStatus::LANDING: return "LANDING";
        case FlightStatus::PARKING: return "PARKING";
        case FlightStatus::REFUELING: return "REFUELING";
        case FlightStatus::MAINTENANCE: return "MAINTENANCE";
        case FlightStatus::BOARDING: return "BOARDING";
        case FlightStatus::BAGGAGE_HANDLING: return "BAGGAGE";
        case FlightStatus::READY_FOR_DEPARTURE: return "READY";
        case FlightStatus::DEPARTING: return "DEPARTING";
        case FlightStatus::DEPARTED: return "DEPARTED";
        default: return "UNKNOWN";
    }
}

std::string UI::getPriorityString(FlightPriority priority) {
    switch (priority) {
        case FlightPriority::HIGH: return "HIGH";
        case FlightPriority::MEDIUM: return "MEDIUM";
        case FlightPriority::LOW: return "LOW";
        default: return "UNKNOWN";
    }
}

std::string UI::getTypeString(FlightType type) {
    switch (type) {
        case FlightType::DOMESTIC: return "DOMESTIC";
        case FlightType::INTERNATIONAL: return "INTERNATIONAL";
        case FlightType::CARGO: return "CARGO";
        case FlightType::EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

std::string UI::getColorCode(int priority) {
    switch (priority) {
        case 3: return "\033[31m"; // Red
        case 2: return "\033[33m"; // Yellow
        case 1: return "\033[32m"; // Green
        default: return "\033[0m";  // Reset
    }
}

