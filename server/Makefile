FLAGS = -O2 -Wall -Wextra -Wno-switch
CXX = g++
CC = cc

TARGET_NAME = e2eep-server


SRC  = $(shell find ./src -type f -name *.cpp)
SRC += $(shell find ../shared/ -type f -name *.cpp)

C_SRC  = $(shell find ../shared/ext -type f -name *.c)

OBJS = $(SRC:.cpp=.o)
C_OBJS = $(C_SRC:.c=.o)

LIBS = -lssl -lcrypto

all: $(TARGET_NAME)


%.o: %.c
	@$(CC) $(FLAGS) -c $< -o $@ && (echo -e "\033[32m[Compiled]\033[0m $<") || (echo -e "\033[31m[Failed]\033[0m $<"; exit 1) 

%.o: %.cpp
	@$(CXX) $(FLAGS) -c $< -o $@ && (echo -e "\033[32m[Compiled]\033[0m $<") || (echo -e "\033[31m[Failed]\033[0m $<"; exit 1) 

$(TARGET_NAME): $(OBJS) $(C_OBJS)
	@echo -e "\033[90mLinking...\033[0m"
	@$(CXX) $(OBJS) $(C_OBJS) -o $@ $(LIBS) $(FLAGS) && (echo -e "\033[36mDone.\033[0m"; ls -lh $(TARGET_NAME))

clean:
	rm $(OBJS) $(TARGET_NAME)

.PHONY: all clean

