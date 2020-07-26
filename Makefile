
Q := @
CXX = g++

EIGEN_PATH := ../eigen
PSLITE_PATH := ../ps-lite

INCLUDE := -I$(EIGEN_PATH)
INCLUDE += -I$(PSLITE_PATH)/include

CXXFLAGS := -std=c++11 -g $(INCLUDE)
LIBS := -lps -lpthread  -lzmq -l:libprotobuf.a
LDFLAGS := -L$(PSLITE_PATH)/build -L$(PSLITE_PATH)/deps/lib

SRC = $(wildcard ./*.cpp)

DEP := $(SRC:%.cpp=%.d)
OBJS := $(SRC:%.cpp=%.o)

pslite-lr: $(OBJS) $(DEP)
	$(CXX)  $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	$(Q) -rm $(shell find ./ -name *.o)
	$(Q) -rm $(shell find ./ -name *.d)

sinclude ./rules.mk
sinclude $(DEP)
