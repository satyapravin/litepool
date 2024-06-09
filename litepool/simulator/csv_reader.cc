#include <fstream>
#include <sstream>
#include <map>
#include "csv_reader.h"

using namespace Simulator;

CsvReader::CsvReader(const std::string& filename) {
    this->readCSV(filename);
    this->iterator.populate(&rows);
}

bool CsvReader::hasNext() const {
    return this->iterator.hasNext();
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

void CsvReader::reset(int counter) {
    return this->iterator.reset(counter);
}

std::vector<double> CsvReader::parseLineToDoubles(const std::string& line) {
    std::istringstream stream(line);
    std::string cell;
    std::vector<double> results;
    std::getline(stream, cell, ','); // Skip the first token (ID)

    while (std::getline(stream, cell, ',')) {
        try {
            results.push_back(std::stod(cell));
        }
        catch (const std::exception& e) {
            throw e;
        }
    }
    return results;
}


void CsvReader::readCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    std::string line;
    std::getline(file, line); // Read the header line
    std::istringstream headerStream(line);
    std::string header;
    std::vector<std::string> headers;
    std::getline(headerStream, header, ','); // Skip the first header as it is the ID
    while (std::getline(headerStream, header, ',')) {
        headers.push_back(header);
    }

    while (std::getline(file, line)) {
        std::istringstream lineStream(line);
        std::string cell;
        std::getline(lineStream, cell, ','); // Read the ID
        long long id = std::stoll(cell);

        std::vector<double> values = parseLineToDoubles(line);
        std::map<std::string, double> data;
        for (size_t i = 0; i < values.size(); ++i) {
            data[headers[i]] = values[i];
        }

        DataRow row(id, data);
        this->rows.push_back(row);
    }

    file.close();
}