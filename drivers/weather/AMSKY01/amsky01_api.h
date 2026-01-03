/*
    AMSKY01 API INDI Weather Station Driver
    HTTP API implementation for AMSKY01 weather station
    
    Connects to amsky01_viewer.py HTTP API endpoint
    URL: http://localhost:8080/data.json
    
    Author: Roman Dvořák <info@astrometers.cz>
    Copyright (C) 2026 Astrometers
*/

#pragma once

#include <libindi/indiweather.h>
#include <string>

class AMSKY01_API : public INDI::Weather
{
public:
    AMSKY01_API();
    virtual ~AMSKY01_API();
    
    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual const char *getDefaultName() override;
    
    virtual IPState updateWeather() override;
    
protected:
    virtual void TimerHit() override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;

private:
    // HTTP Connection
    bool Connect() override;
    bool Disconnect() override;
    
    // API URL configuration
    ITextVectorProperty ApiUrlTP;
    IText ApiUrlT[1];
    
    // Status property
    ITextVectorProperty StatusTP;
    IText StatusT[2];
    
    // Data reading
    bool readHTTPData();
    bool parseJSONData(const std::string& jsonData);
    
    // Weather data structure
    struct {
        // Hygro sensor
        double temperature = 0.0;
        double humidity = 0.0;
        double dewPoint = 0.0;
        
        // Light sensor  
        double lux = 0.0;
        double skyBrightness = 0.0;
        
        // Cloud sensor
        double cloudTemp[5] = {0};
        double avgCloudTemp = 0.0;
        
        bool dataValid = false;
    } weatherData;
    
    std::string apiUrl = "http://localhost:8080/data.json";
};
