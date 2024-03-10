
.DEFAULT_GOAL = app
#g++ -o server -Iinclude webserver.cpp main.cpp http_client.cpp -lpthread -g
CXX = g++
CXXFLAGS += -O2
CXXFLAGS += -g
CXXFLAGS += -lpthread

SRC = *.cpp
INC_PATH := ./include
INCLUDES = $(addprefix -I, $(INC_PATH))

CXXFLAGS := -Wall -Werror $(INCLUDES) $(CXXFLAGS) 

.PHONY: app

app: 
	$(CXX) -o server $(SRC) $(CXXFLAGS) 

