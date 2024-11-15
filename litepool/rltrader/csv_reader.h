#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <iostream>

namespace Simulator {
    struct DataRow {
        DataRow(long long anId, std::unordered_map<std::string, double>& dataMap) {
            this->id = anId;
            this->data = dataMap;
        }

        long long id = 0;
        std::unordered_map<std::string, double> data;

        double getBestBidPrice() const {
            return data.find("bids[0].price")->second;
        }

        double getBestAskPrice() const {
            return data.find("asks[0].price")->second;
        }
    };

    class CsvReader {
    private:
        class Iterator {
        private:
            const std::vector<DataRow>* rowsPtr;
            size_t current = -1;

        public:
            Iterator():rowsPtr(0), current(0) {}

            void populate(std::vector<DataRow>* ptr) {
                rowsPtr = ptr;
                current = -1;
            }

            bool hasNext() const {
                return (current + 1) < rowsPtr->size();
            }

            const DataRow& next() {
                if (!hasNext()) {
                    throw std::out_of_range("No more elements");
                }
                return (*rowsPtr)[++current];
            }

            long long getTimeStamp() const {
                return (*rowsPtr)[current].id;
            }

            double getDouble(const std::string& key) const {
                return (*rowsPtr)[current].data.at(key);
            }

            const DataRow& currentRow() const {
                if (current < 0) throw std::runtime_error("Invalid current row");
                return (*rowsPtr)[current];
            }

            void reset() {
                this->current = 0;
            }
        };

        std::ifstream filestream;
        std::string foldername;
        Iterator iterator;
        std::vector<std::string> headers;
        std::vector<DataRow> rows;
        static std::vector<double> parseLineToDoubles(const std::string& line);
        void readCSV(int start_line);
        bool more_data;
        int start_read;
        int max_read;
        int num_reads;
    public:
        CsvReader(const std::string& foldername, int start_read, int max_read);
        bool hasNext();
        const DataRow& next();
        const DataRow& current() const;
        long long getTimeStamp() const;
        double getDouble(const std::string& keyname) const;
        void reset();
    };
}
