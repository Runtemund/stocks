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

#include "core/Core.h"
#include "Nuitk.h"

class Stock;

/*!
 \brief The trading chart
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
      

      //! Paints the chart
      void paint(nui::Painter* painter);

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

      // The trading chart widget
      TradingChart* mChart;

};

#endif