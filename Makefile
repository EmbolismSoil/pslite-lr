INCLUDE := -I../eigen/ 
INCLUDE += -I../ps-lite/include

Q := @
CXX = g++

CXXFLAGS := -std=c++11 -g $(INCLUDE)
LIBS := -lps -lpthread  -lzmq -l:libprotobuf.a
LDFLAGS := -L../ps-lite/build -L../ps-lite/deps/lib

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
