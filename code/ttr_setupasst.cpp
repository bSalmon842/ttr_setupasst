/*
Project: Trans Tasman Racing Setup Assistant
File: ttr_setupasst.cpp
Author: Brock Salmon
Notice: (C) Copyright 2019 by Brock Salmon and Trans Tasman Racing. All Rights Reserved.
*/

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
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <iostream>
#include <Windows.h>
#include <stdexcept>

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

struct CSVInfo
{
    std::vector<std::vector<std::string>> data;
    s32 lapCol;
    s32 flShockCol;
    s32 frShockCol;
    s32 rlShockCol;
    s32 rrShockCol;
    s32 flTyreCol;
    s32 frTyreCol;
    s32 rlTyreCol;
    s32 rrTyreCol;
};

struct ShockInfo
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
    
    f32 loAdjust;
    f32 hiAdjust;
    
    f32 tyrePressure;
    f32 tyrePresDiff;
};

internal_func CSVInfo InitArray(std::ifstream &file)
{
    CSVInfo result = {};
    std::string line;
    std::string value;
    
    printf("Preparing to Parse File...\n");
    
    std::string calcLine;
    std::vector<std::vector<std::string>> percentCalc;
    while (std::getline(file, calcLine))
    {
        std::vector<std::string> row;
        percentCalc.push_back(row);
    }
    
    file.clear();
    file.seekg(0, std::ios_base::beg);
    
    f32 oneValuePercent = 100.0f / percentCalc.size();
    
    printf("Parsing File...\n");
    while (std::getline(file, line))
    {
        std::vector<std::string> row;
        std::stringstream ss(line);
        
        while (std::getline(ss, value, ','))
        {
            if (value == "Lap")
            {
                result.lapCol = (s32)row.size();
            } 
            else if (value == "LFshockVel")
            {
                result.flShockCol = (s32)row.size();
            }
            else if (value == "RFshockVel")
            {
                result.frShockCol = (s32)row.size();
            }
            else if (value == "LRshockVel")
            {
                result.rlShockCol = (s32)row.size();
            }
            else if (value == "RRshockVel")
            {
                result.rrShockCol = (s32)row.size();
            }
            else if (value == "LFpressure")
            {
                result.flTyreCol = (s32)row.size();
            }
            else if (value == "RFpressure")
            {
                result.frTyreCol = (s32)row.size();
            }
            else if (value == "LRpressure")
            {
                result.rlTyreCol = (s32)row.size();
            }
            else if (value == "RRpressure")
            {
                result.rrTyreCol = (s32)row.size();
            }
            
            row.push_back(value);
        }
        
        result.data.push_back(row);
        if (fmod((s32)result.data.size() * oneValuePercent, 5.0f) < oneValuePercent)
        {
            printf("File Parsed: %.0f%%...\n", (s32)result.data.size() * oneValuePercent);
        }
    }
    return result;
}

s32 main (s32 argc, char *argv[])
{
    HANDLE consoleHandle;
    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
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
    std::cout << "  Setup Assistant v1.0, created by Brock Salmon               ";
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
    
    // Use argv to get CSV filename
    char *csvFilename = 0;
    
    b32 ttrDebug = false;
    
    if (argc == 2 || argc == 3) 
    {
        csvFilename = argv[1];
        if (argc == 3)
        {
            std::string debugStringCheck = argv[2];
            if (debugStringCheck == "-d")
            {
                ttrDebug = true;
            }
        }
    }
    else if (argc < 2)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "FAILED: Missing CSV Filename");
        return 1;
    }
    else if (argc > 3)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "FAILED: Too many arguments, only CSV filepath is needed");
        return 1;
    }
    
    // Open file
    std::ifstream fileIn;
    fileIn.open(csvFilename);
    if (!fileIn.is_open())
    {
        printf("Failed to open %s\n", csvFilename);
        return 1;
    }
    
    CSVInfo csvInfo = InitArray(fileIn);
    fileIn.close();
    
    
    if (csvInfo.lapCol == 0 ||
        csvInfo.flShockCol == 0 || csvInfo.frShockCol == 0 ||
        csvInfo.rlShockCol == 0 || csvInfo.rrShockCol == 0 ||
        csvInfo.flTyreCol == 0 || csvInfo.frTyreCol == 0 ||
        csvInfo.rlTyreCol == 0 || csvInfo.rrTyreCol == 0)
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, RED, "\n!!! Not all Data Columns Found !!!\n");
    }
    else
    {
        COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "\nAll Data Columns Found.");
    }
    
    // Cut header and labels out of the array
    csvInfo.data.erase(csvInfo.data.begin(), csvInfo.data.begin() + 12);
    
    ShockInfo shocks[4];
    shocks[FL].tyrePressure = 0.0f;
    shocks[FR].tyrePressure = 0.0f;
    shocks[RL].tyrePressure = 0.0f;
    shocks[RR].tyrePressure = 0.0f;
    
    // Find highest Tyre Pressures
    printf("\nFinding Max Tyre Pressures...\n");
    for (s32 i = 0; i < 4; ++i)
    {
        for (s32 j = 0; j < csvInfo.data.size(); ++j)
        {
            switch (i)
            {
                case FL:
                {
                    if (std::stof(csvInfo.data[j][csvInfo.flTyreCol]) > shocks[FL].tyrePressure)
                    {
                        shocks[FL].tyrePressure = std::stof(csvInfo.data[j][csvInfo.flTyreCol]);
                    }
                    break;
                }
                case FR:
                {
                    if (std::stof(csvInfo.data[j][csvInfo.frTyreCol]) > shocks[FR].tyrePressure)
                    {
                        shocks[FR].tyrePressure = std::stof(csvInfo.data[j][csvInfo.frTyreCol]);
                    }
                    break;
                }
                case RL:
                {
                    if (std::stof(csvInfo.data[j][csvInfo.rlTyreCol]) > shocks[RL].tyrePressure)
                    {
                        shocks[RL].tyrePressure = std::stof(csvInfo.data[j][csvInfo.rlTyreCol]);
                    }
                    break;
                }
                case RR:
                {
                    if (std::stof(csvInfo.data[j][csvInfo.rrTyreCol]) > shocks[RR].tyrePressure)
                    {
                        shocks[RR].tyrePressure = std::stof(csvInfo.data[j][csvInfo.rrTyreCol]);
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
    
    // Use laps to filter out valid data
    s32 currMin = std::stoi(csvInfo.data[0][csvInfo.lapCol]);
    s32 currMax = INT_MAX;
    s32 minCutoff = 0;
    b32 minCutoffSet = false;
    s32 maxCutoff = INT_MAX;
    b32 maxCutoffSet = false;
    
    for (s32 i = 0; i < csvInfo.data.size(); ++i)
    {
        s32 currLapVal = std::stoi(csvInfo.data[i][csvInfo.lapCol]);
        if (currLapVal < currMin)
        {
            currMin = currLapVal;
            minCutoffSet = false;
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
                currMax = std::stoi(csvInfo.data[maxCutoff][csvInfo.lapCol]);
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
    
    printf("\nGetting Shock Data within correct Lap range...\n");
    std::vector<std::vector<std::string>> newCSVData;
    f32 oneCutoffPercent = 100.0f / (maxCutoff - minCutoff);
    for (s32 i = minCutoff; i <= maxCutoff; ++i)
    {
        newCSVData.push_back(csvInfo.data[i]);
        if (fmod((i - minCutoff) * oneCutoffPercent, 5.0f) < oneCutoffPercent)
        {
            printf("Data Validated: %.0f%%...\n", (i - minCutoff) * oneCutoffPercent);
        }
    }
    
    csvInfo.data = newCSVData;
    
    printf("\nGetting Shock Data...\n");
    // Get shock values from array
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
        
        shocks[i].loCom = 0.0f;
        shocks[i].loReb = 0.0f;
        shocks[i].hiCom = 0.0f;
        shocks[i].hiReb = 0.0f;
        
        for (s32 j = 0; j < csvInfo.data.size(); ++j)
        {
            switch (i)
            {
                case FL:
                {
                    try
                    {
                        f32 fInchValue = std::stof(csvInfo.data[j][csvInfo.flShockCol]) * 39.37f;
                        if (fInchValue > -10.0f && fInchValue < 10.0f)
                        {
                            shocks[FL].shockVelValues.push_back(fInchValue);
                        }
                    }
                    catch (std::invalid_argument e)
                    {
                        std::cerr << "Invalid Argument: " << e.what() << std::endl;
                    }
                    break;
                }
                
                case FR:
                {
                    try
                    {
                        f32 fInchValue = std::stof(csvInfo.data[j][csvInfo.frShockCol]) * 39.37f;
                        if (fInchValue > -10.0f && fInchValue < 10.0f)
                        {
                            shocks[FR].shockVelValues.push_back(fInchValue);
                        }
                    }
                    catch (std::invalid_argument e)
                    {
                        std::cerr << "Invalid Argument: " << e.what() << std::endl;
                    }
                    break;
                }
                
                case RL:
                {
                    try
                    {
                        f32 fInchValue = std::stof(csvInfo.data[j][csvInfo.rlShockCol]) * 39.37f;
                        if (fInchValue > -10.0f && fInchValue < 10.0f)
                        {
                            shocks[RL].shockVelValues.push_back(fInchValue);
                        }
                    }
                    catch (std::invalid_argument e)
                    {
                        std::cerr << "Invalid Argument: " << e.what() << std::endl;
                    }
                    break;
                }
                
                case RR:
                {
                    try
                    {
                        f32 fInchValue = std::stof(csvInfo.data[j][csvInfo.rrShockCol]) * 39.37f;
                        if (fInchValue > -10.0f && fInchValue < 10.0f)
                        {
                            shocks[RR].shockVelValues.push_back(fInchValue);
                        }
                    }
                    catch (std::invalid_argument e)
                    {
                        std::cerr << "Invalid Argument: " << e.what() << std::endl;
                    }
                    break;
                }
                
                default:
                {
                    // NOTE(bSalmon): If this ever gets hit then something has gone badly wrong
                    break;
                }
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
        f32 oneValuePercent = 100.0f / (f32)shocks[i].shockVelValues.size();
        s32 rebCount = 0;
        s32 comCount = 0;
        f32 comTotal = 0.0f;
        f32 rebTotal = 0.0f;
        
        for (s32 j = 0; j < shocks[i].shockVelValues.size(); ++j)
        {
            f32 currVal = shocks[i].shockVelValues[j];
            if (currVal < -hiLoBoundary)
            {
                shocks[i].hiReb += oneValuePercent;
                rebTotal += currVal;
                rebCount++;
            }
            else if (currVal < 0.0f && currVal >= -hiLoBoundary)
            {
                shocks[i].loReb += oneValuePercent;
                rebTotal += currVal;
                rebCount++;
            }
            else if (currVal > hiLoBoundary)
            {
                shocks[i].hiCom += oneValuePercent;
                comTotal += currVal;
                comCount++;
            }
            else if (currVal > 0.0f && currVal <= hiLoBoundary)
            {
                shocks[i].loCom += oneValuePercent;
                comTotal += currVal;
                comCount++;
            }
        }
        
        shocks[i].aveCom = comTotal / comCount;
        shocks[i].aveReb = (rebTotal / rebCount) * -1; // To make it a positive value
    }
    
    // TODO(bSalmon): Is the differing %'s a bug or resolution issue?
    printf("\nFL Reb Ave: %.02f%%", shocks[FL].aveReb);
    printf("\t\tFR Reb Ave: %.02f%%\n", shocks[FR].aveReb);
    printf("FL Hi Reb: %.2f%%", shocks[FL].hiReb);
    printf("\t\tFR Hi Reb: %.2f%%\n", shocks[FR].hiReb);
    printf("FL Lo Reb: %.2f%%", shocks[FL].loReb);
    printf("\t\tFR Lo Reb: %.2f%%\n", shocks[FR].loReb);
    printf("FL Lo Com: %.2f%%", shocks[FL].loCom);
    printf("\t\tFR Lo Com: %.2f%%\n", shocks[FR].loCom);
    printf("FL Hi Com: %.2f%%", shocks[FL].hiCom);
    printf("\t\tFR Hi Com: %.2f%%\n", shocks[FR].hiCom);
    printf("FL Com Ave: %.02f%%", shocks[FL].aveCom);
    printf("\t\tFR Com Ave: %.02f%%\n", shocks[FR].aveCom);
    printf("FL Max Tyre Pres: %0.2fkPa", shocks[FL].tyrePressure);
    printf("\tFR Max Tyre Pres: %0.2fkPa\n\n", shocks[FR].tyrePressure);
    
    printf("RL Reb Ave: %.02f%%", shocks[RL].aveReb);
    printf("\t\tRR Reb Ave: %.02f%%\n", shocks[RR].aveReb);
    printf("RL Hi Reb: %.2f%%", shocks[RL].hiReb);
    printf("\t\tRR Hi Reb: %.2f%%\n", shocks[RR].hiReb);
    printf("RL Lo Reb: %.2f%%", shocks[RL].loReb);
    printf("\t\tRR Lo Reb: %.2f%%\n", shocks[RR].loReb);
    printf("RL Lo Com: %.2f%%", shocks[RL].loCom);
    printf("\t\tRR Lo Com: %.2f%%\n", shocks[RR].loCom);
    printf("RL Hi Com: %.2f%%", shocks[RL].hiCom);
    printf("\t\tRR Hi Com: %.2f%%\n", shocks[RR].hiCom);
    printf("RL Com Ave: %.02f%%", shocks[RL].aveCom);
    printf("\t\tRR Com Ave: %.02f%%\n", shocks[RR].aveCom);
    printf("RL Max Tyre Pres: %0.2fkPa", shocks[RL].tyrePressure);
    printf("\tRR Max Tyre Pres: %0.2fkPa\n\n", shocks[RR].tyrePressure);
    
    // Get difference between %'s for adjustment
    for (s32 i = 0; i < 4; ++i)
    {
        shocks[i].loDiff = shocks[i].loReb - shocks[i].loCom;
        shocks[i].hiDiff = shocks[i].hiReb - shocks[i].hiCom;
        
        shocks[i].loAdjust = roundf(shocks[i].loDiff);
        shocks[i].hiAdjust = roundf(shocks[i].hiDiff);
        
        shocks[i].tyrePresDiff = shocks[i].tyrePressure - 180.0f;
        
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
        
        if (shocks[i].loDiff < 0.0f)
        {
            // Increase Rebound
            if (shocks[i].loAdjust <= -1.0f)
            {
                COLOUR_TEXT(consoleHandle, AQUA, "\tIncrease ");
                printf("LS Reb: ");
                COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, (shocks[i].loAdjust * -1) << " Clicks");
            }
        }
        else if (shocks[i].loDiff > 0.0f)
        {
            // Increase Compression
            if (shocks[i].loAdjust >= 1.0f)
            {
                COLOUR_TEXT(consoleHandle, AQUA, "\tIncrease ");
                printf("LS Comp: ");
                COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, shocks[i].loAdjust << " Clicks");
            }
        }
        else
        {
            // Perfect (Unlikely)
            COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "\tLow Speed Correct");
        }
        
        if (shocks[i].hiDiff < 0.0f)
        {
            // Decrease Compression
            if (shocks[i].hiAdjust <= -1.0f)
            {
                COLOUR_TEXT(consoleHandle, PURPLE, "\tDecrease ");
                printf("HS Comp: ");
                COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, (shocks[i].hiAdjust * -1) << " Clicks");
            }
        }
        else if (shocks[i].hiDiff > 0.0f)
        {
            // Decrease Rebound
            if (shocks[i].hiAdjust >= 1.0f)
            {
                COLOUR_TEXT(consoleHandle, PURPLE, "\tDecrease ");
                printf("HS Reb: ");
                COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, shocks[i].hiAdjust << " Clicks");
            }
        }
        else
        {
            // Perfect (Unlikely)
            COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "\tHigh Speed Correct");
        }
        
        // Use Tyre difference to suggest adjustments
        if (shocks[i].tyrePresDiff < 2.0f && shocks[i].tyrePresDiff > -2.0f)
        {
            COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "\tTyre Pressure Within Optimal Base Range");
        }
        else
        {
            if (shocks[i].tyrePresDiff > 2.0f)
            {
                // Over 182.0kPa
                printf("\tTyre Pressure: ");
                SET_TEXT_YELLOW(consoleHandle);
                printf("%0.2fkPa", shocks[i].tyrePresDiff);
                SET_TEXT_WHITE(consoleHandle);
                COLOUR_TEXT(consoleHandle, PURPLE, " over ");
                printf("optimal baseline of ");
                COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "180kPa");
            }
            else if (shocks[i].tyrePresDiff < -2.0f)
            {
                shocks[i].tyrePresDiff *= -1;
                // Under 178.0kPa
                printf("\tTyre Pressure: ");
                SET_TEXT_YELLOW(consoleHandle);
                printf("%0.2fkPa", shocks[i].tyrePresDiff);
                SET_TEXT_WHITE(consoleHandle);
                COLOUR_TEXT(consoleHandle, AQUA, " under ");
                printf("optimal baseline of ");
                COLOUR_TEXT_ENDLINE(consoleHandle, GREEN, "180kPa");
            }
        }
    }
    
    COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, "\n\tNOTE: If you are unable to complete the changes, \n\tan inverse adjustment on the other damper will make up for this");
    COLOUR_TEXT_ENDLINE(consoleHandle, YELLOW, "\n\teg: Increasing a LS Rebound by 2 clicks is equivalent to \n\tDecreasing the LS Compression by 2 clicks\n");
    
    return 0;
}
