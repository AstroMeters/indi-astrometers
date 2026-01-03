/*
    AMSKY01 API INDI Driver for AstroMeters Sky Sensor

    HTTP API implementation that connects to amsky01_viewer.py
    
    Author: Roman Dvorak <info@astrometers.cz>
    Copyright (C) 2026 Astrometers
*/

#include "amsky01_api.h"
#include "indicom.h"

#include <curl/curl.h>
#include <json/json.h>
#include <memory>
#include <cmath>

static std::unique_ptr<AMSKY01_API> amsky01_api(new AMSKY01_API());

// Callback for libcurl to write data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

AMSKY01_API::AMSKY01_API()
{
    setWeatherConnection(CONNECTION_NONE);
    setVersion(1, 0);
}

AMSKY01_API::~AMSKY01_API()
{
}

const char *AMSKY01_API::getDefaultName()
{
    return "AMSKY01 API";
}

bool AMSKY01_API::initProperties()
{
    INDI::Weather::initProperties();
    
    // 2. ODSTRANĚNÍ CONNECTION TABU
    addParameter("WEATHER_TEMPERATURE", "Temperature (°C)", -50, 80, 15);
    addParameter("WEATHER_HUMIDITY", "Humidity (%)", 0, 100, 15);  
    addParameter("WEATHER_DEW_POINT", "Dew Point (°C)", -50, 50, 15);
    addParameter("WEATHER_LIGHT_LUX", "Light (lux)", 0, 100000, 15);
    addParameter("WEATHER_SKY_BRIGHTNESS", "Sky Brightness (mag/arcsec²)", 10, 25, 15);
    
    // Individual sky temperatures
    addParameter("WEATHER_SKY_TEMP_CENTER", "Sky Temp Center (°C)", -50, 50, 15);
    
    setCriticalParameter("WEATHER_TEMPERATURE");
    setCriticalParameter("WEATHER_HUMIDITY");
    setCriticalParameter("WEATHER_DEW_POINT");
    setCriticalParameter("WEATHER_SKY_TEMP_CENTER");

    // API URL configuration
    IUFillText(&ApiUrlT[0], "API_URL", "API URL", apiUrl.c_str());
    IUFillTextVector(&ApiUrlTP, ApiUrlT, 1, getDeviceName(), "API_CONFIG", "API Configuration", 
                     OPTIONS_TAB, IP_RW, 60, IPS_IDLE);

    // Status display
    IUFillText(&StatusT[0], "DEVICE", "Device", "AMSKY01 API");
    IUFillText(&StatusT[1], "STATUS", "Status", "Disconnected");
    IUFillTextVector(&StatusTP, StatusT, 2, getDeviceName(), "DEVICE_STATUS", "Device Status", 
                     MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    addDebugControl();
    addConfigurationControl();
    addAuxControls();

    return true;
}

bool AMSKY01_API::updateProperties()
{
    INDI::Weather::updateProperties();

    if (isConnected())
    {
        defineProperty(&StatusTP);
        defineProperty(&ApiUrlTP);
        
        IUSaveText(&StatusT[1], "Connected - Reading API");
        StatusTP.s = IPS_OK;
        IDSetText(&StatusTP, nullptr);
        
        LOG_INFO("Device connected - starting API polling");
        
        // Poll every 2 seconds
        SetTimer(2000);
    }
    else
    {
        deleteProperty(StatusTP.name);
        deleteProperty(ApiUrlTP.name);
        
        LOG_INFO("Device disconnected");
    }

    return true;
}

bool AMSKY01_API::Connect()
{
    LOG_INFO("Attempting to connect to API...");
    
    // Try to read data to verify connection
    if (readHTTPData())
    {
        LOG_INFO("Successfully connected to API");
        return true;
    }
    else
    {
        LOGF_ERROR("Failed to connect to API at %s", apiUrl.c_str());
        return false;
    }
}

bool AMSKY01_API::Disconnect()
{
    LOG_INFO("Disconnected from API");
    return true;
}

void AMSKY01_API::TimerHit()
{
    if (!isConnected())
    {
        SetTimer(getCurrentPollingPeriod());
        return;
    }
    
    readHTTPData();
    SetTimer(getCurrentPollingPeriod());
}

bool AMSKY01_API::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, ApiUrlTP.name) == 0)
        {
            IUUpdateText(&ApiUrlTP, texts, names, n);
            apiUrl = ApiUrlT[0].text;
            ApiUrlTP.s = IPS_OK;
            IDSetText(&ApiUrlTP, nullptr);
            LOGF_INFO("API URL set to: %s", apiUrl.c_str());
            return true;
        }
    }

    return INDI::Weather::ISNewText(dev, name, texts, names, n);
}

bool AMSKY01_API::readHTTPData()
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (!curl)
    {
        LOG_ERROR("Failed to initialize CURL");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK)
    {
        LOGF_ERROR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 200)
    {
        LOGF_ERROR("HTTP request failed with code: %ld", http_code);
        return false;
    }

    LOGF_DEBUG("Received JSON data: %s", readBuffer.c_str());
    
    return parseJSONData(readBuffer);
}

bool AMSKY01_API::parseJSONData(const std::string& jsonData)
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;

    std::istringstream s(jsonData);
    if (!Json::parseFromStream(builder, s, &root, &errs))
    {
        LOGF_ERROR("Failed to parse JSON: %s", errs.c_str());
        return false;
    }

    try
    {
        // Parse hygro data
        if (root.isMember("hygro") && root["hygro"].isObject())
        {
            const Json::Value& hygro = root["hygro"];
            if (!hygro["temp"].isNull())
                weatherData.temperature = hygro["temp"].asDouble();
            if (!hygro["rh"].isNull())
                weatherData.humidity = hygro["rh"].asDouble();
            if (!hygro["dew_point"].isNull())
                weatherData.dewPoint = hygro["dew_point"].asDouble();
                
            setParameterValue("WEATHER_TEMPERATURE", weatherData.temperature);
            setParameterValue("WEATHER_HUMIDITY", weatherData.humidity);
            setParameterValue("WEATHER_DEW_POINT", weatherData.dewPoint);
        }

        // Parse light data
        if (root.isMember("light") && root["light"].isObject())
        {
            const Json::Value& light = root["light"];
            if (!light["lux"].isNull())
                weatherData.lux = light["lux"].asDouble();
            if (!light["sqm"].isNull())
                weatherData.skyBrightness = light["sqm"].asDouble();

            setParameterValue("WEATHER_LIGHT_LUX", weatherData.lux);
            setParameterValue("WEATHER_SKY_BRIGHTNESS", weatherData.skyBrightness);
        }

        // Parse cloud data
        if (root.isMember("cloud") && root["cloud"].isObject())
        {
            const Json::Value& cloud = root["cloud"];
            
            if (!cloud["center"].isNull())
            {
                weatherData.cloudTemp[4] = cloud["center"].asDouble();
                setParameterValue("WEATHER_SKY_TEMP_CENTER", weatherData.cloudTemp[4]);
            }
        }

        weatherData.dataValid = true;
        
        LOGF_DEBUG("Parsed data - Temp: %.2f°C, Humidity: %.2f%%, Lux: %.2f, SQM: %.2f",
                   weatherData.temperature, weatherData.humidity, weatherData.lux, weatherData.skyBrightness);
        
        return true;
    }
    catch (const std::exception& e)
    {
        LOGF_ERROR("Exception while parsing JSON: %s", e.what());
        return false;
    }
}

IPState AMSKY01_API::updateWeather()
{
    if (!weatherData.dataValid)
    {
        LOG_WARN("No valid weather data available");
        return IPS_ALERT;
    }

    return IPS_OK;
}
