# Stock Performance Analysis Application

This application analyzes the performance of stocks around earnings announcements, with a focus on comparing stocks that beat, meet, or miss earnings expectations.

## Overview

The Stock Performance Analysis Application is a C++ program designed to help financial analysts and researchers study the relationship between earnings surprises and stock price performance. The application retrieves historical stock data, categorizes stocks based on their earnings performance relative to analyst estimates, and calculates metrics to quantify their abnormal returns around earnings announcements.

## Features

- **Data Retrieval**: Fetches historical stock price data from Alpha Vantage API
- **Earnings Categorization**: Groups stocks into "Beat," "Meet," or "Miss" categories based on EPS surprise
- **Financial Metrics**: Calculates daily returns, abnormal returns, Average Abnormal Returns (AAR), and Cumulative Average Abnormal Returns (CAAR)
- **Statistical Analysis**: Implements bootstrapping to generate more robust results
- **Data Export**: Exports results to CSV for visualization in Excel or other tools
- **Interactive Interface**: Provides a menu-driven UI for easy operation

## Requirements

- C++ compiler with C++11 support
- libcurl library
- Alpha Vantage API key (free tier available)
- Input CSV file with earnings data

## Installation

1. Clone this repository:
   ```
   git clone https://github.com/yourusername/stock-performance-analysis.git
   cd stock-performance-analysis
   ```

2. Install libcurl if not already installed:
   ```
   # Ubuntu/Debian
   sudo apt-get install libcurl4-openssl-dev
   
   # macOS (with Homebrew)
   brew install curl
   
   # Windows (with vcpkg)
   vcpkg install curl
   ```

3. Compile the application:
   ```
   g++ -o stock_analyzer main.cpp -lcurl -std=c++11
   ```

## Usage

1. Run the compiled executable:
   ```
   ./stock_analyzer
   ```

2. Enter your Alpha Vantage API key when prompted

3. Follow the menu options:
   - Option 1: Load stock data from a CSV file
   - Option 2: Retrieve historical price data for all stocks
   - Option 3: View information for a specific stock
   - Option 4: Display AAR for a group
   - Option 5: Display CAAR for a group
   - Option 6: Export CAAR data to CSV for visualization
   - Option 7: Perform bootstrapping analysis
   - Option 8: Exit

## Input Data Format

The application expects a CSV file with the following columns:
- Stock Symbol (e.g., AAPL, MSFT)
- EPS Estimate (analyst expectations)
- Actual EPS (reported earnings)
- Earnings Date (YYYY-MM-DD format)

Example:
```
Symbol,EPS_Estimate,Actual_EPS,Earnings_Date
AAPL,1.43,1.52,2024-01-25
MSFT,2.65,2.93,2024-01-30
GOOG,1.59,1.44,2024-02-01
```

## Implementation Details

The application uses a modular design with several key classes:

- **Stock**: Represents an individual stock with its associated data
- **Group**: Manages collections of stocks in the same category (Beat/Meet/Miss)
- **MarketData**: Handles API requests and market benchmark calculations
- **StockAnalyzer**: Orchestrates the overall analysis process

The analysis follows these steps:

1. Loading stock data with earnings information
2. Retrieving historical prices for each stock and the market benchmark (SPY)
3. Calculating returns and abnormal returns
4. Grouping stocks and computing group-level metrics (AAR, CAAR)
5. Performing bootstrapping for statistical robustness
6. Visualizing results through data export

## Bootstrapping

The bootstrapping procedure:
1. Randomly selects a sample of stocks from each group
2. Calculates metrics for the sampled stocks
3. Repeats the process multiple times
4. Computes average metrics across all iterations

This approach improves the reliability of the results by reducing the impact of outliers.

## Visualization

After exporting the CAAR data to CSV, you can create charts in Excel or other visualization tools to compare the performance of the three stock groups around earnings announcements. The typical pattern shows:

- Beat Group: Positive cumulative abnormal returns after announcement
- Miss Group: Negative cumulative abnormal returns after announcement
- Meet Group: More neutral performance

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Alpha Vantage for providing the financial data API
- libcurl for HTTP request functionality

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
