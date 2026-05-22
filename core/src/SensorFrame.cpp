#include "nmea/SensorFrame.hpp"

namespace nmea {

std::string SensorFrame::nameOf(SensorId id)
{
    switch (id) {
        case SensorId::EngineRpm:       return "Engine RPM";
        case SensorId::CoolantTemp:     return "Coolant Temperature";
        case SensorId::FuelPressure:    return "Fuel Pressure";
        case SensorId::HullVibration:   return "Hull Vibration";
        case SensorId::WindSpeed:       return "Wind Speed";
        case SensorId::WindDirection:   return "Wind Direction";
        case SensorId::WaterDepth:      return "Water Depth";
        case SensorId::PropellerTorque: return "Propeller Torque";
        default:                        return "Unknown";
    }
}

} // namespace nmea