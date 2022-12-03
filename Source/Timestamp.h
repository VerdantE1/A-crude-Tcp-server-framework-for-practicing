#pragma once

#include <iostream>
#include <string>
using namespace std;

class Timestamp
{
public:
    static const int kMicroSecondsPerSecond = 1000 * 1000;
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    string toString() const;
    string toFormattedString(bool showMicroseconds = true);
private:
    int64_t microSecondsSinceEpoch_;
};