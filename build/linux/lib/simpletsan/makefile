default: objects/simpletsan.d.o objects/simpletsan-templates.d.o

objects/simpletsan.d.o: ../../../../source/Lib/simpletsan/simpletsan.cpp  ../../../../source/Lib/simpletsan/simpletsan.h
	mkdir -p objects
	$(CXX) -std=c++11 -O0 -g $< -fno-implicit-templates -c -o $@

objects/simpletsan-templates.d.o: ../../../../source/Lib/simpletsan/simpletsan-templates.cpp ../../../../source/Lib/simpletsan/simpletsan.h
	mkdir -p objects
	$(CXX) -std=c++11 -O0 -g $< -fsanitize=thread -c -o $@
