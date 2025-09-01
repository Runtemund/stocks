
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <algorithm>  // für std::reverse

#include "Stocks.h"

/*!
 \brief The price record for one period (usually a day).
 */
struct PriceRecord
{
   jm::Date date;
   double open;
   double high;
   double low;
   double close;
   int volume;
};

class Stock
{
   public:

      jm::String name;

      std::vector<PriceRecord> priceHistory;

      /*!
       \brief Returns the minimum price in the given time range
       \param start First day of range (including)
       \param end Last day of range (including)
       */
      double minPrice(size_t start,size_t end) const
      {
         double price = priceHistory[start].low;
         for(size_t index=start+1;index <= end; index++)
         {
            double low = priceHistory[index].low;
            if(low < price ) price = low;
         }
         return price;
      }

      /*!
       \brief Returns the maximum price in the given time range
       \param start First day of range (including)
       \param end Last day of range (including)
       */
      double maxPrice(size_t start,size_t end) const
      {
         double price = priceHistory[start].high;
         for(size_t index=start+1;index <= end; index++)
         {
            double high = priceHistory[index].high;
            if(high > price ) price = high;
         }
         return price;
      }

      /*!
       \brief Returns the volume in the given time range
       \param start First day of range (including)
       \param end Last day of range (including)
       */
      int maxVolume(size_t start,size_t end) const
      {
         int volume = priceHistory[start].volume;
         for(size_t index=start+1;index <= end; index++)
         {
            int vol = priceHistory[index].volume;
            if(volume > vol ) volume = vol;
         }
         return volume;
      }

};

struct MACDPoint {
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
std::string fetchAppleStockData()
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://query1.finance.yahoo.com/v7/finance/quote?symbols=AAPL");
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

    return readBuffer;
}


TradingChart::TradingChart()
{
   mLast=jm::Date();
   mFirst=jm::Date();
   mSpan = 30;
   mFirst.setDate(mFirst.date()-mSpan);

   setOnPaint([this](nui::Painter* painter)
   {
      paint(painter);
   });

   setOnMouseWheel([this](nui::EventState& state)
   {
      double delta = state.dy;

      if(delta>0)
      {
         mSpan*=1.1;
      }
      else
      {
         mSpan/=1.1;
      }

      if(mSpan>10000)mSpan=30;
      if(mSpan<10)mSpan=10;

      mFirst=mLast;
      mFirst.setDate(mFirst.date()-mSpan);
      update();
   });
}

void TradingChart::setStock(const Stock* stock)
{
   mStock=stock;
   if(stock->priceHistory.size()>0)
   {
      mLast=stock->priceHistory[stock->priceHistory.size()-1].date;
      mFirst=mLast;
      mFirst.setDate(mFirst.date()-mSpan);
   }
}


bool sameDay(const jm::Date& d1, const jm::Date& d2)
{
   return d1.year() == d2.year()
          && d1.month() == d2.month()
          && d1.date() == d2.date();
}


void TradingChart::paint(nui::Painter* painter)
{
   if(mStock==nullptr) return;

   //
   // Layout Settings
   //
   jm::Color colBackground = jm::Color::fromRgb(30,30,60);
   jm::Color colGrid = jm::Color::fromRgb(60,60,90);
   jm::Color colAxis = jm::Color::fromRgb(130,130,160);
   jm::Color colForeground = jm::Color::fromRgb(180,180,210);

   jm::Color colBullishCandle = jm::Color::fromRgb(80,220,130);
   jm::Color colBearishCandle = jm::Color::fromRgb(240,100,100);

   jm::Color colVolumeChart = jm::Color::fromRgb(100, 150, 220);

   int margin=20;
   int marginBottom=25+painter->wordHeight();

   jm::Rect bounds = this->bounds();
   bounds.setY(0);

   jm::Rect chartArea=jm::Rect(jm::Point(margin,margin),jm::Size(bounds.width()-2*margin,bounds.height()-margin-marginBottom));

   //
   // Chart settings
   //
   // Note: Data can be incomplete, so the x-axis is time based, but the chart may have gaps

   // Find first and last record index
   size_t firstRecord=0;
   size_t lastRecord=mStock->priceHistory.size()-1;

   for(size_t index=0;index<mStock->priceHistory.size();index++)
   {
      const PriceRecord& record = mStock->priceHistory[index];

      // we need this kind of comparing in case, mFirst is not present in price history, then use the most smallest
      if(sameDay(record.date,mFirst) || record.date < mFirst)
      {
         firstRecord=index;
      }

      if(sameDay(record.date, mLast))
      {
         lastRecord=index;
      }
   }

   jm::DateFormatter df2=jm::DateFormatter("dd.MM.yy");

   std::cout<<"Rec: "<<firstRecord<<" "<<lastRecord<<std::endl;
   std::cout<<"Dat: "<<df2.format(mFirst)<<" "<<df2.format(mLast)<<std::endl;

   //Number of shown days
   size_t days = mSpan+1;

   double xScale=chartArea.width()/days;

   double low = mStock->minPrice(firstRecord,lastRecord);
   double high = mStock->maxPrice(firstRecord,lastRecord);
   double delta = high-low;

   double yScale = (double)chartArea.height()/delta;
   double volScale = 100.0/(double)mStock->maxVolume(firstRecord,lastRecord);


   //
   // Drawing
   //
   
   // Fill background
   painter->rectangle(bounds);
   painter->setFillColor(colBackground);
   painter->fill();

   //
   // Draw X-Axis
   //

   // Draw grid (monthly)
   painter->setFillColor(colAxis);
   painter->setStrokeColor(colGrid);
   jm::DateFormatter df=jm::DateFormatter("MMM");
   jm::Date current = mFirst;
   size_t index=0;
   while(current<=mLast)
   {
      if(current.date()==1)// 1st of month
      {
         jm::String label = df.format(current);
         int width = painter->wordWidth(label);
         painter->drawText(label,jm::Point(chartArea.left()+index*xScale-0.5*width,chartArea.bottom()+5+painter->wordAscent()));
         painter->line(chartArea.left()+index*xScale,chartArea.bottom(),chartArea.left()+index*xScale,chartArea.top());
         painter->stroke();
      }
      current.setDate(current.date()+1);
      index++;
   }

   // Draw axis line
   painter->setStrokeColor(colAxis);
   painter->line(chartArea.bottomLeft(),chartArea.bottomRight());
   painter->stroke();


   // Draw Y-Axis
   painter->setStrokeColor(colAxis);
   painter->line(chartArea.topRight(),chartArea.bottomRight());
   painter->stroke();

   // Draw title
   painter->setFillColor(colAxis);
   painter->drawText(mStock->name,jm::Point(chartArea.left(),chartArea.top()+painter->wordAscent()));


   // Draw volume
   painter->setFillColor(colVolumeChart);
   index=0;
   current = mFirst;
   int64 tick=0;
   while(current<=mLast)
   {
      while(mStock->priceHistory[index].date < current)index++;

      const PriceRecord& record = mStock->priceHistory[index];

      if(sameDay(record.date,current))
      {
         jm::Rect rect = jm::Rect(chartArea.left()+tick*xScale-0.2*xScale,
                                       chartArea.bottom()-record.volume*volScale,
                                       xScale*0.4,
                                       record.volume*volScale );
         painter->rectangle(rect);
         painter->fill();
         index++;
      }

      current.setDate(current.date()+1);
      tick++;
   }

   // Draw Linechart
   painter->setStrokeColor(colForeground);
      index=0;
   current = mFirst;
   tick=0;
   bool first=true;
   while(current<=mLast)
   {
      while(mStock->priceHistory[index].date < current)index++;

      const PriceRecord& record = mStock->priceHistory[index];

      if(sameDay(record.date,current))
      {
         if(first)painter->moveTo(jm::Point(chartArea.left()+tick*xScale,chartArea.bottom()-(mStock->priceHistory[index].close-low)*yScale));
         else painter->lineTo(jm::Point(chartArea.left()+tick*xScale,chartArea.bottom()-(mStock->priceHistory[index].close-low)*yScale));
         first=false;
      }

      current.setDate(current.date()+1);
      tick++;
   }
   painter->stroke();

   // Draw candles
   index=0;
   current = mFirst;
   tick=0;
   while(current<=mLast)
   {
      while(mStock->priceHistory[index].date < current)index++;

      const PriceRecord& record = mStock->priceHistory[index];

      if(sameDay(record.date,current))
      {
         const PriceRecord& record = mStock->priceHistory[index];

         double candleTop=std::max(record.open,record.close);
         double candleHeight=std::abs(record.open-record.close);

         if(record.close > record.open)
         {
            painter->setStrokeColor(colBullishCandle);
            painter->setFillColor(colBullishCandle);
         }
         else
         {
            painter->setStrokeColor(colBearishCandle);
            painter->setFillColor(colBearishCandle);            
         }

         painter->line(jm::Point(chartArea.left()+tick*xScale,chartArea.bottom()-(record.low-low)*yScale),
                        jm::Point(chartArea.left()+tick*xScale,chartArea.bottom()-(record.high-low)*yScale));
         painter->stroke();
         painter->rectangle(jm::Rect(chartArea.left()+tick*xScale-0.4*xScale,
                                       chartArea.bottom()-(candleTop-low)*yScale,
                                       xScale*0.8,
                                       candleHeight*yScale ));
         painter->fill();
      }

      current.setDate(current.date()+1);
      tick++;
   }


   // DRAW MACD
   double macdScale = 100/3;

   MACD macdCalculator;
   std::vector<MACDPoint> macdData = macdCalculator.compute(mStock->priceHistory);

   // MACD line
   painter->setStrokeColor(jm::Color::fromRgb(255, 165, 0));
   painter->moveTo(jm::Point(chartArea.left()+35*xScale,chartArea.center().y()-(macdData[0].macd)*macdScale));
   for(size_t index=0;index<=lastRecord-35;index++)
   {
      painter->lineTo(jm::Point(chartArea.left()+(index+35)*xScale,chartArea.center().y()-(macdData[index].macd)*macdScale));
   }
   painter->stroke();

   // Signal line
   painter->setStrokeColor(jm::Color::fromRgb(0, 200, 255));
   painter->moveTo(jm::Point(chartArea.left()+35*xScale,chartArea.center().y()-(macdData[0].signal)*macdScale));
   for(size_t index=0;index<=lastRecord-35;index++)
   {
      painter->lineTo(jm::Point(chartArea.left()+(index+35)*xScale,chartArea.center().y()-(macdData[index].signal)*macdScale));
   }
   painter->stroke();

   // Histogram
}

MainWindow::MainWindow(): nui::ApplicationWindow(nui::Application::instance())
{
   Stock* stock = new Stock();
   stock->name="APPL";

stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 22), 227.79, 228.34, 223.90, 224.53, 43695300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 21), 226.52, 227.98, 225.05, 226.40, 34765500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 20), 225.77, 227.17, 225.45, 226.51, 30299000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 19), 225.72, 225.99, 223.04, 225.89, 40687800});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 16), 223.92, 226.83, 223.65, 226.05, 44340200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 15), 224.60, 225.35, 222.76, 224.72, 46414000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 14), 220.57, 223.03, 219.70, 221.72, 41960600});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 13), 219.01, 221.89, 219.01, 221.27, 44155300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 9), 212.10, 216.78, 211.97, 215.99, 42201600});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 8), 213.11, 214.20, 208.83, 213.06, 47161100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 7), 214.03, 215.26, 211.81, 213.90, 38121100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 6), 212.96, 214.45, 211.99, 213.59, 37495500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 5), 210.25, 212.71, 209.10, 212.48, 40271000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 2), 208.61, 210.29, 208.30, 209.40, 35810700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::AUGUST, 1), 207.50, 208.87, 205.87, 207.47, 38298200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 31), 207.90, 209.23, 205.71, 206.72, 46285800});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 30), 206.81, 209.74, 206.49, 209.57, 40201500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 29), 209.36, 211.51, 208.18, 210.78, 42752200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 26), 207.80, 210.64, 207.45, 210.39, 39900000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 25), 205.62, 207.95, 204.76, 207.17, 45263300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 24), 204.92, 206.37, 204.10, 205.03, 39677000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 23), 203.21, 205.87, 202.71, 204.80, 46227300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 22), 202.03, 204.34, 201.65, 204.19, 41249200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 19), 198.80, 201.62, 198.74, 201.45, 39293000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 18), 198.60, 200.66, 198.05, 200.12, 42282700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 17), 197.98, 199.75, 197.58, 199.01, 39271600});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 16), 197.10, 198.90, 196.47, 198.18, 35666700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 15), 197.69, 199.28, 197.11, 198.43, 43817400});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 12), 194.97, 196.91, 194.50, 196.44, 40395800});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 11), 193.10, 195.26, 192.78, 195.01, 38286500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 10), 193.25, 194.42, 191.61, 193.56, 43101100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 9), 191.90, 193.36, 191.39, 193.06, 37328500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 8), 189.66, 191.84, 189.52, 191.28, 34468200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 5), 187.00, 189.68, 186.73, 189.12, 38577300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 3), 188.50, 189.01, 185.75, 186.15, 34984200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 2), 186.35, 188.47, 186.03, 188.33, 34719200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JULY, 1), 187.78, 188.97, 186.01, 187.07, 36732500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 28), 186.12, 187.45, 185.40, 186.69, 38290100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 27), 185.01, 186.75, 184.82, 186.21, 33358000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 26), 184.50, 185.35, 183.30, 184.85, 31845200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 25), 183.40, 185.20, 183.12, 184.91, 34697100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 24), 184.02, 185.78, 183.11, 185.55, 35021800});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 21), 182.80, 184.52, 182.44, 183.99, 35927100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 20), 181.33, 182.70, 180.89, 182.46, 32452900});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 19), 180.12, 181.45, 179.56, 181.03, 34310300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 18), 178.98, 180.27, 178.25, 179.95, 31827900});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 17), 177.85, 179.32, 177.63, 179.04, 31215700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 14), 176.99, 178.44, 176.56, 177.87, 33450800});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 13), 175.33, 177.16, 174.85, 176.42, 34528700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 12), 174.89, 175.92, 173.99, 175.17, 31872600});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 11), 173.95, 175.10, 173.21, 174.25, 32245500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 10), 172.74, 174.40, 172.32, 173.92, 33401700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 7), 171.50, 172.85, 170.95, 172.28, 31295700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 6), 170.88, 172.10, 170.15, 171.65, 29813000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 5), 169.15, 171.05, 168.75, 170.78, 30987500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 4), 168.33, 169.50, 167.48, 168.90, 29637200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::JUNE, 3), 167.60, 168.75, 166.95, 168.20, 28712300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 31), 166.40, 168.00, 165.90, 167.80, 30917400});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 30), 165.75, 167.50, 165.15, 166.90, 28450200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 29), 165.00, 166.45, 164.50, 165.80, 27384900});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 28), 164.50, 165.90, 164.00, 165.35, 27944100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 27), 163.75, 165.25, 163.10, 164.70, 28567300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 24), 162.85, 164.40, 162.35, 164.10, 29823400});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 23), 162.10, 163.75, 161.50, 163.20, 31254000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 22), 161.50, 163.10, 160.90, 162.35, 30587400});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 21), 160.75, 162.20, 160.25, 161.60, 31456700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 20), 160.00, 161.65, 159.45, 161.05, 29812300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 17), 159.35, 160.85, 158.80, 160.25, 28234700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 16), 158.70, 160.25, 158.10, 159.50, 27439800});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 15), 158.00, 159.60, 157.45, 158.80, 26749100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 14), 157.25, 158.85, 156.75, 158.10, 26985000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 13), 156.50, 158.00, 156.00, 157.35, 27341200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 10), 155.90, 157.45, 155.30, 156.75, 28047300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 9), 155.15, 156.70, 154.60, 156.00, 29051200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 8), 154.50, 156.00, 153.95, 155.25, 28510300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 7), 153.80, 155.25, 153.25, 154.55, 27893100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 6), 153.15, 154.70, 152.60, 153.85, 28271900});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 3), 152.40, 154.00, 151.85, 153.10, 29102300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 2), 151.75, 153.25, 151.20, 152.40, 29531200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::MAY, 1), 151.00, 152.50, 150.45, 151.70, 28284500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 30), 150.40, 152.00, 149.85, 151.05, 27984300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 29), 149.75, 151.25, 149.20, 150.40, 28312100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 26), 149.00, 150.50, 148.45, 149.65, 27698100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 25), 148.25, 149.75, 147.70, 148.90, 27345200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 24), 147.50, 149.00, 146.95, 148.15, 26937500});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 23), 146.75, 148.25, 146.20, 147.40, 27198000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 22), 146.00, 147.50, 145.45, 146.65, 26894200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 19), 145.25, 146.75, 144.70, 145.90, 27463800});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 18), 144.50, 146.00, 143.95, 145.15, 27923100});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 17), 143.75, 145.25, 143.20, 144.40, 27649800});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 16), 143.00, 144.50, 142.45, 143.65, 27394700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 15), 142.25, 143.75, 141.70, 142.90, 26943000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 12), 141.50, 143.00, 140.95, 142.15, 26678200});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 11), 140.75, 142.25, 140.20, 141.40, 26431000});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 10), 140.00, 141.50, 139.45, 140.65, 26198700});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 9), 139.25, 140.75, 138.70, 139.90, 25874300});
stock->priceHistory.push_back({jm::Date(2024, jm::Date::APRIL, 8), 138.50, 140.00, 137.95, 139.15, 25592400});
std::reverse(stock->priceHistory.begin(), stock->priceHistory.end());

   mChart = new TradingChart();
   mChart->setStock(stock);

   setChild(mChart);

   setMinimumSize(jm::Size(800,600));
}

MainWindow::~MainWindow()
{
}
