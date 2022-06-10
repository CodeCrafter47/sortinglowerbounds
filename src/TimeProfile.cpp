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


#include "TimeProfile.h"

TimeProfile::TimeProfile(Section section) :
        begin(std::chrono::steady_clock::now()),
        current(section),
        profile() {
    profile.fill(std::chrono::steady_clock::duration::zero());
}

void TimeProfile::section(Section section) {
    auto now = std::chrono::steady_clock::now();
    auto duration = now - begin;
    if (current < Section::END) {
        profile[current] += duration;
    }
    begin = now;
    current = section;
}

std::string TimeProfile::summary() {
    auto now = std::chrono::steady_clock::now();
    // add time since begin
    auto profile = this->profile;
    auto duration = now - begin;
    if (current < Section::END) {
        profile[current] += duration;
    }
    // compute total
    auto total = profile[0] + profile[1] + profile[2] + profile[3] + profile[4] + profile[5] + profile[6];
    return "Total " + formatDuration(total)
           + ", bw work " + formatDuration(profile[Section::BW_WORK])
           + ", bw io " + formatDuration(profile[Section::BW_IO])
           + ", fw 1 " + formatDuration(profile[Section::FW_PHASE1])
           + ", fw 2 " + formatDuration(profile[Section::FW_PHASE2])
           + ", fw io " + formatDuration(profile[Section::FW_IO])
           + ", fw old " + formatDuration(profile[Section::FW_OLDGEN])
           + ", other " + formatDuration(profile[Section::OTHER]);
}

std::string TimeProfile::totalTime() {
    auto now = std::chrono::steady_clock::now();
    // add time since begin
    auto profile = this->profile;
    auto duration = now - begin;
    if (current < Section::END) {
        profile[current] += duration;
    }
    // compute total
    auto total = profile[0] + profile[1] + profile[2] + profile[3] + profile[4] + profile[5] + profile[6];
    return formatDuration(total);
}

std::string TimeProfile::formatDuration(std::chrono::steady_clock::duration duration) {
    long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    long seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    long minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    long hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    if (milliseconds < 5000) {
        return std::to_string(milliseconds) + " ms";
    } else if (seconds < 300) {
        return std::to_string(seconds) + " sec";
    } else if (minutes < 60) {
        return std::to_string(minutes) + " min";
    } else {
        return std::to_string(hours) + " hr " + std::to_string(minutes - hours * 60) + " min";
    }
}
