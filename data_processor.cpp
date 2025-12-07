// data_processor.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <regex>
#include <unordered_set>
#include <memory>
#include <iomanip>

// Simple CSV-based data structure
struct ExcelData {
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::vector<std::string>> validation_errors;
};

// Log manager class to handle both console and file logging
class LogManager {
private:
    std::ofstream log_file;
    std::string log_file_path;
    
public:
    LogManager() = default;
    
    bool initialize(const std::string& filepath) {
        log_file_path = filepath;
        log_file.open(filepath);
        if (!log_file.is_open()) {
            std::cerr << "ERROR: Cannot create log file " + filepath << std::endl;
            return false;
        }
        log("Log initialized: " + filepath);
        return true;
    }
    
    void log(const std::string& message) {
        // Write to console
        std::cout << message << std::endl;
        
        // Write to log file
        if (log_file.is_open()) {
            log_file << message << std::endl;
        }
    }
    
    void log_auto_correction(size_t row_num, const std::string& action, const std::string& original, const std::string& corrected) {
        std::string msg = "Row " + std::to_string(row_num + 1) + ": AUTO-CORRECTED: " + action + ": '" + original + "' -> '" + corrected + "'";
        log(msg);
    }
    
    void log_auto_fill(size_t row_num, const std::string& action, const std::string& value) {
        std::string msg = "Row " + std::to_string(row_num + 1) + ": AUTO-FILLED: " + action + " with '" + value + "'";
        log(msg);
    }
    
    void log_cleaned(size_t row_num, const std::string& field, const std::string& original, const std::string& cleaned) {
        std::string msg = "Row " + std::to_string(row_num + 1) + ": CLEANED " + field + ": '" + original + "' -> '" + cleaned + "'";
        log(msg);
    }
    
    void log_error(size_t row_num, const std::string& error) {
        std::string msg = "Row " + std::to_string(row_num + 1) + ": ERROR: " + error;
        log(msg);
    }
    
    void log_error(const std::string& error) {
        std::string msg = "ERROR: " + error;
        log(msg);
    }
    
    void log_warning(size_t row_num, const std::string& warning) {
        std::string msg = "Row " + std::to_string(row_num + 1) + ": WARNING: " + warning;
        log(msg);
    }
    
    void log_warning(const std::string& warning) {
        std::string msg = "WARNING: " + warning;
        log(msg);
    }
    
    void log_info(const std::string& info) {
        std::string msg = "INFO: " + info;
        log(msg);
    }
    
    void log_summary(const std::string& summary) {
        std::string msg = "SUMMARY: " + summary;
        log(msg);
    }
    
    void close() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }
    
    ~LogManager() {
        close();
    }
};

class DataProcessor {
private:
    ExcelData data;
    std::map<std::string, std::string> options;
    std::unordered_set<std::string> curp_set;
    std::unordered_set<std::string> control_number_set;
    std::vector<std::string> validation_summary;
    std::vector<bool> valid_rows;
    std::vector<size_t> problematic_rows;
    std::shared_ptr<LogManager> logger;

public:
    DataProcessor(const std::map<std::string, std::string>& opts, std::shared_ptr<LogManager> log_mgr) 
        : options(opts), logger(log_mgr) {
        if (!logger) {
            logger = std::make_shared<LogManager>();
        }
    }

    std::vector<std::vector<std::string>> getValidationErrors() const {
        return data.validation_errors;
    }

    std::vector<std::string> getHeaders() const {
        return data.headers;
    }

    std::vector<std::vector<std::string>> getProblematicRows() const {
        std::vector<std::vector<std::string>> result;
        for (size_t i : problematic_rows) {
            if (i < data.rows.size()) {
                result.push_back(data.rows[i]);
            }
        }
        return result;
    }

    std::string trimTrailingComma(const std::string& str) {
        if (str.empty()) return str;
        
        size_t end = str.length();
        while (end > 0 && str[end - 1] == ',') {
            end--;
        }
        
        return str.substr(0, end);
    }

    bool loadData(const std::string& inputFile) {
        std::ifstream file(inputFile);
        if (!file.is_open()) {
            logger->log_error("Cannot open file " + inputFile);
            return false;
        }

        std::string line;
        
        // Read headers (first line)
        if (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string header;
            while (std::getline(ss, header, ',')) {
                data.headers.push_back(trimTrailingComma(header));
            }
            logger->log_info("Loaded " + std::to_string(data.headers.size()) + " headers");
        }

        // Read data rows
        int row_count = 0;
        while (std::getline(file, line)) {
            row_count++;
            std::vector<std::string> row;
            std::stringstream ss(line);
            std::string cell;
            
            while (std::getline(ss, cell, ',')) {
                row.push_back(trimTrailingComma(cell));
            }
            
            // Fix: Ensure row has correct number of columns
            if (row.size() != data.headers.size()) {
                logger->log_warning("Row " + std::to_string(row_count) + " has " + std::to_string(row.size()) + 
                                   " columns, expected " + std::to_string(data.headers.size()));
                
                // Remove empty cells at the end (from trailing commas)
                while (!row.empty() && row.back().empty() && row.size() > data.headers.size()) {
                    row.pop_back();
                }
                
                // Final resize if needed
                if (row.size() < data.headers.size()) {
                    row.resize(data.headers.size(), "");
                    logger->log_info("Padded row " + std::to_string(row_count) + " with empty cells");
                } else if (row.size() > data.headers.size()) {
                    row.resize(data.headers.size());
                    logger->log_info("Truncated row " + std::to_string(row_count) + " to " + 
                                    std::to_string(data.headers.size()) + " columns");
                }
            }
            
            data.rows.push_back(row);
            data.validation_errors.push_back(std::vector<std::string>(row.size(), ""));
        }

        file.close();
        logger->log_info("Loaded " + std::to_string(data.rows.size()) + " rows from " + inputFile);
        return true;
    }

    bool saveData(const std::string& outputFile) {
        std::ofstream file(outputFile);
        if (!file.is_open()) {
            logger->log_error("Cannot create file " + outputFile);
            return false;
        }

        // Debug: Log what we're writing
        logger->log_info("Saving data to " + outputFile);
        logger->log_info("Total rows: " + std::to_string(data.rows.size()));
        logger->log_info("Problematic rows count: " + std::to_string(problematic_rows.size()));
        
        // Debug first few rows
        for (size_t i = 0; i < std::min(data.rows.size(), (size_t)3); ++i) {
            std::string row_debug = "Row " + std::to_string(i) + ": ";
            for (size_t j = 0; j < std::min(data.rows[i].size(), (size_t)3); ++j) {
                if (j > 0) row_debug += ", ";
                row_debug += data.rows[i][j];
            }
            logger->log_info(row_debug);
        }

        // Write only valid rows
        int valid_count = 0;
        for (size_t i = 0; i < data.rows.size(); ++i) {
            if (std::find(problematic_rows.begin(), problematic_rows.end(), i) != problematic_rows.end()) {
                logger->log_info("Skipping problematic row: " + std::to_string(i));
                continue;
            }
            
            const auto& row = data.rows[i];
            logger->log_info("Writing row: " + std::to_string(i));
            for (size_t j = 0; j < row.size(); ++j) {
                if (j > 0) file << ",";
                file << escapeCSV(row[j]);
            }
            file << "\n";
            valid_count++;
        }

        file.close();
        logger->log_info("Saved " + std::to_string(valid_count) + " valid records to " + outputFile);
        return true;
    }

    std::string escapeCSV(const std::string& value) {
        if (value.find(',') != std::string::npos || 
            value.find('"') != std::string::npos || 
            value.find('\n') != std::string::npos) {
            std::string escaped = value;
            size_t pos = 0;
            while ((pos = escaped.find('"', pos)) != std::string::npos) {
                escaped.replace(pos, 1, "\"\"");
                pos += 2;
            }
            return "\"" + escaped + "\"";
        }
        return value;
    }

    void processData() {
        logger->log_info("Starting validation process...");
        validateAllFields();
        
        // Text replacement (if specified)
        if (options.find("find") != options.end() && options.find("replace") != options.end()) {
            logger->log_info("Applying text replacement: '" + options["find"] + "' -> '" + options["replace"] + "'");
            replaceTextPattern(options["find"], options["replace"]);
        }

        // Text case transformation (if specified)
        if (options.find("case") != options.end()) {
            logger->log_info("Applying case transformation: " + options["case"]);
            transformTextCase(options["case"]);
        }
        
        logger->log_info("Validation process completed");
    }

    bool saveProblematicRows(const std::string& outputFile) {
        if (problematic_rows.empty()) {
            logger->log_info("No problematic records to save");
            return true;
        }

        std::ofstream file(outputFile);
        if (!file.is_open()) {
            logger->log_error("Cannot create file " + outputFile);
            return false;
        }

        // Write headers
        for (size_t i = 0; i < data.headers.size(); ++i) {
            if (i > 0) file << ",";
            file << escapeCSV(data.headers[i]);
        }
        file << "\n";

        // Write problematic rows
        for (size_t i : problematic_rows) {
            if (i < data.rows.size()) {
                const auto& row = data.rows[i];
                for (size_t j = 0; j < row.size(); ++j) {
                    if (j > 0) file << ",";
                    file << escapeCSV(row[j]);
                }
                file << "\n";
            }
        }

        file.close();
        logger->log_info("Saved " + std::to_string(problematic_rows.size()) + " problematic records to " + outputFile);
        return true;
    }

private:
    void validateAllFields() {
        logger->log_info("Validating all fields...");
        curp_set.clear();
        control_number_set.clear();
        validation_summary.clear();

        for (size_t i = 0; i < data.rows.size(); ++i) {
            auto& row = data.rows[i];
            
            // Get CURP value for cross-validation
            std::string curp_value = "";
            std::string nombres_value = "";
            std::string a_paterno_value = "";
            std::string a_materno_value = "";
            
            int nombres_idx = -1, a_paterno_idx = -1, a_materno_idx = -1;
            
            // Validate each field based on header position
            for (size_t j = 0; j < data.headers.size() && j < row.size(); ++j) {
                std::string header = data.headers[j];
                std::string value = row[j];
                
                if (header == "ctr") {
                    validateControlNumber(value, i, j);
                } else if (header == "cur") {
                    validateCURP(value, i, j);
                    curp_value = value;
                } else if (header == "nom") {
                    validateName(value, i, j, "Name");
                    nombres_value = value;
                    nombres_idx = j;
                } else if (header == "app") {
                    validateLastName(value, i, j, "Paternal Last Name");
                    a_paterno_value = value;
                    a_paterno_idx = j;
                } else if (header == "apm") {
                    // Maternal last name can be empty, but if not empty, validate
                    if (!value.empty()) {
                        validateName(value, i, j, "Maternal Last Name");
                    }
                    a_materno_value = value;
                    a_materno_idx = j;
                } else if (header == "sem") {
                    validateSemester(value, i, j);
                } else if (header == "sex") {
                    validateGender(value, i, j);
                } else if (header == "psa1") {
                    validateAverage(value, i, j, "Current Average");
                } else if (header == "pge") {
                    validateAverage(value, i, j, "General Average");
                } else if (header == "cac") {
                    validateCredits(value, i, j);
                } else if (header == "res") {
                    validateYesNo(value, i, j, "Professional Residences");
                } else if (header == "ema") {
                    validateEmail(value, i, j);
                } else if (header == "rfc") {
                    validateRFC(value, i, j);
                } else if (header == "cel") {
                    validatePhone(value, i, j);
                } else if (header == "dis") {
                    validateYesNo(value, i, j, "Disability");
                } else if (header == "tipo_discapacidad") {
                    validateDisabilityType(value, i, j);
                } else if (header == "lengua_indigena") {
                    validateYesNo(value, i, j, "Indigenous Language");
                } else if (header == "reingreso") {
                    validateYesNo(value, i, j, "Re-entry");
                } else if (header == "movilidad") {
                    
                }
            }
            
            // Cross-field validations
            validateCrossFieldRules(i);
            
            // Validate names with CURP
            if (!curp_value.empty()) {
                if (!nombres_value.empty() && nombres_idx != -1) {
                    validateNameWithCURP(nombres_value, curp_value, i, nombres_idx);
                }
                if (!a_paterno_value.empty() && a_paterno_idx != -1) {
                    validatePaternalLastNameWithCURP(a_paterno_value, curp_value, i, a_paterno_idx);
                }
                if (a_materno_idx != -1) {
                    validateMaternalLastNameWithCURP(a_materno_value, curp_value, i, a_materno_idx);
                }
            }
        }
        
        printValidationSummary();
    }

    void validateControlNumber(const std::string& value, size_t row_idx, size_t col_idx) {
        if (value.empty()) {
            addError(row_idx, col_idx, "Control number cannot be empty");
            logger->log_warning(row_idx, "Control number is empty");
            return;
        }
        
        // Check length (typical control numbers are 8-12 digits)
        if (value.length() < 8 || value.length() > 12) {
            addError(row_idx, col_idx, "Control number should be 8-12 digits");
            logger->log_warning(row_idx, "Control number length invalid: " + value);
        }
        
        // Check for duplicates
        if (control_number_set.find(value) != control_number_set.end()) {
            addError(row_idx, col_idx, "Duplicate control number found");
            logger->log_warning(row_idx, "Duplicate control number: " + value);
        } else {
            control_number_set.insert(value);
        }
    }

    void validateCURP(const std::string& value, size_t row_idx, size_t col_idx) {
        bool has_curp_error = false;

        if (value.empty()) {
            addError(row_idx, col_idx, "CURP cannot be empty");
            has_curp_error = true;
            logger->log_error(row_idx, "CURP is empty");
            problematic_rows.push_back(row_idx);
            return;
        }
        
        // Check for duplicates
        if (curp_set.find(value) != curp_set.end()) {
            addError(row_idx, col_idx, "Duplicate CURP found");
            has_curp_error = true;
            logger->log_warning(row_idx, "Duplicate CURP: " + value);
        } else {
            curp_set.insert(value);
        }
        
        // Basic CURP structure validation (18 characters, alphanumeric)
        if (value.length() != 18) {
            addError(row_idx, col_idx, "CURP must be exactly 18 characters");
            logger->log_warning(row_idx, "CURP length invalid: " + 
                               std::to_string(value.length()) + " (should be 18)");
        }
        
        if (!std::all_of(value.begin(), value.end(), [](char c) {
            return std::isalnum(c);
        })) {
            addError(row_idx, col_idx, "CURP contains invalid characters");
            has_curp_error = true;
            logger->log_warning(row_idx, "CURP contains invalid characters");
        }
        
        // Validate CURP format: 4 letters, 6 digits, 1 letter, 1 digit, 1 letter, 4 digits
        if (value.length() == 18 && !has_curp_error) {
            // First 4 characters should be letters
            for (int i = 0; i < 4; i++) {
                if (!std::isalpha(value[i])) {
                    addError(row_idx, col_idx, "CURP format invalid: first 4 characters should be letters");
                    logger->log_warning(row_idx, "CURP first 4 chars should be letters");
                    break;
                }
            }
            
            // Next 6 characters should be digits (birth date)
            for (int i = 4; i < 10; i++) {
                if (!std::isdigit(value[i])) {
                    addError(row_idx, col_idx, "CURP format invalid: characters 5-10 should be digits (birth date)");
                    logger->log_warning(row_idx, "CURP date should be digits");
                    break;
                }
            }
            
            // Character 11 should be a letter (gender)
            if (!std::isalpha(value[10])) {
                addError(row_idx, col_idx, "CURP format invalid: character 11 should be a letter (gender)");
                logger->log_warning(row_idx, "CURP gender should be letter");
            }
            
            // Character 12 should be a letter (state)
            if (!std::isalpha(value[11])) {
                addError(row_idx, col_idx, "CURP format invalid: character 12 should be a letter (state)");
                logger->log_warning(row_idx, "CURP state should be letter");
            }
            
            // Characters 13-16 should be alphanumeric
            for (int i = 12; i < 16; i++) {
                if (!std::isalnum(value[i])) {
                    addError(row_idx, col_idx, "CURP format invalid: characters 13-16 should be alphanumeric");
                    logger->log_warning(row_idx, "CURP characters 13-16 invalid");
                    break;
                }
            }
            
            // Last 2 characters should be digits
            for (int i = 16; i < 18; i++) {
                if (!std::isalnum(value[i])) {
                    addError(row_idx, col_idx, "CURP format invalid: last 2 characters should be digits");
                    logger->log_warning(row_idx, "CURP last 2 chars should be digits");
                    break;
                }
            }
        }

        if (has_curp_error){
            problematic_rows.push_back(row_idx);
        }
    }

    void validateName(const std::string& value, size_t row_idx, size_t col_idx, const std::string& field_name) {
        if (value.empty()) {
            addError(row_idx, col_idx, field_name + " cannot be empty");
            logger->log_warning(row_idx, field_name + " is empty");
            return;
        }
        
        // Check for valid characters (letters, accents, spaces, and basic punctuation)
        bool has_invalid_chars = false;
        for (size_t i = 0; i < value.length(); i++) {
            unsigned char c = value[i];
            
            // Basic ASCII letters and punctuation
            if (std::isalpha(c) || std::isspace(c) || c == '.' || c == '-' || c == '\'') {
                continue;
            }
            
            // Handle UTF-8 encoded accented characters (common in Spanish)
            // Check for 2-byte UTF-8 sequences for accented characters
            if ((c & 0xE0) == 0xC0 && i + 1 < value.length()) {
                // This is a 2-byte UTF-8 character
                unsigned char next = value[i + 1];
                // Check if it's a common Spanish accented character
                if ((c == 0xC3 && (
                    (next >= 0x80 && next <= 0x85) || // À Á Â Ã Ä
                    (next >= 0x88 && next <= 0x8B) || // È É Ê Ë
                    (next >= 0x8C && next <= 0x8F) || // Ì Í Î Ï
                    (next >= 0x92 && next <= 0x96) || // Ò Ó Ô Õ Ö
                    (next >= 0x99 && next <= 0x9C) || // Ù Ú Û Ü
                    (next >= 0x9D) ||                 // Ý
                    (next >= 0xA0 && next <= 0xA5) || // à á â ã ä
                    (next >= 0xA8 && next <= 0xAB) || // è é ê ë
                    (next >= 0xAC && next <= 0xAF) || // ì í î ï
                    (next >= 0xB2 && next <= 0xB6) || // ò ó ô õ ö
                    (next >= 0xB9 && next <= 0xBC) || // ù ú Û ü
                    next == 0x81 || next == 0x91 ||   // ñ Ñ
                    next == 0x87 || next == 0xA7      // ç Ç
                ))) {
                    i++; // Skip the next byte since we processed it
                    continue;
                }
            }
            
            has_invalid_chars = true;
            break;
        }
        
        if (has_invalid_chars) {
            addError(row_idx, col_idx, field_name + " contains invalid characters");
            logger->log_warning(row_idx, field_name + " contains invalid characters");
        }
        
        // Check minimum length
        if (value.length() < 2) {
            addError(row_idx, col_idx, field_name + " is too short");
            logger->log_warning(row_idx, field_name + " is too short");
            return;
        }
        
        // Check for excessive repeated letters (3 or more consecutive identical letters)
        if (hasExcessiveRepeatedLetters(value)) {
            addError(row_idx, col_idx, field_name + " contains excessive repeated letters");
            logger->log_warning(row_idx, field_name + " has excessive repeated letters");
        }
        
        // Check for numbers in names
        if (containsNumbers(value)) {
            addError(row_idx, col_idx, field_name + " contains numbers");
            logger->log_warning(row_idx, field_name + " contains numbers");
        }
    }
    
    bool hasExcessiveRepeatedLetters(const std::string& str) {
        if (str.length() < 3) return false;
            
        for (size_t i = 0; i < str.length() - 2; i++) {
            char current = std::tolower(str[i]);
            // Only check alphabetic characters for repetition
            if (std::isalpha(current) && 
                std::tolower(str[i+1]) == current && 
                std::tolower(str[i+2]) == current) {
                return true;
            }
        }
        return false;
    }
        
    bool containsNumbers(const std::string& str) {
        for (char c : str) {
            if (std::isdigit(c)) {
                return true;
            }
        }
        return false;
    }

    void validateNameWithCURP(const std::string& nombres_value, const std::string& curp_value, 
                         size_t row_idx, size_t nombres_col_idx) {
        if (nombres_value.empty() || curp_value.empty() || curp_value.length() < 4) {
            return;
        }
        
        // Get the first character of the first name
        char first_char_nombre = ' ';
        std::string first_name = nombres_value;
        
        // Extract first name if there are multiple names
        size_t space_pos = first_name.find(' ');
        if (space_pos != std::string::npos) {
            first_name = first_name.substr(0, space_pos);
        }
        
        for (char c : first_name) {
            if (std::isalpha(c)) {
                first_char_nombre = std::toupper(c);
                break;
            }
        }
        
        // Get the fourth character of CURP (should be the first letter of the first name)
        char fourth_char_curp = std::toupper(curp_value[3]);
        
        // Validate: 4th CURP character should match first name initial or be 'X'
        if (first_char_nombre != ' ') {
            if (fourth_char_curp != first_char_nombre && fourth_char_curp != 'X') {
                addError(row_idx, nombres_col_idx, 
                        "First name initial '" + std::string(1, first_char_nombre) + 
                        "' doesn't match CURP 4th character '" + std::string(1, fourth_char_curp) + 
                        "' (should match or be 'X')");
                logger->log_warning(row_idx, "Name-CURP mismatch: '" + 
                                   std::string(1, first_char_nombre) + "' vs '" + 
                                   std::string(1, fourth_char_curp) + "'");
            }
        }
    }

    void validatePaternalLastNameWithCURP(const std::string& a_paterno_value, const std::string& curp_value, 
                                        size_t row_idx, size_t a_paterno_col_idx) {
        if (a_paterno_value.empty() || curp_value.empty() || curp_value.length() < 2) {
            return;
        }
        
        // Get first character of paternal last name
        char first_char_paterno = ' ';
        for (char c : a_paterno_value) {
            if (std::isalpha(c)) {
                first_char_paterno = std::toupper(c);
                break;
            }
        }
        
        // Get first vowel of paternal last name (excluding first character)
        char first_vowel_paterno = ' ';
        bool found_first_consonant = false;
        
        for (char c : a_paterno_value) {
            char upper_c = std::toupper(c);
            if (std::isalpha(upper_c)) {
                if (!found_first_consonant) {
                    found_first_consonant = true;
                    continue; // Skip first character
                }
                
                // Check if it's a vowel
                if (upper_c == 'A' || upper_c == 'E' || upper_c == 'I' || upper_c == 'O' || upper_c == 'U') {
                    first_vowel_paterno = upper_c;
                    break;
                }
            }
        }
        
        // If no vowel found after first character, use 'X'
        if (first_vowel_paterno == ' ') {
            first_vowel_paterno = 'X';
        }
        
        // Get first two characters of CURP
        char first_char_curp = std::toupper(curp_value[0]);
        char second_char_curp = std::toupper(curp_value[1]);
        
        // Validate: First CURP character should match first letter of paternal last name
        if (first_char_paterno != ' ' && first_char_curp != first_char_paterno) {
            addError(row_idx, a_paterno_col_idx, 
                    "Paternal last name first letter '" + std::string(1, first_char_paterno) + 
                    "' doesn't match CURP 1st character '" + std::string(1, first_char_curp) + "'");
            logger->log_warning(row_idx, "Paternal last name-CURP mismatch");
        }
        
        // Validate: Second CURP character should match first vowel of paternal last name (or 'X')
        if (first_vowel_paterno != ' ' && second_char_curp != first_vowel_paterno) {
            addError(row_idx, a_paterno_col_idx, 
                    "Paternal last name first vowel '" + std::string(1, first_vowel_paterno) + 
                    "' doesn't match CURP 2nd character '" + std::string(1, second_char_curp) + "'");
            logger->log_warning(row_idx, "Paternal last name vowel-CURP mismatch");
        }
    }

    void validateMaternalLastNameWithCURP(const std::string& a_materno_value, const std::string& curp_value, 
                                        size_t row_idx, size_t a_materno_col_idx) {
        if (curp_value.empty() || curp_value.length() < 3) {
            return;
        }
        
        // Get first character of maternal last name
        char first_char_materno = ' ';
        if (!a_materno_value.empty()) {
            for (char c : a_materno_value) {
                if (std::isalpha(c)) {
                    first_char_materno = std::toupper(c);
                    break;
                }
            }
        }
        
        // If maternal last name is empty, first_char_materno remains ' '
        
        // Get third character of CURP
        char third_char_curp = std::toupper(curp_value[2]);
        
        // Validate: Third CURP character should match first letter of maternal last name or be 'X'
        if (a_materno_value.empty()) {
            // If no maternal last name, CURP should use 'X'
            if (third_char_curp != 'X') {
                addError(row_idx, a_materno_col_idx, 
                        "No maternal last name but CURP 3rd character is '" + std::string(1, third_char_curp) + 
                        "' (should be 'X')");
                logger->log_warning(row_idx, "No maternal last name but CURP not 'X'");
            }
        } else {
            // If maternal last name exists, should match first letter or be 'X'
            if (first_char_materno != ' ' && third_char_curp != first_char_materno && third_char_curp != 'X') {
                addError(row_idx, a_materno_col_idx, 
                        "Maternal last name first letter '" + std::string(1, first_char_materno) + 
                        "' doesn't match CURP 3rd character '" + std::string(1, third_char_curp) + 
                        "' (should match or be 'X')");
                logger->log_warning(row_idx, "Maternal last name-CURP mismatch");
            }
        }
    }

    void validateLastName(const std::string& value, size_t row_idx, size_t col_idx, const std::string& field_name) {
        if (value.empty()) {
            addError(row_idx, col_idx, "At least one last name is required");
            logger->log_warning(row_idx, "Last name is required");
            return;
        }
        validateName(value, row_idx, col_idx, field_name);
    }

    void validateSemester(const std::string& value, size_t row_idx, size_t col_idx) {
        if (value.empty()) {
            addError(row_idx, col_idx, "Semester cannot be empty");
            logger->log_warning(row_idx, "Semester is empty");
            return;
        }
        
        try {
            int semester = std::stoi(value);
            if (semester < 1 || semester > 100) {
                addError(row_idx, col_idx, "Semester must be between 1 and 100");
                logger->log_warning(row_idx, "Semester out of range: " + value);
            }
        } catch (...) {
            addError(row_idx, col_idx, "Semester must be an integer number");
            logger->log_warning(row_idx, "Semester not integer: " + value);
        }
    }

    void validateGender(const std::string& value, size_t row_idx, size_t col_idx) {
        std::string original_value = value;
        std::string corrected_value = value;
        
        if (value == "M" || value == "m") {
            corrected_value = "H";
            data.rows[row_idx][col_idx] = "H";
            logger->log_auto_correction(row_idx, "Gender", original_value, "H");
        }
        
        if (value == "F" || value == "f") {
            corrected_value = "M";
            data.rows[row_idx][col_idx] = "M";
            logger->log_auto_correction(row_idx, "Gender", original_value, "M");
        }
        
        if (value.empty()) {
            corrected_value = "H";
            data.rows[row_idx][col_idx] = "H";
            addError(row_idx, col_idx, "Gender cannot be empty, Added 'H' by default");
            logger->log_auto_fill(row_idx, "Gender", "H");
            return;
        }
        
        if (corrected_value != "H" && corrected_value != "M") {
            addError(row_idx, col_idx, "Gender must be 'H' or 'M'");
            logger->log_warning(row_idx, "Invalid gender: " + value);
        }
    }

    void validateAverage(const std::string& value, size_t row_idx, size_t col_idx, const std::string& field_name) {
        if (value.empty()) {
            addError(row_idx, col_idx, field_name + " cannot be empty");
            logger->log_warning(row_idx, field_name + " is empty");
            return;
        }
        
        try {
            float avg = std::stof(value);
            if (avg < 0.0f || avg > 100.0f) {
                addError(row_idx, col_idx, field_name + " must be between 0.0 and 100.0");
                logger->log_warning(row_idx, field_name + " out of range: " + value);
            }
            // Removed all auto-correction code for decimal places
            
        } catch (...) {
            addError(row_idx, col_idx, field_name + " must be a valid number (e.g., 89.87)");
            logger->log_warning(row_idx, field_name + " not a number: " + value);
        }
    }

    void validateCredits(const std::string& value, size_t row_idx, size_t col_idx) {
        if (value.empty()) {
            addError(row_idx, col_idx, "Accumulated credits cannot be empty");
            logger->log_warning(row_idx, "Credits is empty");
            return;
        }
        
        try {
            int credits = std::stoi(value);
            if (credits < 0) {
                addError(row_idx, col_idx, "Credits cannot be negative");
                logger->log_warning(row_idx, "Credits negative: " + value);
            }
            
            // For new students, credits should be 0
            // This will be validated in cross-field validation
        } catch (...) {
            addError(row_idx, col_idx, "Credits must be an integer number");
            logger->log_warning(row_idx, "Credits not integer: " + value);
        }
    }

    void validateYesNo(const std::string& value, size_t row_idx, size_t col_idx, const std::string& field_name) {
        std::string original_value = value;
        std::string cleaned_value = value;
        
        // If empty, auto-fill with "N"
        if (value.empty()) {
            cleaned_value = "N";
            data.rows[row_idx][col_idx] = "N";
            addError(row_idx, col_idx, field_name + " was empty - auto-filled with 'N'");
            logger->log_auto_fill(row_idx, field_name, "N");
        }
        
        // Trim whitespace
        if (!cleaned_value.empty()) {
            size_t start = cleaned_value.find_first_not_of(" \t\n\r");
            size_t end = cleaned_value.find_last_not_of(" \t\n\r");
            
            if (start != std::string::npos) {
                cleaned_value = cleaned_value.substr(start, end - start + 1);
            }
        }
        
        // Convert to uppercase for validation
        std::string uppercase_value = cleaned_value;
        std::transform(uppercase_value.begin(), uppercase_value.end(), uppercase_value.begin(), ::toupper);
        
        // Check if valid (S or N)
        if (uppercase_value != "S" && uppercase_value != "N") {
            // Try to correct common variations
            if (uppercase_value == "SI" || uppercase_value == "YES" || uppercase_value == "Y" || uppercase_value == "1") {
                cleaned_value = "S";
                data.rows[row_idx][col_idx] = "S";
                logger->log_auto_correction(row_idx, field_name, original_value, "S");
            } else if (uppercase_value == "NO" || uppercase_value == "0") {
                cleaned_value = "N";
                data.rows[row_idx][col_idx] = "N";
                logger->log_auto_correction(row_idx, field_name, original_value, "N");
            } else {
                addError(row_idx, col_idx, field_name + " must be 'S' or 'N' (was: '" + value + "')");
                logger->log_warning(row_idx, field_name + " invalid: " + value);
            }
        } else if (cleaned_value != uppercase_value) {
            // Auto-correct case if needed
            data.rows[row_idx][col_idx] = uppercase_value;
            if (original_value != uppercase_value) {
                logger->log_auto_correction(row_idx, field_name, original_value, uppercase_value);
            }
        }
    }

    void validateEmail(const std::string& value, size_t row_idx, size_t col_idx) {
        // Extract control number from the same row
        std::string control_number = "";
        for (size_t j = 0; j < data.headers.size(); j++) {
            if (data.headers[j] == "ctr" && j < data.rows[row_idx].size()) {
                control_number = data.rows[row_idx][j];
                break;
            }
        }
        
        // Build expected email
        std::string expected_email = "al" + control_number + "@ite.edu.mx";
        std::transform(expected_email.begin(), expected_email.end(), expected_email.begin(), ::tolower);
        
        // Check if empty or incorrect
        if (value.empty() || value != expected_email) {
            // Auto-fix to expected email
            data.rows[row_idx][col_idx] = expected_email;
            
            if (value.empty()) {
                addError(row_idx, col_idx, "Email was empty - auto-filled with " + expected_email);
                logger->log_auto_fill(row_idx, "Email", expected_email);
            } else {
                logger->log_auto_correction(row_idx, "Email", value, expected_email);
            }
        }
    }

    void validateRFC(const std::string& value, size_t row_idx, size_t col_idx) {
        // First, check if empty
        if (value.empty()) {
            data.rows[row_idx][col_idx] = "XAXX010101000";
            addError(row_idx, col_idx, "RFC was empty - auto-filled with XAXX010101000");
            logger->log_auto_fill(row_idx, "RFC", "XAXX010101000");
            return;
        }
        
        // Clean the value - remove non-printable characters
        std::string cleaned_value;
        for (char c : value) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (std::isprint(uc) || std::isalnum(uc) || c == ' ' || 
                c == '-' || c == '&') {
                cleaned_value += c;
            }
        }
        
        // Trim whitespace
        size_t start = cleaned_value.find_first_not_of(" \t\n\r");
        size_t end = cleaned_value.find_last_not_of(" \t\n\r");
        
        if (start == std::string::npos || cleaned_value.empty()) {
            // After cleaning, it's empty or only whitespace
            data.rows[row_idx][col_idx] = "XAXX010101000";
            addError(row_idx, col_idx, "RFC invalid - auto-filled with XAXX010101000");
            logger->log_auto_fill(row_idx, "RFC (invalid chars)", "XAXX010101000");
            return;
        }
        
        cleaned_value = cleaned_value.substr(start, end - start + 1);
        
        // Check length - use cleaned value
        if (cleaned_value.length() < 10) {
            data.rows[row_idx][col_idx] = "XAXX010101000";
            addError(row_idx, col_idx, "RFC invalid length - auto-filled with XAXX010101000");
            logger->log_auto_fill(row_idx, "RFC (wrong length: " + std::to_string(cleaned_value.length()) + ")", "XAXX010101000");
            return;
        }
        
        // Check for valid characters (alphanumeric, hyphen, and ampersand)
        if (!std::all_of(cleaned_value.begin(), cleaned_value.end(), [](char c) {
            unsigned char uc = static_cast<unsigned char>(c);
            return std::isalnum(uc) || c == '-' || c == '&';
        })) {
            addError(row_idx, col_idx, "RFC contains invalid characters (only letters, numbers, '-', and '&' allowed)");
            logger->log_warning(row_idx, "RFC invalid characters");
            return;
        }
        
        // Check if all letters are uppercase
        if (!std::all_of(cleaned_value.begin(), cleaned_value.end(), [](char c) {
            unsigned char uc = static_cast<unsigned char>(c);
            return !std::isalpha(uc) || std::isupper(uc);
        })) {
            addError(row_idx, col_idx, "RFC must be in uppercase letters");
            logger->log_warning(row_idx, "RFC not uppercase");
            return;
        }
        
        // Validate structure based on length
        if (cleaned_value.length() == 13) {
            // Persona Física: 4 letras + 6 dígitos + 3 alfanuméricos
            validatePersonaFisicaRFC(cleaned_value, row_idx, col_idx);
        } else if (cleaned_value.length() == 12) {
            // Persona Moral: guión + 3 letras + 6 dígitos + 3 alphanuméricos
            validatePersonaMoralRFC(cleaned_value, row_idx, col_idx);
        }
    }

    void validatePersonaFisicaRFC(const std::string& value, size_t row_idx, size_t col_idx) {
        // First 4 characters should be letters
        for (int i = 0; i < 4; i++) {
            if (!std::isalpha(value[i])) {
                addError(row_idx, col_idx, "RFC persona física: first 4 characters should be letters");
                logger->log_warning(row_idx, "RFC first 4 chars not letters");
                break;
            }
        }
        
        // Next 6 characters should be digits (date: YYMMDD)
        for (int i = 4; i < 10; i++) {
            if (!std::isdigit(value[i])) {
                addError(row_idx, col_idx, "RFC persona física: characters 5-10 should be digits (date YYMMDD)");
                logger->log_warning(row_idx, "RFC date not digits");
                break;
            }
        }
        
        // Validate date components
        if (value.length() >= 10) {
            std::string year_str = value.substr(4, 2);
            std::string month_str = value.substr(6, 2);
            std::string day_str = value.substr(8, 2);
            
            try {
                int month = std::stoi(month_str);
                int day = std::stoi(day_str);
                
                // Basic date validation
                if (month < 1 || month > 12) {
                    addError(row_idx, col_idx, "RFC: invalid month in date (should be 01-12)");
                    logger->log_warning(row_idx, "RFC invalid month");
                }
                if (day < 1 || day > 31) {
                    addError(row_idx, col_idx, "RFC: invalid day in date (should be 01-31)");
                    logger->log_warning(row_idx, "RFC invalid day");
                }
            } catch (...) {
                addError(row_idx, col_idx, "RFC: invalid date format");
                logger->log_warning(row_idx, "RFC invalid date format");
            }
        }
        
        // Last 3 characters should be alphanumeric (homoclave)
        for (int i = 10; i < 13; i++) {
            if (!std::isalnum(value[i])) {
                addError(row_idx, col_idx, "RFC persona física: last 3 characters should be alphanumeric (homoclave)");
                logger->log_warning(row_idx, "RFC homoclave invalid");
                break;
            }
        }
    }

    void validatePersonaMoralRFC(const std::string& value, size_t row_idx, size_t col_idx) {
        // First character should be hyphen
        if (value[0] != '-') {
            addError(row_idx, col_idx, "RFC persona moral: first character should be '-'");
            logger->log_warning(row_idx, "RFC should start with '-'");
            return;
        }
        
        // Next 3 characters should be letters
        for (int i = 1; i < 4; i++) {
            if (!std::isalpha(value[i])) {
                addError(row_idx, col_idx, "RFC persona moral: characters 2-4 should be letters");
                logger->log_warning(row_idx, "RFC characters 2-4 not letters");
                break;
            }
        }
        
        // Next 6 characters should be digits (date: YYMMDD)
        for (int i = 4; i < 10; i++) {
            if (!std::isdigit(value[i])) {
                addError(row_idx, col_idx, "RFC persona moral: characters 5-10 should be digits (date YYMMDD)");
                logger->log_warning(row_idx, "RFC date not digits");
                break;
            }
        }
        
        // Validate date components
        if (value.length() >= 10) {
            std::string year_str = value.substr(4, 2);
            std::string month_str = value.substr(6, 2);
            std::string day_str = value.substr(8, 2);
            
            try {
                int month = std::stoi(month_str);
                int day = std::stoi(day_str);
                
                // Basic date validation
                if (month < 1 || month > 12) {
                    addError(row_idx, col_idx, "RFC: invalid month in date (should be 01-12)");
                    logger->log_warning(row_idx, "RFC invalid month");
                }
                if (day < 1 || day > 31) {
                    addError(row_idx, col_idx, "RFC: invalid day in date (should be 01-31)");
                    logger->log_warning(row_idx, "RFC invalid day");
                }
            } catch (...) {
                addError(row_idx, col_idx, "RFC: invalid date format");
                logger->log_warning(row_idx, "RFC invalid date format");
            }
        }
        
        // Last 3 characters should be alphanumeric (homoclave)
        for (int i = 10; i < 12; i++) {
            if (!std::isalnum(value[i])) {
                addError(row_idx, col_idx, "RFC persona moral: last 3 characters should be alphanumeric (homoclave)");
                logger->log_warning(row_idx, "RFC homoclave invalid");
                break;
            }
        }
    }

    void validatePhone(const std::string& value, size_t row_idx, size_t col_idx) {
        // Clean the phone number first
        std::string cleaned_phone;
        for (char c : value) {
            if (std::isdigit(c)) {
                cleaned_phone += c;
            }
            // Skip common phone number separators
            else if (c != ' ' && c != '-' && c != '(' && c != ')' && c != '.') {
                // If there are other invalid characters, add an error
                addError(row_idx, col_idx, "Phone number contains invalid character: '" + std::string(1, c) + "'");
                logger->log_warning(row_idx, "Phone contains invalid char: '" + std::string(1, c) + "'");
            }
        }
        
        // Check if empty after cleaning OR if original was empty
        if (value.empty() || cleaned_phone.empty()) {
            // Auto-fill with "1234567890"
            data.rows[row_idx][col_idx] = "1234567890";
            addError(row_idx, col_idx, "Phone number was empty/invalid - auto-filled with '1234567890'");
            logger->log_auto_fill(row_idx, "Phone", "1234567890");
            return;
        }
        
        // Update the cell with cleaned phone number if different
        if (cleaned_phone != value) {
            data.rows[row_idx][col_idx] = cleaned_phone;
            logger->log_cleaned(row_idx, "Phone", value, cleaned_phone);
        }
        
        // Check for exactly 10 digits
        if (cleaned_phone.length() != 10) {
            addError(row_idx, col_idx, "Phone number must be exactly 10 digits (after cleaning: " + cleaned_phone + ", length: " + std::to_string(cleaned_phone.length()) + ")");
            logger->log_warning(row_idx, "Phone wrong length: " + std::to_string(cleaned_phone.length()));
        }
        
        // Final validation: all characters should be digits
        if (!std::all_of(cleaned_phone.begin(), cleaned_phone.end(), ::isdigit)) {
            addError(row_idx, col_idx, "Phone number must contain only digits");
            logger->log_warning(row_idx, "Phone contains non-digits");
        }
    }

    void validateDisabilityType(const std::string& value, size_t row_idx, size_t col_idx) {
        // Find disability field in the same row
        std::string disability = "";
        for (size_t j = 0; j < data.headers.size(); j++) {
            if (data.headers[j] == "dis" && j < data.rows[row_idx].size()) {  // Changed to "dis"
                disability = data.rows[row_idx][j];
                break;
            }
        }
        
        // Convert disability to uppercase for comparison
        std::string disability_upper = disability;
        std::transform(disability_upper.begin(), disability_upper.end(), disability_upper.begin(), ::toupper);
        
        // If disability is "S", disability type cannot be empty
        if (disability_upper == "S" && value.empty()) {
            addError(row_idx, col_idx, "Disability type is required when disability is 'S'");
            logger->log_warning(row_idx, "Disability type required when S");
        }
        
        // If disability is "N", disability type should be empty
        if (disability_upper == "N" && !value.empty()) {
            addError(row_idx, col_idx, "Disability type should be empty when disability is 'N'");
            logger->log_warning(row_idx, "Disability type should be empty when N");
        }
        
        // If disability type is provided and disability is "S", validate the content
        if (disability_upper == "S" && !value.empty()) {
            // Trim whitespace
            std::string trimmed_value = value;
            size_t start = trimmed_value.find_first_not_of(" \t\n\r");
            size_t end = trimmed_value.find_last_not_of(" \t\n\r");
            if (start != std::string::npos) {
                trimmed_value = trimmed_value.substr(start, end - start + 1);
            }
            
            // Update the cell with trimmed value if different
            if (trimmed_value != value) {
                data.rows[row_idx][col_idx] = trimmed_value;
                logger->log_cleaned(row_idx, "Disability type", value, trimmed_value);
            }
            
            // Validate characters (allow letters, spaces, hyphens, periods)
            if (!std::all_of(trimmed_value.begin(), trimmed_value.end(), [](char c) {
                return std::isalpha(static_cast<unsigned char>(c)) || 
                    std::isspace(static_cast<unsigned char>(c)) || 
                    c == '-' || c == '.';
            })) {
                addError(row_idx, col_idx, "Disability type contains invalid characters (only letters, spaces, hyphens, and periods allowed)");
                logger->log_warning(row_idx, "Disability type invalid chars: " + trimmed_value);
            }
        }
    }

    void validateCrossFieldRules(size_t row_idx) {
        auto& row = data.rows[row_idx];
        
        // Find field indices
        int credits_idx = -1, reentry_idx = -1, avg_curr_idx = -1, avg_gen_idx = -1;
        
        for (size_t j = 0; j < data.headers.size(); j++) {
            if (data.headers[j] == "cac") credits_idx = j;
            else if (data.headers[j] == "reingreso") reentry_idx = j;
            else if (data.headers[j] == "psa1") avg_curr_idx = j;
            else if (data.headers[j] == "pge") avg_gen_idx = j;
        }
        
        // Validate new entry students (reingreso = "N")
        if (reentry_idx >= 0 && credits_idx >= 0 && avg_curr_idx >= 0 && avg_gen_idx >= 0) {
            if (row[reentry_idx] == "N") {
                // New students should have 0 credits and 0 averages
                if (row[credits_idx] != "0") {
                    addError(row_idx, credits_idx, "New students should have 0 accumulated credits");
                    logger->log_warning(row_idx, "New student credits not 0");
                }
                if (row[avg_curr_idx] != "0.00") {
                    addError(row_idx, avg_curr_idx, "New students should have 0.00 current average");
                    logger->log_warning(row_idx, "New student current avg not 0.00");
                }
                if (row[avg_gen_idx] != "0.00") {
                    addError(row_idx, avg_gen_idx, "New students should have 0.00 general average");
                    logger->log_warning(row_idx, "New student general avg not 0.00");
                }
            }
        }
        
        // Validate at least one last name exists
        int paterno_idx = -1, materno_idx = -1;
        for (size_t j = 0; j < data.headers.size(); j++) {
            if (data.headers[j] == "app") paterno_idx = j;
            else if (data.headers[j] == "apm") materno_idx = j;
        }
        
        if (paterno_idx >= 0 && materno_idx >= 0) {
            if (row[paterno_idx].empty() && row[materno_idx].empty()) {
                addError(row_idx, paterno_idx, "At least one last name (paternal or maternal) is required");
                logger->log_warning(row_idx, "Both last names empty");
            }
        }
    }

    void addError(size_t row_idx, size_t col_idx, const std::string& error_msg) {
        if (row_idx < data.validation_errors.size() && col_idx < data.validation_errors[row_idx].size()) {
            if (!data.validation_errors[row_idx][col_idx].empty()) {
                data.validation_errors[row_idx][col_idx] += "; ";
            }
            data.validation_errors[row_idx][col_idx] += error_msg;
        }
    }

    void printValidationSummary() {
        int total_errors = 0;
        int valid_rows = 0;
        
        for (size_t i = 0; i < data.validation_errors.size(); ++i) {
            bool row_has_errors = false;
            for (const auto& error : data.validation_errors[i]) {
                if (!error.empty()) {
                    total_errors++;
                    row_has_errors = true;
                }
            }
            if (!row_has_errors) {
                valid_rows++;
            }
        }
        
        logger->log_summary("=== VALIDATION SUMMARY ===");
        logger->log_summary("Total records processed: " + std::to_string(data.rows.size()));
        logger->log_summary("Valid records: " + std::to_string(valid_rows));
        logger->log_summary("Records with errors: " + std::to_string(data.rows.size() - valid_rows));
        logger->log_summary("Total validation errors: " + std::to_string(total_errors));
        
        // Count errors by field type
        std::map<std::string, int> error_counts;
        for (size_t i = 0; i < data.validation_errors.size(); ++i) {
            for (size_t j = 0; j < data.validation_errors[i].size() && j < data.headers.size(); ++j) {
                if (!data.validation_errors[i][j].empty()) {
                    error_counts[data.headers[j]]++;
                }
            }
        }
        
        logger->log_summary("Errors by field:");
        for (const auto& [field, count] : error_counts) {
            logger->log_summary("  " + field + ": " + std::to_string(count) + " errors");
        }
        logger->log_summary("==========================");
    }

    void replaceTextPattern(const std::string& oldPattern, const std::string& newPattern) {
        int replacements = 0;
        for (auto& row : data.rows) {
            for (auto& cell : row) {
                size_t pos = 0;
                while ((pos = cell.find(oldPattern, pos)) != std::string::npos) {
                    cell.replace(pos, oldPattern.length(), newPattern);
                    pos += newPattern.length();
                    replacements++;
                }
            }
        }
        if (replacements > 0) {
            logger->log_info("Text replacement: " + std::to_string(replacements) + " occurrences replaced");
        }
    }

    void transformTextCase(const std::string& caseType) {
        logger->log_info("Applying case transformation: " + caseType);
        for (auto& row : data.rows) {
            for (auto& cell : row) {
                if (caseType == "uppercase") {
                    std::transform(cell.begin(), cell.end(), cell.begin(), ::toupper);
                } else if (caseType == "lowercase") {
                    std::transform(cell.begin(), cell.end(), cell.begin(), ::tolower);
                } else if (caseType == "title_case") {
                    bool newWord = true;
                    for (char& c : cell) {
                        if (std::isspace(c)) {
                            newWord = true;
                        } else if (newWord) {
                            c = std::toupper(c);
                            newWord = false;
                        } else {
                            c = std::tolower(c);
                        }
                    }
                }
            }
        }
    }
};

bool saveValidationErrors(const DataProcessor& processor, const std::string& errorsFile, std::shared_ptr<LogManager> logger) {
    std::ofstream file(errorsFile);
    if (!file.is_open()) {
        logger->log_error("Cannot create errors file " + errorsFile);
        return false;
    }

    auto headers = processor.getHeaders();
    auto errors = processor.getValidationErrors();

    file << "{\n";
    file << "  \"headers\": [";
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) file << ", ";
        file << "\"" << headers[i] << "\"";
    }
    file << "],\n";
    
    file << "  \"errors\": [\n";
    for (size_t i = 0; i < errors.size(); ++i) {
        file << "    [";
        for (size_t j = 0; j < errors[i].size(); ++j) {
            if (j > 0) file << ", ";
            // Escape quotes in error messages
            std::string escaped_error = errors[i][j];
            size_t pos = 0;
            while ((pos = escaped_error.find('"', pos)) != std::string::npos) {
                escaped_error.replace(pos, 1, "\\\"");
                pos += 2;
            }
            file << "\"" << escaped_error << "\"";
        }
        file << "]";
        if (i < errors.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n";
    file << "}\n";

    file.close();
    logger->log_info("Validation errors saved to " + errorsFile);
    return true;
}

// Function to parse command line arguments
std::map<std::string, std::string> parseArguments(int argc, char* argv[]) {
    std::map<std::string, std::string> options;
    
    for (int i = 3; i < argc; i += 2) {
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            std::string key = argv[i] + 2;
            if (i + 1 < argc) {
                options[key] = argv[i + 1];
            }
        }
    }
    
    return options;
}

int main(int argc, char* argv[]) {
    // Check for exactly 3 arguments
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_csv> <valid_output> <process_log>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Arguments:" << std::endl;
        std::cerr << "  <input_csv>     Input CSV file to process" << std::endl;
        std::cerr << "  <valid_output>  Output CSV file for valid records" << std::endl;
        std::cerr << "  <process_log>   Log file for processing details" << std::endl;
        return 1;
    }

    // Initialize log manager
    auto logger = std::make_shared<LogManager>();
    if (!logger->initialize(argv[3])) {
        std::cerr << "ERROR: Failed to initialize log file" << std::endl;
        return 1;
    }

    logger->log_info("=== DATA PROCESSOR STARTED ===");
    logger->log_info("Input file: " + std::string(argv[1]));
    logger->log_info("Valid output: " + std::string(argv[2]));
    logger->log_info("Process log: " + std::string(argv[3]));

    // No options - create empty options map
    std::map<std::string, std::string> options;

    DataProcessor processor(options, logger);
    
    if (!processor.loadData(argv[1])) {
        logger->log_error("Failed to load data from " + std::string(argv[1]));
        return 1;
    }

    logger->log_info("Processing data with comprehensive validation...");
    processor.processData();

    // Save only valid records
    if (!processor.saveData(argv[2])) {
        logger->log_error("Failed to save valid records to " + std::string(argv[2]));
        return 1;
    }

    // Print final summary
    auto problematic_count = processor.getProblematicRows().size();
    auto total_records = processor.getValidationErrors().size();
    auto valid_count = total_records - problematic_count;
    
    logger->log_summary("=== PROCESSING SUMMARY ===");
    logger->log_summary("Total records: " + std::to_string(total_records));
    logger->log_summary("Valid records saved: " + std::to_string(valid_count));
    logger->log_summary("Problematic records filtered: " + std::to_string(problematic_count));
    logger->log_summary("Output file: " + std::string(argv[2]));
    logger->log_summary("==========================");
    
    logger->log_info("=== DATA PROCESSOR FINISHED ===");
    logger->close();

    return 0;
}