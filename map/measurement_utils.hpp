#pragma once

#include "../std/string.hpp"

namespace MeasurementUtils
{

inline double MetersToMiles(double m) { return m * 0.000621371192; }
inline double MilesToMeters(double mi) { return mi * 1609.344; }
inline double MetersToYards(double m) { return m * 1.0936133; }
inline double YardsToMeters(double yd) { return yd * 0.9144; }
inline double MetersToFeet(double m) { return m * 3.2808399; }
inline double FeetToMeters(double ft) {  return ft * 0.3048; }

/// Takes into an account user settings [metric, imperial]
/// @param[in] m meters
/// @param[out] drawDir should be direction arrow drawed? false if distance is < 1.0
/// @return formatted string for search
string FormatDistance(double m, bool & drawDir);

}
