////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        Stocks.h
// Application: Stock Analyser
// Purpose:     Declaration of Application
//
// Author:      Uwe Runtemund (2025-today)
// Modified by:
// Created:     01.09.2025
//
// Copyright:   (c) 2025 Jameo Software, Germany. https://jameo.de
//
//              All rights reserved. The methods and techniques described herein are considered
//              trade secrets and/or confidential. Reproduction or distribution, in whole or in
//              part, is forbidden except by express written permission of Jameo.
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef StockMainWindow_h
#define StockMainWindow_h

#include <sqlite3.h>

#include "core/Core.h"
#include "Nuitk.h"

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
      jm::String symbol;
      jm::String currency;

      std::vector<PriceRecord> priceHistory;

      /*!
       \brief Returns the minimum price in the given time range
       \param start First day of range (including)
       \param end Last day of range (including)
       */
      double minPrice(size_t start,size_t end) const
      {
         if(priceHistory.size()==0)return 0;

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
         if(priceHistory.size()==0)return 0;
         
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
         if(priceHistory.size()==0)return 0;
         
         int volume = priceHistory[start].volume;
         for(size_t index=start+1;index <= end; index++)
         {
            int vol = priceHistory[index].volume;
            if(volume > vol ) volume = vol;
         }
         return volume;
      }

};

/*!
 \brief Represents a chart line.
 */
class DllExport Chart
{

};

/*!
 \brief The trading chart.

  - The horizontal axis is the time line
  - The vertical axis depends on the shown chart
  - The trading chart may contain multiple subcharts. 
  - Eachs subchart may contain multiple overlays
 */
class DllExport TradingChart: public nui::Canvas
{
   public:

      TradingChart();

      void setStock(const Stock* stock);

   private:

      //! The main stock to display.
      const Stock* mStock = nullptr;

      //! The first visible tick
      int64 mFirst;

      //! The last visible tick
      int64 mLast;

      //! The visible span;
      int64 mSpan;
      
      //! Current curser position
      jm::Point mCursor;

      //! Area of the chart
      jm::Rect chartArea;

      //! Paints the chart
      void paint(nui::Painter* painter);

};

/*!
 \brief The stock database stores and manages the prices of the stocks in a local database.

 */
class StockDatabase 
{
   public:

      /*!
       \brief Construktor
       */
      StockDatabase(const jm::String& dbFile);

      /*!
       \brief Destruktor
       */
      ~StockDatabase();

      /*!
       \brief Initializes the stock database.
       */
      bool initSchema();

      /*!
       \brief Adds an empty stock to the database.
       */
      int addStock(const jm::String& symbol, 
                   const jm::String& name , 
                   const jm::String& currency);
      
      bool insertPrice(const jm::String& symbol, const PriceRecord& record);

      /*!
       \brief Returns the stock, if it exists in the database.

       \return The stock or nullptr, if the stock does not exist in the local database.
       */
      Stock* stock(const jm::String& symbol);
      
   private:

      sqlite3* mDb;

      int getStockId(const jm::String& symbol);

      bool getStockData(const jm::String& symbol,
                        jm::String& name,
                        jm::String& currency);

      std::vector<PriceRecord> getPrices(const jm::String& symbol);
};

/*!
 \brief This is the main window of the application
 */
class DllExport MainWindow: public nui::ApplicationWindow
{

   public:

      /*!
       \brief Default constructor
       */
      MainWindow();

      /*!
       \brief Default destructor
       */
      ~MainWindow();

   private:

      //! The local stock database, it contains the market data.
      StockDatabase* mDb;

      // The trading chart widget
      TradingChart* mChart;

};

#endif