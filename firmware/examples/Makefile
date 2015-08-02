TARGET  = firmware.bin
SOURCES = main.ino

all : $(TARGET)

$(TARGET): $(SOURCES)
	particle compile $(SOURCES) $(TARGET)

install: $(TARGET)
	particle flash $(SPARK_CORE_ID) $(TARGET)

clean:
	rm -f $(TARGET)

check-syntax:
	particle compile $(SOURCES) $(TARGET) | sed s/cpp/ino/
