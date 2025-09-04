
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <algorithm>  // für std::reverse

#include "Stocks.h"




struct MACDPoint
{
    double macd;
    double signal;
    double histogram;
};

class MACD {
public:
    // Standardparameter (können angepasst werden)
    int shortPeriod = 12;
    int longPeriod = 26;
    int signalPeriod = 9;

    std::vector<MACDPoint> compute(const std::vector<PriceRecord>& prices) 
    {
        std::vector<double> closes;
        for (const auto& p : prices) closes.push_back(p.close);

        std::vector<double> emaShort = computeEMA(closes, shortPeriod);
        std::vector<double> emaLong  = computeEMA(closes, longPeriod);

        std::vector<double> macdLine;
        size_t minSize = std::min(emaShort.size(), emaLong.size());

        for (size_t i = 0; i < minSize; ++i) {
            macdLine.push_back(emaShort[i + (longPeriod - shortPeriod)] - emaLong[i]);
        }

        std::vector<double> signalLine = computeEMA(macdLine, signalPeriod);
        std::vector<MACDPoint> result;

        size_t signalOffset = macdLine.size() - signalLine.size();
        for (size_t i = 0; i < signalLine.size(); ++i) {
            double macd = macdLine[i + signalOffset];
            double signal = signalLine[i];
            result.push_back({ macd, signal, macd - signal });
        }

        return result;
    }

private:
    std::vector<double> computeEMA(const std::vector<double>& data, int period)
    {
        std::vector<double> ema;
        if (data.size() < period) return ema;

        // Startwert: einfacher Durchschnitt der ersten "period" Werte
        double sum = 0.0;
        for (int i = 0; i < period; ++i)
            sum += data[i];
        double prevEma = sum / period;
        ema.push_back(prevEma);

        double multiplier = 2.0 / (period + 1);

        for (size_t i = period; i < data.size(); ++i) {
            double currentEma = (data[i] - prevEma) * multiplier + prevEma;
            ema.push_back(currentEma);
            prevEma = currentEma;
        }

        return ema;
    }
};

// Callback zum Sammeln der HTTP-Response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Funktion zum Abrufen des aktuellen Apple-Kurses
jm::String fetchStockData(const jm::String& symbol)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();

    jm::String apiKey;

    // Alpha Vantage API-Key: 
    if (curl)
    {
      jm::String url=jm::String("https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=%1&outputsize=full&datatype=csv&apikey=%2")
                     .arg(symbol)
                     .arg(apiKey);
        curl_easy_setopt(curl, CURLOPT_URL, url.toCString().constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0"); // Verhindert Blockierung

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    // Symbolsuche: Scheint ohne API-Key zu funktionieren
    // Liefert: 
    // symbol,name,type,region,marketOpen,marketClose,timezone,currency,matchScore
    // BMW.FRK,Bayerische Motoren Werke Aktiengesellschaft,Equity,Frankfurt,08:00,20:00,UTC+02,EUR,0.7500
    // https://www.alphavantage.co/query?function=SYMBOL_SEARCH&keywords=microsoft&datatype=csv&apikey=DEIN_API_KEY


    return jm::String(readBuffer.c_str());
}

MainWindow::MainWindow(): nui::ApplicationWindow(nui::Application::instance())
{
    mDb = new StockDatabase("stocks.db");

    if (!mDb->initSchema()) 
    {
        std::cerr << "Failed to initialize DB schema\n";
    }
    else
    {
      
    /*   std::cout << "Add Stock: AAPL" << std::endl;
      mDb->addStock("AAPL", "Apple Inc.");
      jm::String rawcsv = fetchStockData("AAPL");

      jm::StringTokenizer st=jm::StringTokenizer(rawcsv,"\r\n",false);

      // ignore 1st line
      if(st.hasNext())st.next();

      while(st.hasNext())
      {
         jm::String line=st.next();

         jm::StringTokenizer lt=jm::StringTokenizer(line,",",false);

         // Alpha Vantage order:
         // timestamp,open,high,low,close,volume
         jm::Date timestamp=sqlToDate(lt.next());
         double open=lt.next().toDouble();
         double high=lt.next().toDouble();
         double low=lt.next().toDouble();
         double close=lt.next().toDouble();
         int volume=lt.next().toInt();

         PriceRecord record = {timestamp, open, high, low, close, volume};

         std::cout << "insert price " <<(mDb->insertPrice("AAPL", record)?"YES":"NO")<<std::endl;
      }*/
    }



   Stock* stock = mDb->stock("AAPL");

   mChart = new TradingChart();
   mChart->setStock(stock);

   setChild(mChart);

   setMinimumSize(jm::Size(800,600));
}

MainWindow::~MainWindow()
{
   delete mDb;
}
