CXX=g++
CXXFLAGS=-std=c++17 -I/usr/local/include/antlr4-runtime
LDFLAGS=-L/usr/local/lib -lantlr4-runtime

SRCS=main.cpp TmplangBaseListener.cpp TmplangLexer.cpp TmplangListener.cpp TmplangParser.cpp Type.cpp SymbolTable.cpp HMTypeInference.cpp
OBJS=$(subst .cpp,.o,$(SRCS))


main: $(OBJS)
	$(CXX) $(LDFLAGS) -o main $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	rm -f $(OBJS) main

distclean: clean
	rm -f *~ .depend

include .depend
