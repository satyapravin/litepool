#include <sstream>
#include "csv_reader.h"


using namespace Simulator;

CsvReader::CsvReader(const std::string& filename):filestream(filename), more_data(true) {
    this->readCSV();
    this->iterator.populate(&rows);
}

bool CsvReader::hasNext() {
    bool retval = this->iterator.hasNext();
    if (!retval) {
        if (more_data) {
            this->readCSV();
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

void CsvReader::reset(int counter) {
    this->headers.clear();
    this->iterator.reset(0);
    this->readCSV();
    this->iterator.populate(&rows);
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


void CsvReader::readCSV() {
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
    }

    int num_lines = 0;
    rows.clear();
    while (std::getline(filestream, line)) {
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
        if (++num_lines >= 360) {
            batch_read = true;
            break;
        }
    }

    if (!batch_read) {
        more_data = false;
        filestream.close();
    }
}
