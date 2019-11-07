/*
Project: Trans Tasman Racing Setup Assistant
File: ttr_setupasst.cpp
Author: Brock Salmon
Notice: (C) Copyright 2019 by Brock Salmon and Trans Tasman Racing. All Rights Reserved.
*/

// TODO v1.2
// TODO(bSalmon): Show Comparison with Best Time on closest weather
//   TODO(bSalmon): Register Weather, Lap Time, Driver, and Car
//   TODO(bSalmon): Upload File to the TTR FTP
//   TODO(bSalmon): Download .ttrsa files and show closest weather's best lap
// NOTE(bSalmon): .ttrsa file structure (20 bytes)
// NOTE(bSalmon): Driver: TTR_Drivers enum (4 bytes)
// NOTE(bSalmon): Car: TTR_Cars enum (4 bytes)
// NOTE(bSalmon): Time: f32 (4 bytes)
// NOTE(bSalmon): Weather: WeatherInfo struct (8 bytes)
//   NOTE(bSalmon): Ambient Temp: u8 (1 byte)
//   NOTE(bSalmon): Track Temp: u8 (1 byte)
//   NOTE(bSalmon): Wind Dir: u8 (1 byte)
//   NOTE(bSalmon): Wind Speed: u8 (1 byte)
//   NOTE(bSalmon): Time of Day: f32 (4 bytes)
// TODO(bSalmon): General efficiency updates

// TODO Long-Term
// TODO(bSalmon): Change how telemetry is parsed based on car
// TODO(bSalmon): Maybe make a Windows Application?

// Static Definitions
#define internal_func static
#define local_persist static
#define global_var static

// Typedefs
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int32_t b32;

typedef float f32;
typedef double f64;

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <Windows.h>
#include <regex>
#include <strsafe.h>

#include "irsdk_defines.h"
#include "ttr_verify.h"

#define FL 0
#define FR 1
#define RL 2
#define RR 3

#define SET_TEXT_RED(h) SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_INTENSITY)
#define SET_TEXT_WHITE(h) SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define SET_TEXT_YELLOW(h) SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define SET_TEXT_AQUA(h) SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define SET_TEXT_GREEN(h) SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define SET_TEXT_PURPLE(h) SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define SET_TEXT_BLUE(h) SetConsoleTextAttribute(h, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#define SET_TEXT_BRIGHT_WHITE(h) SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#define COLOUR_TEXT_ENDLINE(h, c, s) SET_TEXT_##c##(h); \
std::cout << s << std::endl; \
SET_TEXT_WHITE(h)
#define COLOUR_TEXT(h, c, s) SET_TEXT_##c##(h); \
std::cout << s; \
SET_TEXT_WHITE(h)

struct SetupInfo
{
    s32 lsCom;
    s32 hsCom;
    s32 lsReb;
    s32 hsReb;
};

struct DataSetInfo
{
    s32 lapIndex;
    
    s32 tyreIndices[4];
    s32 shockIndices[4];
};

struct IBTInfo
{
    std::vector<s32> laps;
    f32 tyrePressures[4];
    std::vector<f32> shockVels[4];
    
    SetupInfo setupInfo[4];
};

struct OutputInfo
{
    std::vector<f32> shockVelValues;
    f32 loCom;
    f32 loReb;
    f32 hiCom;
    f32 hiReb;
    f32 aveCom;
    f32 aveReb;
    
    f32 loDiff;
    f32 hiDiff;
    
    s32 loAdjust;
    s32 hiAdjust;
    
    f32 tyrePresDiff;
};

internal_func void RegexMatchSetup(SetupInfo *setupInfo, std::string line, HANDLE consoleHandle)
{
    std::regex damperRegex("^   (.s)(Comp|Rbd)(Damping: )([0-9]|1[0-6])( clicks)");
    
    if (std::regex_match(line, damperRegex))
    {
        std::stringstream stream;
        std::string word;
        stream << line;
        s32 index = -1;
        while (!stream.eof())
        {
            s32 checker = 0;
            stream >> word;
            
            if (word == "LsCompDamping:")
            {
                index = 0;
            }
            else if (word == "HsCompDamping:")
            {
                index = 1;
            }
            else if (word == "LsRbdDamping:")
            {
                index = 2;
            }
            else if (word == "HsRbdDamping:")
            {
                index = 3;
            }
            
            if (index == 0)
            {
                if (std::stringstream(word) >> checker)
                {
                    setupInfo->lsCom = checker;
                }
            }
            else if (index == 1)
            {
                if (std::stringstream(word) >> checker)
                {
                    setupInfo->hsCom = checker;
                }
            }
            else if (index == 2)
            {
                if (std::stringstream(word) >> checker)
                {
                    setupInfo->lsReb = checker;
                }
            }
            else if (index == 3)
            {
                if (std::stringstream(word) >> checker)
                {
                    setupInfo->hsReb = checker;
                }
            }
        }
        
        COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, line);
    }
}

internal_func IBTInfo ReadIBT(FILE *file, HANDLE consoleHandle, b32 ttrDebug)
{
    DataSetInfo dataSetInfo = {};
    IBTInfo ibtInfo = {};
    
    irsdk_header header;
    irsdk_diskSubHeader diskSubHeader;
    irsdk_varHeader *varHeaders = 0;
    
    fread(&header, 1, sizeof(header), file);
    fread(&diskSubHeader, 1, sizeof(diskSubHeader), file);
    
    if (ttrDebug)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "--- Session Info ---");
    }
    
    char *sessionInfoString = new char[header.sessionInfoLen];
    if (sessionInfoString)
    {
        fseek(file, header.sessionInfoOffset, SEEK_SET);
        fread(sessionInfoString, sizeof(char), header.sessionInfoLen, file);
        sessionInfoString[header.sessionInfoLen - 1] = '\0';
        
        // Session String Processing Here
        // NOTE(bSalmon): This is where setup is held
        
        b32 flSection = false;
        b32 frSection = false;
        b32 rlSection = false;
        b32 rrSection = false;
        
        std::string tempString = "";
        for (s32 i = 0; i < header.sessionInfoLen; ++i)
        {
            char currChar = sessionInfoString[i];
            tempString += currChar;
            
            if (tempString == "  LeftFront:")
            {
                COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, tempString);
                flSection = true;
                frSection = false;
                rlSection = false;
                rrSection = false;
            }
            else if (tempString == "  RightFront:")
            {
                COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, tempString);
                flSection = false;
                frSection = true;
                rlSection = false;
                rrSection = false;
            }
            else if (tempString == "  LeftRear:")
            {
                COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, tempString);
                flSection = false;
                frSection = false;
                rlSection = true;
                rrSection = false;
            }
            else if (tempString == "  RightRear:")
            {
                COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, tempString);
                flSection = false;
                frSection = false;
                rlSection = false;
                rrSection = true;
            }
            
            if (flSection)
            {
                RegexMatchSetup(&ibtInfo.setupInfo[FL], tempString, consoleHandle);
            }
            else if (frSection)
            {
                RegexMatchSetup(&ibtInfo.setupInfo[FR], tempString, consoleHandle);
            }
            else if (rlSection)
            {
                RegexMatchSetup(&ibtInfo.setupInfo[RL], tempString, consoleHandle);
            }
            else if (rrSection)
            {
                RegexMatchSetup(&ibtInfo.setupInfo[RR], tempString, consoleHandle);
            }
            
            if (currChar == '\n')
            {
                tempString = "";
            }
        }
    }
    
    if (ttrDebug)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "\n--- Var Header ---");
    }
    else
    {
        printf("\n");
    }
    
    varHeaders = new irsdk_varHeader[header.numVars];
    if (varHeaders)
    {
        fseek(file, header.varHeaderOffset, SEEK_SET);
        fread(varHeaders, 1, header.numVars * sizeof(irsdk_varHeader), file);
        
        for (s32 i = 0; i < header.numVars; ++i)
        {
            irsdk_varHeader *currHeader = &varHeaders[i];
            
            if (currHeader)
            {
                std::string nameString(currHeader->name);
                if (nameString == "Lap" ||
                    nameString == "LFpressure" || nameString == "RFpressure" || nameString == "LRpressure" || nameString == "RRpressure" || nameString == "LFshockVel" || nameString == "RFshockVel" || nameString == "LRshockVel" || nameString == "RRshockVel")
                {
                    if (nameString == "Lap")
                    {
                        dataSetInfo.lapIndex = i;
                    }
                    else if (nameString == "LFpressure")
                    {
                        dataSetInfo.tyreIndices[FL] = i;
                    }
                    else if (nameString == "RFpressure")
                    {
                        dataSetInfo.tyreIndices[FR] = i;
                    }
                    else if (nameString == "LRpressure")
                    {
                        dataSetInfo.tyreIndices[RL] = i;
                    }
                    else if (nameString == "RRpressure")
                    {
                        dataSetInfo.tyreIndices[RR] = i;
                    }
                    else if (nameString == "LFshockVel")
                    {
                        dataSetInfo.shockIndices[FL] = i;
                    }
                    else if (nameString == "RFshockVel")
                    {
                        dataSetInfo.shockIndices[FR] = i;
                    }
                    else if (nameString == "LRshockVel")
                    {
                        dataSetInfo.shockIndices[RL] = i;
                    }
                    else if (nameString == "RRshockVel")
                    {
                        dataSetInfo.shockIndices[RR] = i;
                    }
                    
                    COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, nameString << " Index Set: " << i);
                }
            }
        }
    }
    
    if (ttrDebug)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "\n--- Data ---");
    }
    
    char *varBuffer = new char[header.bufLen];
    std::vector<std::string> dataLines;
    if (varBuffer)
    {
        fseek(file, header.varBuf[0].bufOffset, SEEK_SET);
        
        COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "\nRetrieving Data from IBT...");
        
        std::string tempString = "";
        while (fread(varBuffer, sizeof(char), header.bufLen, file))
        {
            // Data Line Processing
            for (s32 i = 0; i < header.numVars; ++i)
            {
                irsdk_varHeader *currHeader = &varHeaders[i];
                
                if (currHeader)
                {
                    if (i == dataSetInfo.lapIndex)
                    {
                        s32 lap = *((s32 *)(varBuffer + currHeader->offset));
                        ibtInfo.laps.push_back(lap);
                    }
                    else if (i == dataSetInfo.tyreIndices[FL])
                    {
                        f32 pressure = *((f32 *)(varBuffer + currHeader->offset));
                        if (pressure > ibtInfo.tyrePressures[FL])
                        {
                            ibtInfo.tyrePressures[FL] = pressure;
                        }
                    }
                    else if (i == dataSetInfo.tyreIndices[FR])
                    {
                        f32 pressure = *((f32 *)(varBuffer + currHeader->offset));
                        if (pressure > ibtInfo.tyrePressures[FR])
                        {
                            ibtInfo.tyrePressures[FR] = pressure;
                        }
                    }
                    else if (i == dataSetInfo.tyreIndices[RL])
                    {
                        f32 pressure = *((f32 *)(varBuffer + currHeader->offset));
                        if (pressure > ibtInfo.tyrePressures[RL])
                        {
                            ibtInfo.tyrePressures[RL] = pressure;
                        }
                    }
                    else if (i == dataSetInfo.tyreIndices[RR])
                    {
                        f32 pressure = *((f32 *)(varBuffer + currHeader->offset));
                        if (pressure > ibtInfo.tyrePressures[RR])
                        {
                            ibtInfo.tyrePressures[RR] = pressure;
                        }
                    }
                    else if (i == dataSetInfo.shockIndices[FL])
                    {
                        f32 shockVel = *((f32 *)(varBuffer + currHeader->offset));
                        shockVel *= 39.37f; // Convert from mm/s to in/s
                        ibtInfo.shockVels[FL].push_back(shockVel);
                    }
                    else if (i == dataSetInfo.shockIndices[FR])
                    {
                        f32 shockVel = *((f32 *)(varBuffer + currHeader->offset));
                        shockVel *= 39.37f; // Convert from mm/s to in/s
                        ibtInfo.shockVels[FR].push_back(shockVel);
                    }
                    else if (i == dataSetInfo.shockIndices[RL])
                    {
                        f32 shockVel = *((f32 *)(varBuffer + currHeader->offset));
                        shockVel *= 39.37f; // Convert from mm/s to in/s
                        ibtInfo.shockVels[RL].push_back(shockVel);
                    }
                    else if (i == dataSetInfo.shockIndices[RR])
                    {
                        f32 shockVel = *((f32 *)(varBuffer + currHeader->offset));
                        shockVel *= 39.37f; // Convert from mm/s to in/s
                        ibtInfo.shockVels[RR].push_back(shockVel);
                    }
                }
            }
        }
        
        COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "\nData Retrieved from IBT!");
    }
    
    if (ttrDebug)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "\n--- Clean Up ---");
    }
    
    // Clean Up
    if (sessionInfoString)
    {
        if (ttrDebug)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, "Session Data Cleansed");
        }
        delete[] sessionInfoString;
        sessionInfoString = 0;
    }
    
    if (varHeaders)
    {
        if (ttrDebug)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, "Var Header Cleansed");
        }
        delete[] varHeaders;
        varHeaders = 0;
    }
    
    if (varBuffer)
    {
        if (ttrDebug)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, "Telemetry Data Cleansed\n");
        }
        delete[] varBuffer;
        varBuffer = 0;
    }
    
    return ibtInfo;
}

s32 main (s32 argc, char *argv[])
{
    HANDLE consoleHandle;
    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    
    f32 version = 1.25f;
    
    if (!TTR_Verify(argv[argc - 1]))
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "User could not be verified.\n\n");
        return 1;
    }
    else
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "User Verified: " << argv[argc - 1]);
    }
    
    if (strcmp(argv[argc - 2], "update") == 0)
    {
        printf("Checking for Updates...\n");
        s32 updateCode = TTR_Update(&version);
        if (updateCode == 0)
        {
            // Up-To-Date
            COLOUR_TEXT_ENDLINE(consoleHandle, BLUE, "Software is already Up-To-Date\n");
            return 0;
        }
        else if (updateCode == 1)
        {
            // Updated
            SET_TEXT_GREEN(consoleHandle);
            printf("Successfully Updated to Version %.2f\n", version);
            printf("Restart Program now to use the updated version\n");
            SET_TEXT_WHITE(consoleHandle);
            
            STARTUPINFO si = {};
            PROCESS_INFORMATION pi = {};
            
            char exePath[MAX_PATH];
            char *exeName = 0;
            
            GetModuleFileNameA(0, exePath, MAX_PATH);
            
            for (char *scan = exePath; *scan; ++scan)
            {
                if (*scan == '\\')
                {
                    exeName = scan + 1;
                    
                }
            }
            
            s32 pathIndex = 0;
            while (exePath[pathIndex] != '\0')
            {
                pathIndex++;
            }
            
            s32 exeIndex = 0;
            while (exeName[exeIndex] != '\0')
            {
                exeIndex++;
            }
            
            exePath[pathIndex - exeIndex] = '\0';
            
            char moduleName[MAX_PATH];
            ConcatenateStrings(StringLength(exePath), exePath,
                               StringLength("old.exe"), "old.exe",
                               moduleName);
            
            char cmd[MAX_PATH];
            StringCbPrintf(cmd, MAX_PATH, 
                           TEXT("cmd.exe /C ping 1.1.1.1 -n 1 -w 3000 > Nul & Del /f /q \"%s\""),
                           moduleName);
            
            CreateProcess(0, cmd, 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &si, &pi);
            
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            
            return 0;
        }
        else if (updateCode == 2)
        {
            // Failed to Update
            COLOUR_TEXT_ENDLINE(consoleHandle, RED, "Update Failed\n");
            return 1;
        }
    }
    
    LARGE_INTEGER perfCounterFreqResult;
    QueryPerformanceFrequency(&perfCounterFreqResult);
    s64 perfCounterFreq = perfCounterFreqResult.QuadPart;
    
    LARGE_INTEGER beginTimer;
    QueryPerformanceCounter(&beginTimer);
    u64 beginCycleCount = __rdtsc();
    
    SET_TEXT_WHITE(consoleHandle);
    std::cout << std::endl;
    std::cout << "---------------------------------------------------------------------------" << std::endl;
    std::cout << " -dNNNNNNNNNNNNNNNNNNNNNN/`dNNNNNNNNNNNNNNNNNNNNN/";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, "`osssssssssssssssssssss+ ");
    std::cout << "-mNNNNNNNNNNNNNNNNNNNNNN+`dNNNNNNNNNNNNNNNNNNNNN/";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, "`oyyyyyyyyyyyyyyyyyyyyyyy+");
    std::cout << "       /mNNNNNNNNy:           yNNNNNNNNm+                       ";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, "+yyyyyyyys.");
    std::cout << "      `hNNNNNNNNy`           /NNNNNNNNm:                       ";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, ":yyyyyyyys. ");
    std::cout << "     `hNNNNNNNNy`           /NNNNNNNNm:       ";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, ".sssssssss/ .ssssyyyyyyyys.  ");
    std::cout << "    .dNNNNNNNNy`           +NNNNNNNNN:       ";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, ".syyyyyyyy+ .yyyyyyyyysso/`   ");
    std::cout << "   .dMMMMMMMMy`           +NMMMMMMMN:       ";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, ".shhhhhhhh+  `/yhhhhhhho.      ");
    std::cout << "  .dMMMMMMMMy`           +MMMMMMMMm:       ";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, ".yhhhhhhhy/     .shhhhhhhs-     ");
    std::cout << "                                                             ";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, "/hhhhhhhh+    ");
    std::cout << "  Setup Assistant v";
    printf("%.02f", version);
    std::cout << ", created by Brock Salmon              ";
    COLOUR_TEXT_ENDLINE(consoleHandle, RED, ".oyhhhhhhs.  ");
    std::cout << "---------------------------------------------------------------------------\n\n";
    
    COLOUR_TEXT_ENDLINE(consoleHandle, BLUE, "---------------------------------------------------------------------------");
    std::cout << "* Use of this software is limited to present members of Trans Tasman Racing. *" << std::endl;
    std::cout << "*   Distribution of this software to those outside of Trans Tasman Racing    *" << std::endl;
    std::cout << "*    without the express permission of ";
    COLOUR_TEXT(consoleHandle, BRIGHT_WHITE, "Brock Salmon (Software Developer)");
    std::cout << ",    *" << std::endl;
    std::cout << "*  or ";
    COLOUR_TEXT(consoleHandle, BRIGHT_WHITE, "Madison Down (Owner and Manager of Trans Tasman Racing)");
    std::cout << ", is strictly   *" << std::endl;
    std::cout << "*      forbidden, through using this software you agree to these terms.      *" << std::endl;
    COLOUR_TEXT_ENDLINE(consoleHandle, BLUE, "---------------------------------------------------------------------------\n\n");
    
    // Use argv to get IBT filename
    char *ibtFilename = 0;
    
    b32 ttrDebug = false;
    
    ibtFilename = argv[1];
    std::string debugStringCheck = argv[2];
    if (debugStringCheck == "-d")
    {
        ttrDebug = true;
    }
    
    // Open file
    FILE *file = 0;
    fopen_s(&file, ibtFilename, "rb");
    if (!file)
    {
        printf("Failed to open %s\n", ibtFilename);
        return 1;
    }
    
    IBTInfo ibtInfo = ReadIBT(file, consoleHandle, ttrDebug);
    fclose(file);
    
    // Use laps to filter out valid data
    s32 currMin = ibtInfo.laps[1];
    s32 currMax = INT_MAX;
    s32 minCutoff = 0;
    b32 minCutoffSet = false;
    s32 maxCutoff = INT_MAX;
    b32 maxCutoffSet = false;
    
    for (s32 i = 1; i < ibtInfo.laps.size(); ++i)
    {
        s32 currLapVal = ibtInfo.laps[i];
        if (currLapVal < currMin)
        {
            if (ibtInfo.laps[i + 1] != currLapVal)
            {
                // Do nothing as it's probably a telemetry glitch
            }
            else
            {
                currMin = currLapVal;
                minCutoffSet = false;
            }
        }
        
        if (currLapVal == (currMin + 1) && !minCutoffSet)
        {
            minCutoff = i;
            minCutoffSet = true;
            currMin = currLapVal;
            maxCutoff = i - 1;
            currMax = currLapVal;
        }
        
        if (minCutoffSet)
        {
            if (currLapVal == currMax + 2)
            {
                maxCutoff = i - 1;
                currMax = ibtInfo.laps[maxCutoff];
            }
        }
    }
    
    if (ttrDebug)
    {
        printf("\n");
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "--DEBUG INFO--");
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "currMin: " << currMin);
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "minCutoff: " << minCutoff);
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "currMax: " << currMax);
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "maxCutoff: " << maxCutoff);
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "--------------");
    }
    
    if (maxCutoff <= minCutoff)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "\nData Sample Size too small! (Too Few Laps?)");
        return 1;
    }
    
    printf("\nValidating Shock Data within correct Lap range...\n");
    
    b32 lapsInvalid = true;
    for (s32 i = 0; i < 4; ++i)
    {
        std::vector<s32> validatedLaps; 
        std::vector<f32> validatedShockData;
        for (s32 j = minCutoff; j <= maxCutoff; ++j)
        {
            validatedShockData.push_back(ibtInfo.shockVels[i][j]);
            
            if (lapsInvalid)
            {
                validatedLaps.push_back(ibtInfo.laps[j]);
            }
        }
        ibtInfo.shockVels[i] = validatedShockData;
        if (lapsInvalid)
        {
            ibtInfo.laps = validatedLaps;
            lapsInvalid = false;
        }
    }
    
    printf("\nShock Data Validated\n");
    
    printf("\nGetting Shock Data...\n");
    // Get shock values from array
    OutputInfo outputInfo[4] = {};
    for (s32 i = 0; i < 4; ++i)
    {
        if (i == FL)
        {
            printf("Getting FL Shock Data...\n");
        }
        else if (i == FR)
        {
            printf("Getting FR Shock Data...\n");
        }
        else if (i == RL)
        {
            printf("Getting RL Shock Data...\n");
        }
        else if (i == RR)
        {
            printf("Getting RR Shock Data...\n");
        }
        
        for (s32 j = 0; j < ibtInfo.laps.size(); ++j)
        {
            try
            {
                f32 fInchValue = ibtInfo.shockVels[i][j];
                if (fInchValue > -10.0f && fInchValue < 10.0f)
                {
                    outputInfo[i].shockVelValues.push_back(fInchValue);
                }
            }
            catch (std::invalid_argument e)
            {
                std::cerr << "Invalid Argument: " << e.what() << std::endl;
            }
        }
    }
    
    
    printf("\nProcessing Shock Data...\n");
    
    // Calculate hi lo%'s
    f32 hiLoBoundary = 1.00f;
    for (s32 i = 0; i < 4; ++i)
    {
        if (i == FL)
        {
            printf("Processing FL Shock Data...\n");
        }
        else if (i == FR)
        {
            printf("Processing FR Shock Data...\n");
        }
        else if (i == RL)
        {
            printf("Processing RL Shock Data...\n");
        }
        else if (i == RR)
        {
            printf("Processing RR Shock Data...\n");
        }
        
        // Get percent of one value of an array
        f32 oneValuePercent = 100.0f / (f32)outputInfo[i].shockVelValues.size();
        s32 rebCount = 0;
        s32 comCount = 0;
        f32 comTotal = 0.0f;
        f32 rebTotal = 0.0f;
        
        for (s32 j = 0; j < outputInfo[i].shockVelValues.size(); ++j)
        {
            f32 currVal = outputInfo[i].shockVelValues[j];
            if (currVal < -hiLoBoundary)
            {
                outputInfo[i].hiReb += oneValuePercent;
                rebTotal += currVal;
                rebCount++;
            }
            else if (currVal < 0.0f && currVal >= -hiLoBoundary)
            {
                outputInfo[i].loReb += oneValuePercent;
                rebTotal += currVal;
                rebCount++;
            }
            else if (currVal > hiLoBoundary)
            {
                outputInfo[i].hiCom += oneValuePercent;
                comTotal += currVal;
                comCount++;
            }
            else if (currVal > 0.0f && currVal <= hiLoBoundary)
            {
                outputInfo[i].loCom += oneValuePercent;
                comTotal += currVal;
                comCount++;
            }
        }
        
        outputInfo[i].aveCom = comTotal / comCount;
        outputInfo[i].aveReb = (rebTotal / rebCount) * -1; // To make it a positive value
    }
    
    // TODO(bSalmon): Is the differing %'s a bug or resolution issue?
    printf("\nFL Reb Ave: %.02f%%", outputInfo[FL].aveReb);
    printf("\t\tFR Reb Ave: %.02f%%\n", outputInfo[FR].aveReb);
    printf("FL Hi Reb: %.2f%%", outputInfo[FL].hiReb);
    printf("\t\tFR Hi Reb: %.2f%%\n", outputInfo[FR].hiReb);
    printf("FL Lo Reb: %.2f%%", outputInfo[FL].loReb);
    printf("\t\tFR Lo Reb: %.2f%%\n", outputInfo[FR].loReb);
    printf("FL Lo Com: %.2f%%", outputInfo[FL].loCom);
    printf("\t\tFR Lo Com: %.2f%%\n", outputInfo[FR].loCom);
    printf("FL Hi Com: %.2f%%", outputInfo[FL].hiCom);
    printf("\t\tFR Hi Com: %.2f%%\n", outputInfo[FR].hiCom);
    printf("FL Com Ave: %.02f%%", outputInfo[FL].aveCom);
    printf("\t\tFR Com Ave: %.02f%%\n", outputInfo[FR].aveCom);
    printf("FL Max Tyre Pres: %0.2fkPa", ibtInfo.tyrePressures[FL]);
    printf("\tFR Max Tyre Pres: %0.2fkPa\n\n", ibtInfo.tyrePressures[FR]);
    
    printf("RL Reb Ave: %.02f%%", outputInfo[RL].aveReb);
    printf("\t\tRR Reb Ave: %.02f%%\n", outputInfo[RR].aveReb);
    printf("RL Hi Reb: %.2f%%", outputInfo[RL].hiReb);
    printf("\t\tRR Hi Reb: %.2f%%\n", outputInfo[RR].hiReb);
    printf("RL Lo Reb: %.2f%%", outputInfo[RL].loReb);
    printf("\t\tRR Lo Reb: %.2f%%\n", outputInfo[RR].loReb);
    printf("RL Lo Com: %.2f%%", outputInfo[RL].loCom);
    printf("\t\tRR Lo Com: %.2f%%\n", outputInfo[RR].loCom);
    printf("RL Hi Com: %.2f%%", outputInfo[RL].hiCom);
    printf("\t\tRR Hi Com: %.2f%%\n", outputInfo[RR].hiCom);
    printf("RL Com Ave: %.02f%%", outputInfo[RL].aveCom);
    printf("\t\tRR Com Ave: %.02f%%\n", outputInfo[RR].aveCom);
    printf("RL Max Tyre Pres: %0.2fkPa", ibtInfo.tyrePressures[RL]);
    printf("\tRR Max Tyre Pres: %0.2fkPa\n\n", ibtInfo.tyrePressures[RR]);
    
    // Get difference between %'s for adjustment
    for (s32 i = 0; i < 4; ++i)
    {
        outputInfo[i].loDiff = outputInfo[i].loReb - outputInfo[i].loCom;
        outputInfo[i].hiDiff = outputInfo[i].hiReb - outputInfo[i].hiCom;
        
        outputInfo[i].loAdjust = (s32)roundf(outputInfo[i].loDiff);
        outputInfo[i].hiAdjust = (s32)roundf(outputInfo[i].hiDiff);
        
        outputInfo[i].tyrePresDiff = ibtInfo.tyrePressures[i] - 180.0f;
        
        // Shock Changes
        // Suggest adjustments based on %'s
        switch (i)
        {
            case FL:
            {
                printf("\nFL:\n");
                break;
            }
            case FR:
            {
                printf("\nFR:\n");
                break;
            }
            case RL:
            {
                printf("\nRL:\n");
                break;
            }
            case RR:
            {
                printf("\nRR:\n");
                break;
            }
            default:
            {
                break;
            }
        }
        
        SetupInfo adjustedSet[4] = {ibtInfo.setupInfo[0], ibtInfo.setupInfo[1], ibtInfo.setupInfo[2], ibtInfo.setupInfo[3]};
        
        if (outputInfo[i].loDiff < 0.0f)
        {
            // Increase Rebound
            if (outputInfo[i].loAdjust <= -1.0f)
            {
                if (adjustedSet[i].lsReb + (outputInfo[i].loAdjust * -1) <= 16)
                {
                    adjustedSet[i].lsReb += (outputInfo[i].loAdjust * -1);
                }
                else if (adjustedSet[i].lsReb + (outputInfo[i].loAdjust * -1) > 16)
                {
                    s32 overflow = (adjustedSet[i].lsReb + (outputInfo[i].loAdjust * -1)) - 16;
                    adjustedSet[i].lsReb = 16;
                    
                    if (adjustedSet[i].lsCom - overflow >= 0)
                    {
                        adjustedSet[i].lsCom -= overflow;
                    }
                    else
                    {
                        adjustedSet[i].lsCom = 0;
                    }
                }
            }
        }
        else if (outputInfo[i].loDiff > 0.0f)
        {
            // Increase Compression
            if (outputInfo[i].loAdjust >= 1.0f)
            {
                if (adjustedSet[i].lsCom + outputInfo[i].loAdjust <= 16)
                {
                    adjustedSet[i].lsCom += outputInfo[i].loAdjust;
                }
                else if (adjustedSet[i].lsCom + outputInfo[i].loAdjust > 16)
                {
                    s32 overflow = (adjustedSet[i].lsCom + outputInfo[i].loAdjust) - 16;
                    adjustedSet[i].lsCom = 16;
                    
                    if (adjustedSet[i].lsReb - overflow >= 0)
                    {
                        adjustedSet[i].lsReb -= overflow;
                    }
                    else
                    {
                        adjustedSet[i].lsReb = 0;
                    }
                }
            }
        }
        
        if (outputInfo[i].hiDiff < 0.0f)
        {
            // Decrease Compression
            if (outputInfo[i].hiAdjust <= -1.0f)
            {
                if (adjustedSet[i].hsCom - (outputInfo[i].hiAdjust * -1) >= 0)
                {
                    adjustedSet[i].hsCom -= (outputInfo[i].hiAdjust * -1);
                }
                else if (adjustedSet[i].hsCom - (outputInfo[i].hiAdjust * -1) < 0)
                {
                    s32 overflow = (adjustedSet[i].hsCom - (outputInfo[i].hiAdjust * -1)) * -1;
                    adjustedSet[i].hsCom = 0;
                    
                    if (adjustedSet[i].hsReb + overflow <= 12)
                    {
                        adjustedSet[i].hsReb += overflow;
                    }
                    else
                    {
                        adjustedSet[i].hsReb = 12;
                    }
                }
            }
        }
        else if (outputInfo[i].hiDiff > 0.0f)
        {
            // Decrease Rebound
            if (outputInfo[i].hiAdjust >= 1.0f)
            {
                if (adjustedSet[i].hsReb - outputInfo[i].hiAdjust >= 0)
                {
                    adjustedSet[i].hsReb -= outputInfo[i].hiAdjust;
                }
                else if (adjustedSet[i].hsReb - outputInfo[i].hiAdjust < 0)
                {
                    s32 overflow = (adjustedSet[i].hsReb - outputInfo[i].hiAdjust) * -1;
                    adjustedSet[i].hsReb = 0;
                    
                    if (adjustedSet[i].hsCom + overflow <= 12)
                    {
                        adjustedSet[i].hsCom += overflow;
                    }
                    else
                    {
                        adjustedSet[i].hsCom = 12;
                    }
                }
            }
        }
        
        printf("\tLow Speed Comp: ");
        if (adjustedSet[i].lsCom != ibtInfo.setupInfo[i].lsCom)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, adjustedSet[i].lsCom << " clicks");
        }
        else
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, adjustedSet[i].lsCom << " clicks");
        }
        printf("\tHigh Speed Comp: ");
        if (adjustedSet[i].hsCom != ibtInfo.setupInfo[i].hsCom)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, adjustedSet[i].hsCom << " clicks");
        }
        else
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, adjustedSet[i].hsCom << " clicks");
        }
        printf("\tLow Speed Reb: ");
        if (adjustedSet[i].lsReb != ibtInfo.setupInfo[i].lsReb)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, adjustedSet[i].lsReb << " clicks");
        }
        else
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, adjustedSet[i].lsReb << " clicks");
        }
        printf("\tHigh Speed Reb: ");
        if (adjustedSet[i].hsReb != ibtInfo.setupInfo[i].hsReb)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, adjustedSet[i].hsReb << " clicks");
        }
        else
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, adjustedSet[i].hsReb << " clicks");
        }
        
        // Use Tyre difference to suggest adjustments
        if (outputInfo[i].tyrePresDiff < 2.0f && outputInfo[i].tyrePresDiff > -2.0f)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "\tTyre Pressure Within Optimal Base Range");
        }
        else
        {
            if (outputInfo[i].tyrePresDiff > 2.0f)
            {
                // Over 182.0kPa
                printf("\tTyre Pressure: ");
                SET_TEXT_YELLOW(consoleHandle);
                printf("%0.2fkPa", outputInfo[i].tyrePresDiff);
                SET_TEXT_WHITE(consoleHandle);
                COLOUR_TEXT(consoleHandle, PURPLE, " over ");
                printf("optimal baseline of ");
                COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "180kPa");
            }
            else if (outputInfo[i].tyrePresDiff < -2.0f)
            {
                outputInfo[i].tyrePresDiff *= -1;
                // Under 178.0kPa
                printf("\tTyre Pressure: ");
                SET_TEXT_YELLOW(consoleHandle);
                printf("%0.2fkPa", outputInfo[i].tyrePresDiff);
                SET_TEXT_WHITE(consoleHandle);
                COLOUR_TEXT(consoleHandle, AQUA, " under ");
                printf("optimal baseline of ");
                COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "180kPa");
            }
        }
    }
    
    u64 endCycleCount = __rdtsc();
    u64 cyclesElapsed = endCycleCount - beginCycleCount;
    LARGE_INTEGER endTimer;
    QueryPerformanceCounter(&endTimer);
    f32 timeElapsed = ((f32)(endTimer.QuadPart - beginTimer.QuadPart) / (f32)perfCounterFreq);
    
    if (ttrDebug)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, PURPLE, "\nExecuted in " << timeElapsed << " seconds");
        COLOUR_TEXT_ENDLINE(consoleHandle, PURPLE, "Executed in " << cyclesElapsed << " cycles");
    }
    
    return 0;
}