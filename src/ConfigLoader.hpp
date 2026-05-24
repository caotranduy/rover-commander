/**
 * @file ConfigLoader.hpp
 * @brief Robust utility module for loading, parsing, and validating JSON configuration files.
 *
 * Implements a decoupled file stream parser using the nlohmann/json library,
 * featuring strict defensive checks against missing files or invalid syntax.
 */
#pragma once

#include <string>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

class ConfigLoader
{
public:
    /**
     * @brief Loads a JSON configuration file from disk into memory.
     * @param filepath Relative or absolute filesystem path to the target JSON file.
     * @return Parsed nlohmann::json object ready for structural querying.
     * @throws std::runtime_error if the file is inaccessible or contains syntax errors.
     */
    static nlohmann::json load(const std::string &filepath)
    {
        /* Open the file input stream in text mode */
        std::ifstream file(filepath);

        /* Defensive check: Verify if the file exists and is accessible by the OS */
        if (!file.is_open())
        {
            throw std::runtime_error("ConfigLoader Fatal: Unable to locate or open configuration file at: " + filepath);
        }

        nlohmann::json parsed_config;
        try
        {
            /* Deserialize the file stream directly into the modern JSON container */
            file >> parsed_config;
        }
        catch (const nlohmann::json::parse_error &e)
        {
            /* Intercept JSON syntax errors and wrap them into clean runtime exceptions */
            throw std::runtime_error("ConfigLoader Fatal: JSON grammatical syntax violation detected - " + std::string(e.what()));
        }

        /* Return by value; modern C++ utilizes Move Semantics (RVO) to make this zero-cost */
        return parsed_config;
    }
};