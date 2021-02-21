all: concluding_task.cpp 
	g++ concluding_task.cpp -o concluding_task 
all-GDB: concluding_task.cpp 
	g++ -g concluding_task.cpp -o concluding_task