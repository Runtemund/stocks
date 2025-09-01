//
//  Main.cpp
//  stocks
//
//  Created by Uwe Runtemund on 01.09.2025
//  Copyright © 2025 Jameo Software. All rights reserved.
//

#include "Stocks.h"


int main(int argc, const char* argv[])
{
   nui::Application* application = new nui::Application(argc, argv, "de.runtemund.stocks", "Stock Charts");

   application->mOnStartUp = [ = ]()
   {
      MainWindow* win = new MainWindow();
      win->show();
   };

   application->mOnCleanUp = [ = ]()
   {
      return 0;
   };

   int32 status = application->run();
   delete application;
   return status;
}
