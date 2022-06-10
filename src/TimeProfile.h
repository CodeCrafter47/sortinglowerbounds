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


#ifndef SORTINGLOWERBOUNDS_TIMEPROFILE_H
#define SORTINGLOWERBOUNDS_TIMEPROFILE_H

#include <chrono>
#include <array>
#include <string>


enum Section: int {
    FW_PHASE1, FW_PHASE2, FW_IO, FW_OLDGEN, BW_WORK, BW_IO, OTHER, END
};

class TimeProfile {

private:
    std::chrono::steady_clock::time_point begin;
    Section current;
    std::array<std::chrono::steady_clock::duration, Section::END> profile;

public:
    explicit TimeProfile(Section section);

    void section(Section section);

    std::string summary();

    std::string totalTime();

private:

    static std::string formatDuration(std::chrono::steady_clock::duration duration);
};


#endif //SORTINGLOWERBOUNDS_TIMEPROFILE_H