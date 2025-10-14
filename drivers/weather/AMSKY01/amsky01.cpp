/*
    AMSKY01 INDI Driver for AstroMeters Sky Sensor

    This is an INDI (Instrument Neutral Distributed Interface) driver for the AMSKY01 sky sensor,
    designed to provide real-time weather and sky condition data for astronomical observatories.

    The AMSKY01 sensor measures temperature, humidity, dew point, ambient light, sky brightness,
    and cloud cover using multiple thermopile segments. Data is received via serial connection
    in CSV format with $light, $cloud, and $hygro messages.

    Author: Roman Dvorak <info@astrometers.cz>
    Copyright (C) 2025 Astrometers
*/

#include "amsky01.h"
#include "indicom.h"
#include "libindi/connectionplugins/connectionserial.h"
#include <termios.h>

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

static std::unique_ptr<AMSKY01> amsky01(new AMSKY01());

AMSKY01::AMSKY01()
{
    setVersion(1, 0);
}

AMSKY01::~AMSKY01()
{
}

const char *AMSKY01::getDefaultName()
{
    return "AMSKY01";
}

bool AMSKY01::initProperties()
{
    INDI::Weather::initProperties();

    // Add weather parameters podle AMSKY01 senzorů
    addParameter("WEATHER_TEMPERATURE", "Temperature (°C)", -50, 80, 15);
    addParameter("WEATHER_HUMIDITY", "Humidity (%)", 0, 100, 15);  
    addParameter("WEATHER_DEW_POINT", "Dew Point (°C)", -50, 50, 15);
    addParameter("WEATHER_LIGHT_LUX", "Light (lux)", 0, 100000, 15);
    addParameter("WEATHER_SKY_BRIGHTNESS", "Sky Brightness (mag/arcsec²)", 10, 25, 15);
    addParameter("WEATHER_CLOUD_COVER", "Cloud Cover (%)", 0, 100, 15);
    addParameter("WEATHER_SKY_TEMPERATURE", "Sky Temperature Avg (ADU)", 60000, 70000, 15);
    
    // Individuální teploty ze sky senzoru (5 thermopile segmentů)
    addParameter("WEATHER_SKY_TEMP_1", "Sky Temp 1 (ADU)", 60000, 70000, 15);
    addParameter("WEATHER_SKY_TEMP_2", "Sky Temp 2 (ADU)", 60000, 70000, 15);
    addParameter("WEATHER_SKY_TEMP_3", "Sky Temp 3 (ADU)", 60000, 70000, 15);
    addParameter("WEATHER_SKY_TEMP_4", "Sky Temp 4 (ADU)", 60000, 70000, 15);
    addParameter("WEATHER_SKY_TEMP_5", "Sky Temp 5 - Zenith (ADU)", 60000, 70000, 15);
    
    setCriticalParameter("WEATHER_TEMPERATURE");
    setCriticalParameter("WEATHER_HUMIDITY");
    setCriticalParameter("WEATHER_DEW_POINT");
    setCriticalParameter("WEATHER_LIGHT_LUX");
    setCriticalParameter("WEATHER_SKY_BRIGHTNESS");
    setCriticalParameter("WEATHER_CLOUD_COVER");
    setCriticalParameter("WEATHER_SKY_TEMPERATURE");

    // Device info
    addDebugControl();
    addSimulationControl();
    addConfigurationControl();
    
    // Status display
    IUFillText(&StatusT[0], "DEVICE", "Device", "AMSKY01");
    IUFillText(&StatusT[1], "STATUS", "Status", "Disconnected");
    IUFillTextVector(&StatusTP, StatusT, 2, getDeviceName(), "DEVICE_STATUS", "Device Status", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    // Add standard controls
    addAuxControls();

    return true;
}

bool AMSKY01::updateProperties()
{
    INDI::Weather::updateProperties();

    if (isConnected())
    {
        // Add properties when connected
        defineProperty(&StatusTP);
        
        // Update status and start automatic data reading
        IUSaveText(&StatusT[1], "Connected - Auto Reading");
        StatusTP.s = IPS_OK;
        IDSetText(&StatusTP, nullptr);
        
        printf("[AMSKY01] Device connected - starting automatic data reading\n");
        std::cout.flush();
        
        // Start continuous data reading
        SetTimer(100); // Read every 100ms
    }
    else
    {
        // Remove properties when disconnected
        deleteProperty(StatusTP.name);
        
        printf("[AMSKY01] Device disconnected\n");
        std::cout.flush();
    }

    return true;
}

bool AMSKY01::Handshake()
{
    if (isSimulation())
    {
        LOGF_INFO("Connected successfully to simulated %s.", getDeviceName());
        return true;
    }

    // Weather base class handles connection management
    // Just confirm connection is ready
    LOGF_INFO("Connected successfully to %s.", getDeviceName());
    
    return true;
}

bool AMSKY01::sendCommand(const char *cmd)
{
    int nbytes_read = 0, nbytes_written = 0, tty_rc = 0;
    char res[256] = {0};
    LOGF_DEBUG("CMD <%s>", cmd);

    if (!isSimulation())
    {
        tcflush(PortFD, TCIOFLUSH);
        if ((tty_rc = tty_write_string(PortFD, cmd, &nbytes_written)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial write error: %s", errorMessage);
            return false;
        }
    }

    if (isSimulation())
    {
        strncpy(res, "OK#", 8);
        nbytes_read = 3;
    }
    else
    {
        if ((tty_rc = tty_read_section(PortFD, res, '#', 1, &nbytes_read)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial read error: %s", errorMessage);
            // Don't print error for timeout - normal for continuous reading
            return false;
        }
    }

    res[nbytes_read - 1] = '\0';
    LOGF_DEBUG("RES <%s>", res);

    return true;
}

bool AMSKY01::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    return INDI::Weather::ISNewSwitch(dev, name, states, names, n);
}

void AMSKY01::TimerHit()
{
    if (isConnected())
    {
        readSerialData();
        SetTimer(100); // Continue reading every 100ms
    }
}

bool AMSKY01::readSerialData()
{
    // Use PortFD from base Weather class
    if (PortFD < 0)
    {
        return false;
    }

    char buffer[1024] = {0};
    int nbytes_read = 0;
    
    if (isSimulation())
    {
        // Generate realistic AMSKY01 test data
        static int counter = 0;
        counter++;
        
        switch (counter % 3)
        {
            case 0:
                // Hygro: temperature, humidity
                snprintf(buffer, sizeof(buffer), "$hygro,%.2f,%.2f", 
                        25.0 + (counter % 20), 45.0 + (counter % 30));
                break;
            case 1:
                // Light: lux, raw1, raw2, gain, integration_time
                snprintf(buffer, sizeof(buffer), "$light,%.2f,%d,%d,%d,%d", 
                        1500.0 + (counter % 1000), 4500 + (counter % 500), 
                        2100 + (counter % 200), 1, 300);
                break;
            case 2:
                // Cloud: 5 sky temperatures (ADC values)
                snprintf(buffer, sizeof(buffer), "$cloud,%.2f,%.2f,%.2f,%.2f,%.2f",
                        65100.0 + (counter % 50), 65140.0 + (counter % 40), 
                        65050.0 + (counter % 30), 65070.0 + (counter % 45),
                        65100.0 + (counter % 35));
                break;
        }
        
        nbytes_read = strlen(buffer);
        processData(std::string(buffer));
        return true;
    }

    LOGF_DEBUG("Reading serial data from FD %d", PortFD);
    
    // Try to read a line of data (ending with newline)
    int tty_rc = tty_read_section(PortFD, buffer, '\n', 3, &nbytes_read);
    
    if (tty_rc == TTY_OK && nbytes_read > 0)
    {
        buffer[nbytes_read - 1] = '\0'; // Remove newline
        std::string dataLine(buffer);
        processData(dataLine);
        return true;
    }
    else if (tty_rc == TTY_TIME_OUT)
    {
        // Timeout is normal when no data is available
        return true;
    }
    else if (tty_rc != TTY_OK)
    {
        char errorMessage[MAXRBUF];
        tty_error_msg(tty_rc, errorMessage, MAXRBUF);
        LOGF_DEBUG("Serial read error: %s", errorMessage);
        return false;
    }
    
    return true;
}

void AMSKY01::processData(const std::string& data)
{
    if (data.empty())
        return;
        
    // Ignoruj řádky nezačínající $
    if (data[0] != '$')
        return;
        
    
    // Parse weather data
    if (parseHygro(data) || parseLight(data) || parseCloud(data))
    {
        weatherData.dataValid = (weatherData.hygroValid || weatherData.lightValid || weatherData.cloudValid);
        
        // Update weather parameters
        if (weatherData.hygroValid)
        {
            setParameterValue("WEATHER_TEMPERATURE", weatherData.temperature);
            setParameterValue("WEATHER_HUMIDITY", weatherData.humidity);
            setParameterValue("WEATHER_DEW_POINT", weatherData.dewPoint);
        }
        
        if (weatherData.lightValid)
        {
            setParameterValue("WEATHER_LIGHT_LUX", weatherData.lux);
            setParameterValue("WEATHER_SKY_BRIGHTNESS", weatherData.skyBrightness);
        }
        
        if (weatherData.cloudValid)
        {
            setParameterValue("WEATHER_SKY_TEMPERATURE", weatherData.avgCloudTemp);
            setParameterValue("WEATHER_CLOUD_COVER", weatherData.cloudCover);
            
            // Aktualizuj všechny individuální sky teploty
            setParameterValue("WEATHER_SKY_TEMP_1", weatherData.cloudTemp[0]);
            setParameterValue("WEATHER_SKY_TEMP_2", weatherData.cloudTemp[1]);
            setParameterValue("WEATHER_SKY_TEMP_3", weatherData.cloudTemp[2]);
            setParameterValue("WEATHER_SKY_TEMP_4", weatherData.cloudTemp[3]);
            setParameterValue("WEATHER_SKY_TEMP_5", weatherData.cloudTemp[4]);
        }
    }
    
    // Log received data only in debug mode
    if (isDebug())
        LOGF_DEBUG("Received data: %s", data.c_str());
}

// Weather-specific functions
IPState AMSKY01::updateWeather()
{
    if (weatherData.dataValid)
        return IPS_OK;
    else
        return IPS_BUSY;
}

bool AMSKY01::parseHygro(const std::string& data)
{
    // Parse: $hygro,temperature,humidity
    if (data.find("$hygro,") == 0)
    {
        std::vector<std::string> tokens;
        std::stringstream ss(data);
        std::string token;
        
        while (std::getline(ss, token, ','))
        {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 3)
        {
            try
            {
                weatherData.temperature = std::stod(tokens[1]);
                weatherData.humidity = std::stod(tokens[2]);
                
                // Calculate dew point using Magnus formula
                double a = 17.27;
                double b = 237.7;
                double alpha = ((a * weatherData.temperature) / (b + weatherData.temperature)) + log(weatherData.humidity / 100.0);
                weatherData.dewPoint = (b * alpha) / (a - alpha);
                
                weatherData.hygroValid = true;
                return true;
            }
            catch (const std::exception& e)
            {
                LOGF_ERROR("Error parsing hygro data: %s", e.what());
            }
        }
    }
    return false;
}

bool AMSKY01::parseLight(const std::string& data)
{
    // Parse: $light,lux,raw1,raw2,gain,integration_time_ms
    if (data.find("$light,") == 0)
    {
        std::vector<std::string> tokens;
        std::stringstream ss(data);
        std::string token;
        
        while (std::getline(ss, token, ','))
        {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 6)
        {
            try
            {
                weatherData.lux = std::stod(tokens[1]);
                weatherData.raw1 = std::stoi(tokens[2]);
                weatherData.raw2 = std::stoi(tokens[3]);
                weatherData.gain = std::stoi(tokens[4]);
                weatherData.integrationTime = std::stoi(tokens[5]);
                
                // Convert lux to sky brightness (lepší aproximace)
                // Velmi tmavá obloha: ~22 mag/arcsec² při <0.01 lux
                // Jasná obloha při úplňku: ~19 mag/arcsec² při ~0.1 lux  
                // Městské světlo: ~16-18 mag/arcsec² při >10 lux

                weatherData.lux = (static_cast<float>(weatherData.raw1) / static_cast<float>(weatherData.gain)) / static_cast<float>(weatherData.integrationTime);
                weatherData.lux *= 1000000.0;

                if (weatherData.lux < 0.001)
                    weatherData.skyBrightness = 22.0;
                else
                    weatherData.skyBrightness = 22.0 - 2.5 * log10(weatherData.lux * 100);
                
                // Omez na rozumné hodnoty
                if (weatherData.skyBrightness < 15.0) weatherData.skyBrightness = 15.0;
                if (weatherData.skyBrightness > 22.5) weatherData.skyBrightness = 22.5;
                
                weatherData.lightValid = true;
                return true;
            }
            catch (const std::exception& e)
            {
                LOGF_ERROR("Error parsing light data: %s", e.what());
            }
        }
    }
    return false;
}

bool AMSKY01::parseCloud(const std::string& data)
{
    // Parse: $cloud,temp1,temp2,temp3,temp4,temp5 (4 segmenty + zenit)
    if (data.find("$cloud,") == 0)
    {
        std::vector<std::string> tokens;
        std::stringstream ss(data);
        std::string token;
        
        while (std::getline(ss, token, ','))
        {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 6)
        {
            try
            {
                // Načti 5 teplot oblohy
                double tempSum = 0.0;
                for (int i = 0; i < 5; i++)
                {
                    weatherData.cloudTemp[i] = std::stod(tokens[i + 1]);
                    tempSum += weatherData.cloudTemp[i];
                }
                
                weatherData.avgCloudTemp = tempSum / 5.0;
                
                double minSkyTemp = 64000.0;  // jasná studená obloha
                double maxSkyTemp = 66000.0;  // velmi oblačno
                
                weatherData.cloudCover = ((weatherData.avgCloudTemp - minSkyTemp) / (maxSkyTemp - minSkyTemp)) * 100.0;
                if (weatherData.cloudCover < 0.0) weatherData.cloudCover = 0.0;
                if (weatherData.cloudCover > 100.0) weatherData.cloudCover = 100.0;
                
                weatherData.cloudValid = true;
                return true;
            }
            catch (const std::exception& e)
            {
                LOGF_ERROR("Error parsing cloud data: %s", e.what());
            }
        }
    }
    return false;
}
