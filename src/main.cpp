#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>

static void menu() {
    std::cout << "\n=== System Resource Monitor Menu ===\n";
    std::cout << "1. Start Monitor\n";
    std::cout << "2. Run Scheduler (FCFS demo)\n";
    std::cout << "3. Generate Report\n";
    std::cout << "4. Cleanup Logs\n";
    std::cout << "5. Exit\n> ";
}

int main(){
    while (true) {
        menu();
        int ch; if(!(std::cin>>ch)) break;
        if (ch==1) {
            std::cout << "Starting monitor... (Ctrl+C in its terminal to stop)\n";
            std::system("./bin/monitor &> /dev/null &");
        } else if (ch==2) {
            std::cout << "Running scheduler demo...\n";
            std::system("./bin/scheduler");
        } else if (ch==3) {
            std::cout << "Generating report...\n";
            std::system("bash scripts/generate_report.sh");
        } else if (ch==4) {
            std::cout << "Cleanup logs...\n";
            std::system("bash scripts/cleanup_logs.sh 5");
        } else if (ch==5) {
            break;
        }
    }
    return 0;
}
