#include <iostream>
#include <curl/curl.h>
#include <string>
#include <stack>
#include <vector>
#include <sstream>
#include <map>
#include <cmath>
#include <fstream>
#include <random>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <thread>

// Write callback function for libcurl
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

// Function to fetch data from API
std::string fetchData(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        }
    }
    return readBuffer;
}

// Parse CSV data to extract prices
std::vector<double> parsePrices(const std::string& data) {
    std::vector<double> prices;
    std::istringstream iss(data);
    std::string line;

    // Skip header
    std::getline(iss, line);

    while (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        std::string date, open, high, low, close, adjustedClose, volume, dividend, splitCoefficient;
        
        std::getline(lineStream, date, ',');
        std::getline(lineStream, open, ',');
        std::getline(lineStream, high, ',');
        std::getline(lineStream, low, ',');
        std::getline(lineStream, close, ',');
        std::getline(lineStream, adjustedClose, ',');
        
        try {
            prices.push_back(std::stod(adjustedClose));
        } catch (const std::exception& e) {
            std::cerr << "Error converting price: " << adjustedClose << " - " << e.what() << std::endl;
        }
    }

    // Reverse to get chronological order
    std::reverse(prices.begin(), prices.end());
    return prices;
}

// Stock class definition
class Stock {
public:
    std::string symbol;
    double epsEstimate;
    double actualEPS;
    std::string earningsDate;
    std::vector<double> prices;
    std::vector<double> returns;
    std::vector<double> abnormalReturns;
    
    // Constructor
    Stock(const std::string& sym, double eps, double actual, const std::string& date) 
        : symbol(sym), epsEstimate(eps), actualEPS(actual), earningsDate(date) {}
    
    // Add a price
    void addPrice(double price) {
        prices.push_back(price);
    }
    
    // Calculate daily returns
    void calculateReturns() {
        returns.clear();
        for (size_t i = 1; i < prices.size(); i++) {
            double ret = (prices[i] - prices[i-1]) / prices[i-1];
            returns.push_back(ret);
        }
    }
    
    // Calculate abnormal returns against market (SPY)
    void calculateAbnormalReturns(const std::vector<double>& marketReturns) {
        abnormalReturns.clear();
        for (size_t i = 0; i < returns.size() && i < marketReturns.size(); i++) {
            abnormalReturns.push_back(returns[i] - marketReturns[i]);
        }
    }
    
    // Get surprise percentage
    double getSurprisePercentage() const {
        if (epsEstimate != 0) {
            return (actualEPS - epsEstimate) / std::abs(epsEstimate) * 100.0;
        }
        return 0.0;
    }
    
    // Determine group (Beat, Meet, Miss)
    std::string getGroup() const {
        double surprise = getSurprisePercentage();
        if (surprise > 5.0) return "Beat";
        else if (surprise < -5.0) return "Miss";
        else return "Meet";
    }
};

// Group class to store collections of stocks
class Group {
public:
    std::string name;
    std::vector<Stock*> stocks;
    std::vector<double> aar; // Average Abnormal Return
    std::vector<double> caar; // Cumulative Average Abnormal Return
    
    Group(const std::string& groupName) : name(groupName) {}
    
    void addStock(Stock* stock) {
        stocks.push_back(stock);
    }
    
    // Calculate AAR for the group
    void calculateAAR() {
        if (stocks.empty()) return;
        
        // Initialize AAR vector with zeros
        int daysCount = stocks[0]->abnormalReturns.size();
        aar.resize(daysCount, 0.0);
        
        // Sum abnormal returns for each day
        for (Stock* stock : stocks) {
            for (size_t day = 0; day < daysCount && day < stock->abnormalReturns.size(); day++) {
                aar[day] += stock->abnormalReturns[day];
            }
        }
        
        // Calculate average
        for (size_t day = 0; day < daysCount; day++) {
            aar[day] /= stocks.size();
        }
    }
    
    // Calculate CAAR for the group
    void calculateCAAR() {
        caar.clear();
        double cumulative = 0.0;
        
        for (double dailyAAR : aar) {
            cumulative += dailyAAR;
            caar.push_back(cumulative);
        }
    }
    
    // Sample stocks for bootstrapping
    std::vector<Stock*> sampleStocks(int sampleSize) {
        std::vector<Stock*> sample;
        if (stocks.size() <= sampleSize) {
            return stocks; // Return all if we don't have enough
        }
        
        // Create a copy to shuffle
        std::vector<Stock*> stocksCopy = stocks;
        
        // Use a random device for true randomness
        std::random_device rd;
        std::mt19937 g(rd());
        
        // Shuffle and take the first sampleSize elements
        std::shuffle(stocksCopy.begin(), stocksCopy.end(), g);
        sample.assign(stocksCopy.begin(), stocksCopy.begin() + sampleSize);
        
        return sample;
    }
};

// MarketData class to handle market data
class MarketData {
private:
    std::string apiKey;
    
public:
    MarketData(const std::string& key) : apiKey(key) {}
    
    // Fetch historical data for a symbol and date range
    std::vector<double> fetchHistoricalData(const std::string& symbol, const std::string& startDate, const std::string& endDate) {
        std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY_ADJUSTED"
                         "&symbol=" + symbol + 
                         "&apikey=" + apiKey + 
                         "&datatype=csv";
        
        std::string data = fetchData(url);
        return parsePrices(data);
    }
    
    // Calculate market returns (SPY)
    std::vector<double> calculateMarketReturns(const std::vector<double>& marketPrices) {
        std::vector<double> returns;
        for (size_t i = 1; i < marketPrices.size(); i++) {
            double ret = (marketPrices[i] - marketPrices[i-1]) / marketPrices[i-1];
            returns.push_back(ret);
        }
        return returns;
    }
};

// StockAnalyzer class to handle the analysis process
class StockAnalyzer {
private:
    MarketData marketData;
    std::map<std::string, Stock> stocksMap;
    Group beatGroup;
    Group meetGroup;
    Group missGroup;
    std::vector<double> marketPrices;
    std::vector<double> marketReturns;
    
public:
    StockAnalyzer(const std::string& apiKey) 
        : marketData(apiKey), beatGroup("Beat"), meetGroup("Meet"), missGroup("Miss") {}
    
    // Load stock data from a file
    void loadStockDataFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        // Skip header
        std::getline(file, line);
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string symbol, date;
            double epsEstimate, actualEPS;
            
            std::getline(iss, symbol, ',');
            iss >> epsEstimate;
            iss.ignore(); // Skip comma
            iss >> actualEPS;
            iss.ignore(); // Skip comma
            std::getline(iss, date);
            
            stocksMap.emplace(symbol, Stock(symbol, epsEstimate, actualEPS, date));
        }
        
        file.close();
    }
    
    // Retrieve historical data for all stocks
    void retrieveHistoricalData() {
        // First, retrieve market data (SPY)
        std::cout << "Retrieving market data (SPY)...\n";
        marketPrices = marketData.fetchHistoricalData("SPY", "", "");
        marketReturns = marketData.calculateMarketReturns(marketPrices);
        
        // Then retrieve data for each stock
        int count = 0;
        for (auto& pair : stocksMap) {
            std::cout << "Retrieving data for " << pair.first << " (" << ++count << "/" << stocksMap.size() << ")\n";
            Stock& stock = pair.second;
            
            // Fetch 61 days around earnings date (30 before, earnings day, 30 after)
            std::vector<double> prices = marketData.fetchHistoricalData(stock.symbol, "", "");
            
            // Add prices to stock
            stock.prices = prices;
            
            // Calculate returns
            stock.calculateReturns();
            
            // Calculate abnormal returns
            stock.calculateAbnormalReturns(marketReturns);
            
            // Categorize stock based on EPS surprise
            if (stock.getGroup() == "Beat") {
                beatGroup.addStock(&stock);
            } else if (stock.getGroup() == "Meet") {
                meetGroup.addStock(&stock);
            } else {
                missGroup.addStock(&stock);
            }
            
            // Add delay to avoid API rate limiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        
        // Calculate AAR and CAAR for each group
        calculateGroupMetrics();
    }
    
    // Calculate AAR and CAAR for all groups
    void calculateGroupMetrics() {
        beatGroup.calculateAAR();
        beatGroup.calculateCAAR();
        
        meetGroup.calculateAAR();
        meetGroup.calculateCAAR();
        
        missGroup.calculateAAR();
        missGroup.calculateCAAR();
    }
    
    // Get stock by symbol
    Stock* getStock(const std::string& symbol) {
        auto it = stocksMap.find(symbol);
        if (it != stocksMap.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    
    // Export CAAR data to CSV for visualization
    void exportCAARtoCSV(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return;
        }
        
        // Write header
        file << "Day,Beat,Meet,Miss\n";
        
        // Write data
        size_t maxDays = std::max({beatGroup.caar.size(), meetGroup.caar.size(), missGroup.caar.size()});
        
        for (size_t day = 0; day < maxDays; day++) {
            file << (day - 30) << ","; // Day relative to earnings announcement
            
            if (day < beatGroup.caar.size()) {
                file << beatGroup.caar[day];
            }
            file << ",";
            
            if (day < meetGroup.caar.size()) {
                file << meetGroup.caar[day];
            }
            file << ",";
            
            if (day < missGroup.caar.size()) {
                file << missGroup.caar[day];
            }
            file << "\n";
        }
        
        file.close();
        std::cout << "CAAR data exported to " << filename << std::endl;
    }
    
    // Perform bootstrapping
    void performBootstrapping(int sampleSize, int iterations) {
        std::vector<std::vector<double>> beatCAARs(iterations);
        std::vector<std::vector<double>> meetCAARs(iterations);
        std::vector<std::vector<double>> missCAARs(iterations);
        
        for (int i = 0; i < iterations; i++) {
            std::cout << "Bootstrapping iteration " << (i+1) << "/" << iterations << std::endl;
            
            // Sample stocks from each group
            std::vector<Stock*> beatSample = beatGroup.sampleStocks(sampleSize);
            std::vector<Stock*> meetSample = meetGroup.sampleStocks(sampleSize);
            std::vector<Stock*> missSample = missGroup.sampleStocks(sampleSize);
            
            // Create temporary groups with the samples
            Group tempBeatGroup("TempBeat");
            Group tempMeetGroup("TempMeet");
            Group tempMissGroup("TempMiss");
            
            for (Stock* stock : beatSample) tempBeatGroup.addStock(stock);
            for (Stock* stock : meetSample) tempMeetGroup.addStock(stock);
            for (Stock* stock : missSample) tempMissGroup.addStock(stock);
            
            // Calculate AAR and CAAR for temporary groups
            tempBeatGroup.calculateAAR();
            tempBeatGroup.calculateCAAR();
            tempMeetGroup.calculateAAR();
            tempMeetGroup.calculateCAAR();
            tempMissGroup.calculateAAR();
            tempMissGroup.calculateCAAR();
            
            // Store CAAR values
            beatCAARs[i] = tempBeatGroup.caar;
            meetCAARs[i] = tempMeetGroup.caar;
            missCAARs[i] = tempMissGroup.caar;
        }
        
        // Calculate average CAAR across bootstrapping iterations
        std::vector<double> avgBeatCAAR;
        std::vector<double> avgMeetCAAR;
        std::vector<double> avgMissCAAR;
        
        // Find the minimum size across all iterations
        size_t minBeatSize = std::numeric_limits<size_t>::max();
        size_t minMeetSize = std::numeric_limits<size_t>::max();
        size_t minMissSize = std::numeric_limits<size_t>::max();
        
        for (const auto& caar : beatCAARs) minBeatSize = std::min(minBeatSize, caar.size());
        for (const auto& caar : meetCAARs) minMeetSize = std::min(minMeetSize, caar.size());
        for (const auto& caar : missCAARs) minMissSize = std::min(minMissSize, caar.size());
        
        // Initialize average vectors
        avgBeatCAAR.resize(minBeatSize, 0.0);
        avgMeetCAAR.resize(minMeetSize, 0.0);
        avgMissCAAR.resize(minMissSize, 0.0);
        
        // Sum across iterations
        for (int i = 0; i < iterations; i++) {
            for (size_t day = 0; day < minBeatSize; day++) {
                avgBeatCAAR[day] += beatCAARs[i][day];
            }
            for (size_t day = 0; day < minMeetSize; day++) {
                avgMeetCAAR[day] += meetCAARs[i][day];
            }
            for (size_t day = 0; day < minMissSize; day++) {
                avgMissCAAR[day] += missCAARs[i][day];
            }
        }
        
        // Calculate averages
        for (size_t day = 0; day < minBeatSize; day++) {
            avgBeatCAAR[day] /= iterations;
        }
        for (size_t day = 0; day < minMeetSize; day++) {
            avgMeetCAAR[day] /= iterations;
        }
        for (size_t day = 0; day < minMissSize; day++) {
            avgMissCAAR[day] /= iterations;
        }
        
        // Export bootstrapped results
        std::ofstream file("bootstrapped_caar.csv");
        if (file.is_open()) {
            file << "Day,Beat,Meet,Miss\n";
            
            size_t maxDays = std::max({minBeatSize, minMeetSize, minMissSize});
            
            for (size_t day = 0; day < maxDays; day++) {
                file << (day - 30) << ","; // Day relative to earnings announcement
                
                if (day < minBeatSize) {
                    file << avgBeatCAAR[day];
                }
                file << ",";
                
                if (day < minMeetSize) {
                    file << avgMeetCAAR[day];
                }
                file << ",";
                
                if (day < minMissSize) {
                    file << avgMissCAAR[day];
                }
                file << "\n";
            }
            
            file.close();
            std::cout << "Bootstrapped CAAR data exported to bootstrapped_caar.csv" << std::endl;
        }
    }
    
    // Display information about a specific stock
    void displayStockInfo(const std::string& symbol) {
        Stock* stock = getStock(symbol);
        if (!stock) {
            std::cout << "Stock " << symbol << " not found.\n";
            return;
        }
        
        std::cout << "===== Stock Information =====\n";
        std::cout << "Symbol: " << stock->symbol << "\n";
        std::cout << "EPS Estimate: " << stock->epsEstimate << "\n";
        std::cout << "Actual EPS: " << stock->actualEPS << "\n";
        std::cout << "Surprise %: " << stock->getSurprisePercentage() << "%\n";
        std::cout << "Group: " << stock->getGroup() << "\n";
        std::cout << "Earnings Date: " << stock->earningsDate << "\n";
        std::cout << "\nPrices around earnings date:\n";
        
        // Display a subset of prices
        int midPoint = stock->prices.size() / 2;
        int start = std::max(0, midPoint - 5);
        int end = std::min(static_cast<int>(stock->prices.size()) - 1, midPoint + 5);
        
        for (int i = start; i <= end; i++) {
            std::cout << "Day " << (i - midPoint) << ": $" << std::fixed << std::setprecision(2) << stock->prices[i] << "\n";
        }
        
        std::cout << "\nAbnormal Returns around earnings date:\n";
        start = std::max(0, midPoint - 5 - 1); // -1 because returns have one less element than prices
        end = std::min(static_cast<int>(stock->abnormalReturns.size()) - 1, midPoint + 5 - 1);
        
        for (int i = start; i <= end; i++) {
            std::cout << "Day " << (i - (midPoint-1)) << ": " 
                     << std::fixed << std::setprecision(4) << stock->abnormalReturns[i] * 100 << "%\n";
        }
    }
    
    // Display AAR or CAAR for a group
    void displayGroupMetrics(const std::string& groupName, bool showCAAR) {
        Group* group = nullptr;
        
        if (groupName == "Beat") group = &beatGroup;
        else if (groupName == "Meet") group = &meetGroup;
        else if (groupName == "Miss") group = &missGroup;
        
        if (!group) {
            std::cout << "Invalid group name. Please choose Beat, Meet, or Miss.\n";
            return;
        }
        
        std::cout << "===== " << groupName << " Group " << (showCAAR ? "CAAR" : "AAR") << " =====\n";
        std::cout << "Number of stocks: " << group->stocks.size() << "\n\n";
        
        const std::vector<double>& data = showCAAR ? group->caar : group->aar;
        
        std::cout << "Day\t" << (showCAAR ? "CAAR" : "AAR") << "\n";
        for (size_t i = 0; i < data.size(); i++) {
            std::cout << (i - 30) << "\t" << std::fixed << std::setprecision(6) << data[i] * 100 << "%\n";
        }
    }
    
    // Run the analysis process
    void runAnalysis() {
        // Menu implementation
        int choice;
        std::string apiKey, stocksFile;
        bool dataLoaded = false;
        
        std::cout << "===== Stock Performance Analysis =====\n";
        std::cout << "Enter your Alpha Vantage API key: ";
        std::getline(std::cin, apiKey);
        
        MarketData md(apiKey);
        
        do {
            std::cout << "\nMenu:\n";
            std::cout << "1. Load stocks from file\n";
            std::cout << "2. Retrieve historical price data for all stocks\n";
            std::cout << "3. Pull information for one stock\n";
            std::cout << "4. Show AAR for one group\n";
            std::cout << "5. Show CAAR for one group\n";
            std::cout << "6. Export CAAR data to CSV\n";
            std::cout << "7. Perform bootstrapping\n";
            std::cout << "8. Exit\n";
            std::cout << "Enter your choice: ";
            std::cin >> choice;
            std::cin.ignore(); // Clear the newline
            
            switch (choice) {
                case 1: {
                    std::cout << "Enter stocks file path: ";
                    std::getline(std::cin, stocksFile);
                    loadStockDataFromFile(stocksFile);
                    std::cout << "Loaded " << stocksMap.size() << " stocks.\n";
                    dataLoaded = true;
                    break;
                }
                case 2: {
                    if (!dataLoaded) {
                        std::cout << "Please load stocks from file first (option 1).\n";
                        break;
                    }
                    retrieveHistoricalData();
                    break;
                }
                case 3: {
                    if (!dataLoaded) {
                        std::cout << "Please load stocks from file first (option 1).\n";
                        break;
                    }
                    std::string symbol;
                    std::cout << "Enter stock symbol: ";
                    std::getline(std::cin, symbol);
                    displayStockInfo(symbol);
                    break;
                }
                case 4: {
                    if (!dataLoaded) {
                        std::cout << "Please load stocks from file first (option 1).\n";
                        break;
                    }
                    std::string group;
                    std::cout << "Enter group (Beat, Meet, Miss): ";
                    std::getline(std::cin, group);
                    displayGroupMetrics(group, false); // Show AAR
                    break;
                }
                case 5: {
                    if (!dataLoaded) {
                        std::cout << "Please load stocks from file first (option 1).\n";
                        break;
                    }
                    std::string group;
                    std::cout << "Enter group (Beat, Meet, Miss): ";
                    std::getline(std::cin, group);
                    displayGroupMetrics(group, true); // Show CAAR
                    break;
                }
                case 6: {
                    if (!dataLoaded) {
                        std::cout << "Please load stocks from file first (option 1).\n";
                        break;
                    }
                    std::string filename;
                    std::cout << "Enter output filename (e.g., caar_data.csv): ";
                    std::getline(std::cin, filename);
                    exportCAARtoCSV(filename);
                    break;
                }
                case 7: {
                    if (!dataLoaded) {
                        std::cout << "Please load stocks from file first (option 1).\n";
                        break;
                    }
                    int sampleSize, iterations;
                    std::cout << "Enter sample size: ";
                    std::cin >> sampleSize;
                    std::cout << "Enter number of iterations: ";
                    std::cin >> iterations;
                    std::cin.ignore(); // Clear the newline
                    performBootstrapping(sampleSize, iterations);
                    break;
                }
                case 8:
                    std::cout << "Exiting...\n";
                    break;
                default:
                    std::cout << "Invalid choice. Try again.\n";
            }
        } while (choice != 8);
    }
};

int main() {
    // Initialize CURL globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Create and run the analyzer
    StockAnalyzer analyzer("");
    analyzer.runAnalysis();
    
    // Cleanup CURL
    curl_global_cleanup();
    
    return 0;
}
