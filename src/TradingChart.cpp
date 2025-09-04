//
//  Main.cpp
//  stocks
//
//  Created by Uwe Runtemund on 01.09.2025
//  Copyright © 2025 Jameo Software. All rights reserved.
//

#include "Precompiled.hpp"


TradingChart::TradingChart()
{
   mLast=0;
   mFirst=0;
   mSpan = 30;

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

      if(mSpan>mStock->priceHistory.size())mSpan=mStock->priceHistory.size();
      if(mSpan<10)mSpan=10;

      mFirst=std::max(mLast-mSpan,int64(0));
      update();
   });

   setOnMouseMove([this](nui::EventState& state)
   {
      if(state.down == true && state.button == nui::MouseButton::kLeft)
      {
         
      }
      mCursor=state.position();
      update();
   });

}

void TradingChart::setStock(const Stock* stock)
{
   mStock=stock;
   if(stock->priceHistory.size()>0)
   {
      mLast=stock->priceHistory.size()-1;
      mFirst=std::max(mLast-mSpan,int64(0));
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
   painter->setLineStyle(nui::LineStyle::kSolid);

   if(mStock==nullptr) return;
   if(mStock->priceHistory.size()==0)return;

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
   int marginRight=25+painter->wordWidth(jm::String("%1").arg(mStock->maxPrice(mFirst,mLast),0,2));
   int marginBottom=25+painter->wordHeight();

   jm::Rect bounds = this->bounds();
   bounds.setY(0);

   chartArea=jm::Rect(jm::Point(margin,margin),jm::Size(bounds.width()-margin-marginRight,bounds.height()-margin-marginBottom));

   //
   // Chart settings
   //
   // Note: Data can be incomplete, so the x-axis is time based, but the chart may have gaps

   //Number of shown days
   size_t days = mSpan+1;

   mXScale=chartArea.width()/days;

   double low = mStock->minPrice(mFirst,mLast);
   double high = mStock->maxPrice(mFirst,mLast);

   double volScale = 100.0/(double)mStock->maxVolume(mFirst,mLast);


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
   int64 tick=0;
   jm::Date last=mStock->priceHistory[0].date;
   for(int64 index=mFirst;index<=mLast;index++)
   {
      jm::Date current=mStock->priceHistory[index].date;
      if(current.month()!=last.month())// 1st of month
      {
         jm::String label = df.format(current);
         int width = painter->wordWidth(label);
         painter->drawText(label,jm::Point(chartArea.left()+tick*mXScale-0.5*width,chartArea.bottom()+5+painter->wordAscent()));
         painter->line(chartArea.left()+tick*mXScale,chartArea.bottom(),chartArea.left()+tick*mXScale,chartArea.top());
         painter->stroke();
      }
      tick++;
      last=current;
   }

   // Draw axis line
   painter->setStrokeColor(colAxis);
   painter->line(chartArea.bottomLeft(),chartArea.bottomRight());
   painter->stroke();

   //
   // Draw Y-Axis
   //
   painter->setFillColor(colAxis);
   painter->setStrokeColor(colGrid);
   double priceStep=1;
   double range=(mStock->maxPrice(mFirst,mLast)-mStock->minPrice(mFirst,mLast));
   if(range<5)priceStep=1.0;
   else if(range<10)priceStep=2.0;
   else if(range<25)priceStep=5.0;
   else if(range<50)priceStep=10.0;
   else if(range<100)priceStep=20.0;
   else priceStep=std::floor(range/5.0);

   double start=mStock->minPrice(mFirst,mLast)-std::fmod(mStock->minPrice(mFirst,mLast),priceStep);
   double current=0;
   int ticks= std::ceil((mStock->maxPrice(mFirst,mLast)-start)/priceStep);

   double yScale = chartArea.height()/(priceStep*ticks);

   for(int64 index=0;index<=ticks;index++)
   {
      painter->drawText(jm::String("%1").arg(start+current,0,2),
                        jm::Point(chartArea.right()+4,chartArea.bottom()-yScale*current));
      painter->line(chartArea.left(),chartArea.bottom()-yScale*current,
                    chartArea.right(),chartArea.bottom()-yScale*current);
      painter->stroke();
      current+=priceStep;
   }
   // Draw grid (price)

   // Draw axis line
   painter->setStrokeColor(colAxis);
   painter->line(chartArea.topRight(),chartArea.bottomRight());
   painter->stroke();

   // Draw title
   painter->setFillColor(colAxis);
   painter->drawText(mStock->name,jm::Point(chartArea.left(),chartArea.top()+painter->wordAscent()));


   // Draw volume
   painter->setFillColor(colVolumeChart);
   tick=0;
   for(int64 index=mFirst;index<=mLast;index++)
   {
      const PriceRecord& record = mStock->priceHistory[index];

      jm::Rect rect = jm::Rect(chartArea.left()+tick*mXScale-0.2*mXScale,
                                    chartArea.bottom()-record.volume*volScale,
                                    mXScale*0.4,
                                    record.volume*volScale );
      painter->rectangle(rect);
      painter->fill();
      tick++;
   }

   // Draw Linechart
   painter->setStrokeColor(colForeground);
   tick=0;
   bool first=true;
   for(int64 index=mFirst;index<=mLast;index++)
   {
      const PriceRecord& record = mStock->priceHistory[index];

      if(first)painter->moveTo(jm::Point(chartArea.left()+tick*mXScale,chartArea.bottom()-(mStock->priceHistory[index].close-start)*yScale));
      else painter->lineTo(jm::Point(chartArea.left()+tick*mXScale,chartArea.bottom()-(mStock->priceHistory[index].close-start)*yScale));
      first=false;

      tick++;
   }
   painter->stroke();

   // Draw candles
   tick=0;
   for(int64 index=mFirst;index<=mLast;index++)
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

      painter->line(jm::Point(chartArea.left()+tick*mXScale,chartArea.bottom()-(record.low-start)*yScale),
                     jm::Point(chartArea.left()+tick*mXScale,chartArea.bottom()-(record.high-start)*yScale));
      painter->stroke();
      painter->rectangle(jm::Rect(chartArea.left()+tick*mXScale-0.4*mXScale,
                                    chartArea.bottom()-(candleTop-start)*yScale,
                                    mXScale*0.8,
                                    candleHeight*yScale ));
      painter->fill();
      tick++;
   }

/*
   // DRAW MACD
   double macdScale = 100/3;

   MACD macdCalculator;
   std::vector<MACDPoint> macdData = macdCalculator.compute(mStock->priceHistory);

   // MACD line
   painter->setStrokeColor(jm::Color::fromRgb(255, 165, 0));
   painter->moveTo(jm::Point(chartArea.left()+35*xScale,chartArea.center().y()-(macdData[0].macd)*macdScale));
   for(size_t index=0;index<=mLast-35;index++)
   {
      painter->lineTo(jm::Point(chartArea.left()+(index+35)*xScale,chartArea.center().y()-(macdData[index].macd)*macdScale));
   }
   painter->stroke();

   // Signal line
   painter->setStrokeColor(jm::Color::fromRgb(0, 200, 255));
   painter->moveTo(jm::Point(chartArea.left()+35*xScale,chartArea.center().y()-(macdData[0].signal)*macdScale));
   for(size_t index=0;index<=mLast-35;index++)
   {
      painter->lineTo(jm::Point(chartArea.left()+(index+35)*xScale,chartArea.center().y()-(macdData[index].signal)*macdScale));
   }
   painter->stroke();

   // Histogram
*/
   // Draw line cross
   if(chartArea.contains(mCursor))
   {
      painter->setLineStyle(nui::LineStyle::kDashed);
      painter->line(mCursor.x(),chartArea.top(),mCursor.x(),chartArea.bottom());
      painter->line(chartArea.left(),mCursor.y(),chartArea.right(),mCursor.y());
      painter->stroke();
   }
}
