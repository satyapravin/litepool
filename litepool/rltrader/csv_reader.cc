#include <sstream>
#include <random>
#include <filesystem>
#include <stdexcept>
#include <system_error> 

#include "csv_reader.h"
namespace fs = std::filesystem;
using namespace Simulator;

CsvReader::CsvReader(const std::string& fname, int start_read_lines, int max_read_lines):foldername(fname),
                                                                                         more_data(true), start_read(start_read_lines),
                                                                                         max_read(max_read_lines), num_reads(0) {
}

bool CsvReader::hasNext() {
    bool retval = this->iterator.hasNext();
    if (!retval) {
        if (more_data) {
            this->readCSV(0);
            this->iterator.populate(&rows);
            return this->iterator.hasNext();
        }
        else {
            return more_data;
        }
    }

    return retval;
}

const DataRow& CsvReader::next() {
    return this->iterator.next();
}

const DataRow& CsvReader::current() const {
    return this->iterator.currentRow();
}

long long CsvReader::getTimeStamp() const {
    return this->iterator.getTimeStamp();
}

double CsvReader::getDouble(const std::string& keyName) const {
    return this->iterator.getDouble(keyName);
}

std::vector<std::string> read_files(const std::string& foldername) {
    std::vector<std::string> files;

    try {
        // Check if the folder exists and is a directory
        if (!fs::exists(foldername) || !fs::is_directory(foldername)) {
            std::cerr << "Error: " << foldername << " is not a valid directory." << std::endl;
            return files; // Return an empty vector
        }

        // Iterate through directory entries
        for (const auto& entry : fs::directory_iterator(foldername)) {
            if (entry.is_regular_file()) {
                std::string filepath = entry.path().string();
                std::cout << filepath << std::endl;  // Print the file path
                files.push_back(filepath);           // Add the file path to the vector
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // Handle any filesystem-related exceptions
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // Handle any other exceptions
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return files;
}

void CsvReader::reset() {
    num_reads = 0;
    this->headers.clear();
    this->iterator.reset();
    more_data = true;
    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> distr(0, start_read);
    int start_line = distr(gen);
    auto files = read_files(this->foldername);
    if (this->filestream.is_open()) this->filestream.close();
    this->filestream.open(files[start_line % files.size()], std::ios::in);
    this->readCSV(start_line);
    this->iterator.populate(&rows);
}

std::vector<double> CsvReader::parseLineToDoubles(const std::string& line) {
    std::istringstream stream(line);
    std::string cell;
    std::vector<double> results;
    std::getline(stream, cell, ','); // Skip the first token (ID)

    while (std::getline(stream, cell, ',')) {
        results.push_back(std::stod(cell));
    }
    return results;
}


void CsvReader::readCSV(int start_line) {
    if (!filestream.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    std::string line;
    bool batch_read = false;
    if (this->headers.empty()) {
        filestream.clear();
        filestream.seekg(0, std::ios::beg);
        std::getline(filestream, line); // Read the header line
        std::istringstream headerStream(line);
        std::string header;
        std::getline(headerStream, header, ','); // Skip the first header as it is the ID
        while (std::getline(headerStream, header, ',')) {
            headers.push_back(header);
        }

        for(int linenum=0; linenum < start_line; ++linenum) {
            std::getline(filestream, line); 
        }
    }

    int num_lines = 0;
    rows.clear();
    while (std::getline(filestream, line)) {
        ++num_reads;
        std::istringstream lineStream(line);
        std::string cell;
        std::getline(lineStream, cell, ','); // Read the ID
        long long id = std::stoll(cell);

        std::vector<double> values = parseLineToDoubles(line);
        std::unordered_map<std::string, double> data;
        for (size_t i = 0; i < values.size(); ++i) {
            data[headers[i]] = values[i];
        }

        DataRow row(id, data);
        this->rows.push_back(row);
        if (++num_lines >= 2500) {
            batch_read = true;
            break;
        }

        if (num_reads > max_read) {
            break;
        }
    }

    if (!batch_read) {
        more_data = false;
        filestream.close();
    }
}
