#Pfade
PATH_SRC = src
PATH_INC = include
PATH_BIN = bin

PATH_JAMEORT = ../libcore
PATH_NUITK = ../nuitk

# Linker-Optionen und Lib-Name
# mit BS-Switch Unix

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Linux)
   CXX=clang++
   CFLAGS = -Wall -Wextra -pedantic -Werror -Os -g -Woverloaded-virtual -c -pipe -g -Wall -fPIC -std=c++20
   LFLAGS = -pthread -lcurl -lsqlite3 -lcore -lnuitk -pthread -L.  -Wl,-rpath .
   EXEC_NAME = astruss
endif

ifeq ($(UNAME_S),Darwin)
   CXX=clang++
   CFLAGS = -Wall -pedantic -Wextra -O3 -c -pipe -g  -fPIC -std=c++20
   LFLAGS = -L. -ljameo -lnuitk -lcurl -lsqlite3 -framework CoreFoundation -framework CoreServices -framework Foundation -headerpad_max_install_names
   EXEC_NAME = stocks
endif


#
# AB HIER SOLLTEN KEINE EINSTELLUNGEN MEHR VORGENOMMEN WERDEN
#
#

# Liste der Quelltextdateien
SOURCES =\
 $(PATH_SRC)/Main.cpp\
 $(PATH_SRC)/MainWindow.cpp\
 $(PATH_SRC)/StockDatabase.cpp\
 $(PATH_SRC)/TradingChart.cpp\


OBJECTS = $(SOURCES:.cpp=.o)

INCLUDE = -I$(PATH_INC)\
 -I$(PATH_JAMEORT)/include/\
 -I$(PATH_SRC)/\
 -I$(PATH_NUITK)/include

ifeq ($(UNAME_S),Linux)

# Target = ALL
all: $(OBJECTS)

	cd $(PATH_JAMEORT)/; make -j8
	cp $(PATH_JAMEORT)/bin/libcore.so libcore.so

	cd $(PATH_NUITK)/; make -j8
	cp $(PATH_NUITK)/bin/libnuitk.so libnuitk.so

	$(CXX) $(LFLAGS) -o $(EXEC_NAME) $(OBJECTS)
	mkdir -p $(PATH_BIN)
	mv $(EXEC_NAME) $(PATH_BIN)/$(EXEC_NAME)

endif

ifeq ($(UNAME_S),Darwin)

# Target = ALL
all: $(OBJECTS)
	mkdir -p $(PATH_BIN)/Stocks.app/Contents/{MacOS,Resources/{icons,translations},Library/{QuickLook,Spotlight}}

	cd $(PATH_JAMEORT)/; make -j8
	cp $(PATH_JAMEORT)/bin/libjameo.dylib libjameo.dylib

	cd $(PATH_NUITK)/; make -j8
	cp $(PATH_NUITK)/bin/libnuitk.dylib libnuitk.dylib

	$(CXX) $(LFLAGS) -o $(EXEC_NAME) $(OBJECTS)

	#Nametool
	install_name_tool -id @executable_path/libjameo.dylib libjameo.dylib
	install_name_tool -id @executable_path/libnuitk.dylib libnuitk.dylib

	# executable
	install_name_tool -change libjameo.dylib @executable_path/libjameo.dylib $(EXEC_NAME)
	install_name_tool -change libnuitk.dylib @executable_path/libnuitk.dylib $(EXEC_NAME)

	# libnuitk
	install_name_tool -change libjameo.dylib @executable_path/libjameo.dylib libnuitk.dylib

	mv $(EXEC_NAME) $(PATH_BIN)/Stocks.app/Contents/MacOS/$(EXEC_NAME)
	mv *.dylib $(PATH_BIN)/Stocks.app/Contents/MacOS/
	cp Info.plist $(PATH_BIN)/Stocks.app/Contents/Info.plist

endif

install:
	# create directories
	mkdir -p $(prefix)/usr/lib/astruss/
	mkdir -p $(prefix)/usr/share/astruss/
	mkdir -p $(prefix)/usr/share/astruss/cad
	mkdir -p $(prefix)/usr/share/astruss/translations

	# copy app binary
	cp $(PATH_BIN)/astruss $(prefix)/usr/bin/astruss

	# copy libraries
	cp *.so $(prefix)/usr/lib/astruss

	# copy fonts
	cp $(PATH_VXF)/resources/fonts/jameo.shx $(prefix)/usr/share/astruss/cad/

	# Translations
	# cp ../translations/de_DE.mo $(prefix)/usr/share/engineer/translations/de.jameo.engineer.de.mo
	cp $(PATH_PHYSICS)/translations/de_DE.mo $(prefix)/usr/share/astruss/translations/de.jameo.Physics.de.mo

uninstall:
	rm -rf /usr/lib/astruss
	rm -rf /usr/share/astruss
	rm /usr/bin/astruss

test:
	make all
	$(CXX) $(CFLAGS) $(INCLUDE) -c ../test/main.cpp -o ../test/main.o
	$(CXX) -g -o test ../test/main.o $(PATH_BIN)/libphysics.a ../../../jameort/trunk/bin/libjameo.a

$(PATH_SRC)/Precompiled.pch: $(PATH_SRC)/Precompiled.hpp
	$(CXX) $(CFLAGS) $(INCLUDE)  $(PATH_SRC)/Precompiled.hpp  -o $(PATH_SRC)/Precompiled.pch

%.o: %.cpp $(PATH_SRC)/Precompiled.pch
	$(CXX) $(CFLAGS) $(INCLUDE) -include-pch $(PATH_SRC)/Precompiled.pch -c $< -o $@

$(PATH_SRC)/moc_%.cpp: $(PATH_INC)/%.h
	$(MOC) $(INCLUDE) $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(PATH_SRC)/Precompiled.pch
	rm -Rf $(PATH_BIN)/*
	rm -f *.so
#	cd $(PATH_JAMEORT)/; make clean
#	cd $(PATH_NUITK)/; make clean
# DO NOT DELETE
