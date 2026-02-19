#ifndef UI_H
#define UI_H

#include "Airport.h"
#include <ncurses.h>
#include <pthread.h>
#include <string>
#include <vector>

class UI {
private:
    Airport* airport;
    WINDOW* mainWindow;
    WINDOW* statusWindow;
    WINDOW* resourceWindow;
    WINDOW* flightWindow;
    WINDOW* logWindow;
    
    pthread_t uiThread;
    bool uiRunning;
    bool shouldTerminate;
    
    std::vector<std::string> logMessages;
    pthread_mutex_t mutex_log;
    
    static void* uiUpdateLoop(void* arg);
    void updateDisplay();
    void drawHeader();
    void drawResourceStatus();
    void drawFlightStatus();
    void drawLog();
    void addLogMessage(const std::string& message);
    
    std::string getStatusString(FlightStatus status);
    std::string getPriorityString(FlightPriority priority);
    std::string getTypeString(FlightType type);
    std::string getColorCode(int priority);
    
public:
    UI(Airport* apt);
    ~UI();
    
    void start();
    void stop();
    void join();
    
    void log(const std::string& message);
};

#endif // UI_H

