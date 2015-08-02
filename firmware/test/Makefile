CPP = g++
RM = rm -f
INCLUDE_DIRS += ..

TARGET  = test1
SOURCES = test1.cpp

all : $(TARGET)

$(TARGET) : $(SOURCES)
	$(CPP) -I $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES)

clean :
	rm $(TARGET) *.o

test :
	./$(TARGET)
