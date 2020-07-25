%.d : %.cpp
	$(Q) set -e;rm -f $@;\
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$;\
	sed 's/\($(notdir $*)\)\.o[ : ]*/\1.o $(notdir $@) : /g' < $@.$$     > $@;\
	rm -f $@.$$
