// MIT License
//
// Copyright (c) 2022 Florian Stober and Armin Weiß
// Institute for Formal Methods of Computer Science, University of Stuttgart
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "eventLog.h"
#include <string>
#include <ctime>
#include <mutex>

std::ofstream *EventLog::log = nullptr;
std::ofstream *EventLog::eventLog = nullptr;
std::vector<std::string> EventLog::history{};

bool EventLog::writeCout = false;

namespace {

    std::mutex lock;

    // source: https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c
    // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
    std::string currentDateTime() {
        time_t now = time(nullptr);
        char buf[80];
        struct tm tstruct = *localtime(&now);
        // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
        // for more information about date/time format
        strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

        return buf;
    }
}


void EventLog::init(std::ofstream *plog, std::ofstream *peventLog) {
    std::lock_guard lockGuard{lock};
    EventLog::log = plog;
    EventLog::eventLog = peventLog;
}

void EventLog::write(bool event, const std::string& message) {
    std::lock_guard lockGuard{lock};
    std::string msgWithTime = currentDateTime() + " " + message;
    if (EventLog::eventLog != nullptr) {
        *EventLog::eventLog << msgWithTime << std::endl;
    }
    if (EventLog::log != nullptr && !event) {
        *EventLog::log << msgWithTime << std::endl;
    }
    if (EventLog::writeCout) {
        std::cout << msgWithTime << std::endl;
    }
    history.emplace_back(message);
}

std::vector<std::string> EventLog::getHistory(size_t limit) {
    std::lock_guard lockGuard{lock};

    int begin = 0;
    if (history.size() > limit) {
        begin = static_cast<int>(history.size() - limit);
    }
    std::vector<std::string> result{history.cbegin() + begin, history.cend()};

    return result;
}
