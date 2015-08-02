#include "AsebaLite.h"
#include <string.h>

static uint8 buffer[ASEBA_MAX_INNER_PACKET_SIZE];
static uint16 buffer_pos;
static uint16 read_pos;
static char string_buffer[ASEBA_MAX_INNER_PACKET_SIZE];

// From Aseba vm-buffer.cpp

static void buffer_add(const uint8* data, const uint16 len)
{
  uint16 i = 0;
  while (i < len)
    buffer[buffer_pos++] = data[i++];
}

static void buffer_add_uint8(const uint8 value)
{
  buffer_add(&value, 1);
}

static void buffer_add_uint16(const uint16 value)
{
  const uint16 temp = bswap16(value);
  buffer_add((const unsigned char *) &temp, 2);
}

static void buffer_add_sint16(const sint16 value)
{
  const uint16 temp = bswap16(value);
  buffer_add((const unsigned char *) &temp, 2);
}

static void buffer_add_string(const char* s)
{
  uint16 len = strlen(s);
  buffer_add_uint8((uint8)len);
  while (*s)
    buffer_add_uint8(*s++);
}

// From vm-buffer.cpp, modified for Particle

template<typename T>
T get()
{
  uint16 pos = read_pos;
  read_pos += sizeof(T);
  T val;
  uint8 *ptr = reinterpret_cast<uint8 *>(&val);
  memcpy(ptr, buffer + pos, sizeof(T));
  return val;
}
	
template<>
String get()
{
  String s;
  size_t len = get<uint8>();
  s.reserve(len);
  for (size_t i = 0; i < len; i++) {
    s +=  (char)get<uint8>();
  }
  return s;
}


// Constructor
AsebaLite::AsebaBuffer::AsebaBuffer()
{
  verbose = true;
}
AsebaLite::AsebaBuffer::AsebaBuffer(bool _verbose)
{
  verbose = _verbose;
}

// Initializers that should be called in the `setup()` function
// void AsebaLite::AsebaBuffer::setTcpServer(TCPServer tcpserver)
// {
//   aseba_tcp = tcpserver;
// }

// Main API functions that the library provides
// typically called in `loop()` or `setup()` functions

void AsebaLite::AsebaBuffer::resetBuffer() {
  read_pos = 0;
}

void AsebaLite::AsebaBuffer::setBuffer(const uint8* data, uint16 length) {
  memcpy((uint8*)buffer, data, length);
  read_pos = 0;
}

void AsebaLite::AsebaBuffer::appendBuffer(const uint8* data, uint16 length) {
  memcpy((uint8*)buffer + buffer_pos, data, length);
  read_pos += length;
}

uint16 AsebaLite::AsebaBuffer::getBuffer(uint8* data, uint16 maxLength, uint16* source) {
  uint16 payload_length;
  memcpy(&payload_length, buffer, 2);
  memcpy( source, buffer, 2);
  uint16 length = payload_length + 6;
  length = (length < maxLength) ? length : maxLength;
  memcpy(data, (uint8*)buffer, length);
  return length;
}

uint16 AsebaLite::AsebaBuffer::getBufferPayload(uint8 * data, uint16 maxLength) {
  uint16 payload_length;
  memcpy(&payload_length, buffer, 2);
  Serial.println("getBufferPayload length = " + String(payload_length));
  payload_length = (payload_length < maxLength - 6) ? payload_length : maxLength;
  if (payload_length > 0)
    memcpy(data, (uint8*)buffer + 6, payload_length);
  return payload_length;
}

void AsebaLite::AsebaBuffer::writeBufferAll(TCPServer aseba_tcp) {
  Spark.publish("beacon","writing "+String(buffer_pos)+" from buffer");
  Serial1.write(buffer, buffer_pos);
  //aseba_tcp.write(buffer, buffer_pos);
  parseBuffer();
}

bool AsebaLite::AsebaBuffer::messageReady() {
  if (buffer_pos >= 6) {
    uint16 payload_length;
    memcpy(&payload_length, buffer, 2);
    if (buffer_pos >= payload_length + 6)
      return true;
  }
  return false;
}

void AsebaLite::AsebaBuffer::parseBuffer() {
  int data[200];
  sint16 psize[10];
  String pname[10];
  bool verbose = true;
    
  read_pos = 0;
  uint16 length = get<uint16>();
  uint16 source = get<uint16>();
  uint16 type   = get<uint16>();
  if (verbose)
    Serial.println("# recv frame len " + String(length) + " source " + String(source) + " type 0x" + String(type, HEX));

  // Parse known messages
  switch(type) {
  case 0x8000: { // ASEBA_MESSAGE_BOOTLOADER_RESET
    if (! verbose)
      break;
    Serial.println("0x8000 bootloader reset");
    break;
  }
  case 0x8001: { // ASEBA_MESSAGE_BOOTLOADER_READ_PAGE
    uint16 pageNumber = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x8001 bootloader read page");
    Serial.println(" pageNumber "+String(pageNumber,DEC));
    break;
  }
  case 0x8002: { // ASEBA_MESSAGE_BOOTLOADER_WRITE_PAGE
    uint16 pageNumber = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x8002 bootloader write page");
    Serial.println(" pageNumber "+String(pageNumber,DEC));
    break;
  }
  case 0x8003: { // ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_WRITE
    unsigned int limit = (length - (read_pos-6));
    for(unsigned int i=0; i<limit; i++) {
      data[i] = get<uint8>(); // data buffer is static uint8 []
    }
    if (! verbose)
      break;
    Serial.print("0x8003 bootloader page data write");
    for(unsigned int i=0; i<limit; i++) {
      Serial.print(" "+String(data[i],DEC));
    }
    Serial.println();
    break;
  }
  case 0x8004: { // ASEBA_MESSAGE_BOOTLOADER_DESCRIPTION
    uint16 pageSize = get<uint16>();
    uint16 pageStart = get<uint16>();
    uint16 pageCount = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x8004 bootloader description");
    Serial.print(" pageSize "+String(pageSize,DEC));
    Serial.print(" pageStart "+String(pageStart,DEC));
    Serial.println(" pageCount "+String(pageCount,DEC));
    break;
  }
  case 0x8005: { // ASEBA_MESSAGE_BOOTLOADER_PAGE_DATA_READ
    unsigned int limit = (length - (read_pos-6));
    for(unsigned int i=0; i<limit; i++) {
      data[i] = get<uint8>(); // data buffer is static uint8 []
    }
    if (! verbose)
      break;
    Serial.print("0x8005 bootloader page data read");
    for(unsigned int i=0; i<limit; i++) {
      Serial.print(" "+String(data[i],DEC));
    }
    Serial.println();
    break;
  }
  case 0x8006: { // ASEBA_MESSAGE_BOOTLOADER_ACK
    uint16 errorCode = get<uint16>();
    uint16 errorAddress = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x8006 bootloader ack");
    Serial.print(" errorCode "+String(errorCode,DEC));
    Serial.println(" errorAddress "+String(errorAddress,DEC));
    break;
  }
  case 0x9000: { // ASEBA_MESSAGE_DESCRIPTION
    String name = get<String>();
    uint16 protocolVersion = get<uint16>();
    uint16 bytecodeSize = get<uint16>();
    uint16 stackSize = get<uint16>();
    uint16 variablesSize = get<uint16>();
    uint16 num_namedVariables = get<uint16>();
    uint16 num_localEvents = get<uint16>();
    uint16 num_nativeFunctions = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x9000 description");
    Serial.print(" name "+name);
    Serial.print(" protocolVersion "+String(protocolVersion,DEC));
    Serial.print(" bytecodeSize "+String(bytecodeSize,DEC));
    Serial.print(" stackSize "+String(stackSize,DEC));
    Serial.print(" variablesSize "+String(variablesSize,DEC));
    Serial.print(" num_namedVariables "+String(num_namedVariables,DEC));
    Serial.print(" num_localEvents "+String(num_localEvents,DEC));
    Serial.println(" num_nativeFunctions "+String(num_nativeFunctions,DEC));
    break;
  }
  case 0x9001: { // ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION
    uint16 size = get<uint16>();
    String name = get<String>();
    if (! verbose)
      break;
    Serial.print("0x9001 named variable description");
    Serial.print(" size "+String(size,DEC));
    Serial.println(" name "+name);
    break;
  }
  case 0x9002: { // ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION
    String name = get<String>();
    String description = get<String>();
    if (! verbose)
      break;
    Serial.print("0x9002 local event description");
    Serial.print(" name "+name);
    Serial.println(" description "+description);
    break;
  }
  case 0x9003: { // ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION
    String name = get<String>();
    String description = get<String>();
    uint16 num_params = get<uint16>();
    unsigned int limit = num_params;
    for(unsigned int i=0; i<limit; i++) {
      psize[i] = get<sint16>(); // psize buffer is static sint16 []
      pname[i] = get<String>(); // pname buffer is static String []
    }
    if (! verbose)
      break;
    Serial.print("0x9003 native function description");
    Serial.print(" name "+name);
    Serial.print(" description "+description);
    Serial.print(" num_params "+String(num_params,DEC));
    for(unsigned int i=0; i<limit; i++) {
      Serial.print(" "+String(psize[i],DEC));
      Serial.print(" "+pname[i]);
    }
    Serial.println();
    break;
  }
  case 0x9004: { // ASEBA_MESSAGE_DISCONNECTED
    if (! verbose)
      break;
    Serial.println("0x9004 disconnected");
    break;
  }
  case 0x9005: { // ASEBA_MESSAGE_VARIABLES
    uint16 start = get<uint16>();
    unsigned int limit = (length - (read_pos-6)) / 2;
    for(unsigned int i=0; i<limit; i++) {
      data[i] = get<sint16>(); // data buffer is static sint16 []
    }
    if (! verbose)
      break;
    Serial.print("0x9005 variables");
    Serial.print(" start "+String(start,DEC));
    for(unsigned int i=0; i<limit; i++) {
      Serial.print(" "+String(data[i],DEC));
    }
    Serial.println();
    break;
  }
  case 0x9006: { // ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS
    uint16 pc = get<uint16>();
    uint16 size = get<uint16>();
    uint16 index = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x9006 array access out of bounds");
    Serial.print(" pc "+String(pc,DEC));
    Serial.print(" size "+String(size,DEC));
    Serial.println(" index "+String(index,DEC));
    break;
  }
  case 0x9007: { // ASEBA_MESSAGE_DIVISION_BY_ZERO
    uint16 pc = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x9007 division by zero");
    Serial.println(" pc "+String(pc,DEC));
    break;
  }
  case 0x9008: { // ASEBA_MESSAGE_EVENT_EXECUTION_KILLED
    uint16 pc = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x9008 event execution killed");
    Serial.println(" pc "+String(pc,DEC));
    break;
  }
  case 0x9009: { // ASEBA_MESSAGE_NODE_SPECIFIC_ERROR
    uint16 pc = get<uint16>();
    String message = get<String>();
    if (! verbose)
      break;
    Serial.print("0x9009 node specific error");
    Serial.print(" pc "+String(pc,DEC));
    Serial.println(" message "+message);
    break;
  }
  case 0x900a: { // ASEBA_MESSAGE_EXECUTION_STATE_CHANGED
    uint16 pc = get<uint16>();
    uint16 flags = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x900a execution state changed");
    Serial.print(" pc "+String(pc,DEC));
    Serial.println(" flags "+String(flags,DEC));
    break;
  }
  case 0x900b: { // ASEBA_MESSAGE_BREAKPOINT_SET_RESULT
    uint16 pc = get<uint16>();
    uint16 success = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0x900b breakpoint set result");
    Serial.print(" pc "+String(pc,DEC));
    Serial.println(" success "+String(success,DEC));
    break;
  }
  case 0xa000: { // ASEBA_MESSAGE_GET_DESCRIPTION
    uint16 version = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0xa000 get description");
    Serial.println(" version "+String(version,DEC));
    break;
  }
  case 0xa001: { // ASEBA_MESSAGE_SET_BYTECODE
    uint16 start = get<uint16>();
    unsigned int limit = (length - (read_pos-6)) / 2;
    for(unsigned int i=0; i<limit; i++) {
      data[i] = get<uint16>(); // data buffer is static uint16 []
    }
    if (! verbose)
      break;
    Serial.print("0xa001 set bytecode");
    Serial.print(" start "+String(start,DEC));
    for(unsigned int i=0; i<limit; i++) {
      Serial.print(" "+String(data[i],DEC));
    }
    Serial.println();
    break;
  }
  case 0xa002: { // ASEBA_MESSAGE_RESET
    if (! verbose)
      break;
    Serial.println("0xa002 reset");
    break;
  }
  case 0xa003: { // ASEBA_MESSAGE_RUN
    if (! verbose)
      break;
    Serial.println("0xa003 run");
    break;
  }
  case 0xa004: { // ASEBA_MESSAGE_PAUSE
    if (! verbose)
      break;
    Serial.println("0xa004 pause");
    break;
  }
  case 0xa005: { // ASEBA_MESSAGE_STEP
    if (! verbose)
      break;
    Serial.println("0xa005 step");
    break;
  }
  case 0xa006: { // ASEBA_MESSAGE_STOP
    if (! verbose)
      break;
    Serial.println("0xa006 stop");
    break;
  }
  case 0xa007: { // ASEBA_MESSAGE_GET_EXECUTION_STATE
    if (! verbose)
      break;
    Serial.println("0xa007 get execution state");
    break;
  }
  case 0xa008: { // ASEBA_MESSAGE_BREAKPOINT_SET
    uint16 pc = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0xa008 breakpoint set");
    Serial.println(" pc "+String(pc,DEC));
    break;
  }
  case 0xa009: { // ASEBA_MESSAGE_BREAKPOINT_CLEAR
    uint16 pc = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0xa009 breakpoint clear");
    Serial.println(" pc "+String(pc,DEC));
    break;
  }
  case 0xa00a: { // ASEBA_MESSAGE_BREAKPOINT_CLEAR_ALL
    if (! verbose)
      break;
    Serial.println("0xa00a breakpoint clear all");
    break;
  }
  case 0xa00b: { // ASEBA_MESSAGE_GET_VARIABLES
    uint16 start = get<uint16>();
    uint16 length = get<uint16>();
    if (! verbose)
      break;
    Serial.print("0xa00b get variables");
    Serial.print(" start "+String(start,DEC));
    Serial.println(" length "+String(length,DEC));
    break;
  }
  case 0xa00c: { // ASEBA_MESSAGE_SET_VARIABLES
    uint16 start = get<uint16>();
    unsigned int limit = (length - (read_pos-6)) / 2;
    for(unsigned int i=0; i<limit; i++) {
      data[i] = get<sint16>(); // data buffer is static sint16 []
    }
    if (! verbose)
      break;
    Serial.print("0xa00c set variables");
    Serial.print(" start "+String(start,DEC));
    for(unsigned int i=0; i<limit; i++) {
      Serial.print(" "+String(data[i],DEC));
    }
    Serial.println();
    break;
  }
  case 0xa00d: { // ASEBA_MESSAGE_WRITE_BYTECODE
    if (! verbose)
      break;
    Serial.println("0xa00d write bytecode");
    break;
  }
  case 0xa00e: { // ASEBA_MESSAGE_REBOOT
    if (! verbose)
      break;
    Serial.println("0xa00e reboot");
    break;
  }
  case 0xa00f: { // ASEBA_MESSAGE_SUSPEND_TO_RAM
    if (! verbose)
      break;
    Serial.println("0xa00f suspend to ram");
    break;
  }
  case 0xffff: { // ASEBA_MESSAGE_INVALID
    if (! verbose)
      break;
    Serial.println("0xffff invalid");
    break;
  }
  default: {
    if (! verbose)
      break;
    if (type < 0x8000)
      Serial.println(String(type,HEX) + " user message");
    else
      Serial.println(String(type,HEX) + " unknown message");
  }
  } // switch type

  unsigned int payload = length - (read_pos-6);
  if (verbose && payload > 0) {
    Serial.println("# payload " + String(payload) + " = " + String(length) + " - (" + String(read_pos) + " - 6)");
    for (unsigned int i=0;i<payload;i++) {
      uint16 data = get<uint8>();
      Serial.print(data, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void AsebaLite::AsebaBuffer::messageUserMessage(int type, std::vector<sint16> data) {
    buffer_pos = 0;
	buffer_add_uint16(0 + data.size()*2); // length in bytes
	buffer_add_uint16(0x0009); // source
	buffer_add_uint16(type); // type
	for (std::vector<sint16>::iterator it = data.begin(); it != data.end(); ++it)
        buffer_add_sint16( *it );
}

void AsebaLite::AsebaBuffer::messageBootloaderReset() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x8000); // message type
}
void AsebaLite::AsebaBuffer::messageBootloaderReadPage(uint16 pageNumber) {
  buffer_pos = 0;
  buffer_add_uint16(2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x8001); // message type
  buffer_add_uint16(pageNumber);
}
void AsebaLite::AsebaBuffer::messageBootloaderWritePage(uint16 pageNumber) {
  buffer_pos = 0;
  buffer_add_uint16(2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x8002); // message type
  buffer_add_uint16(pageNumber);
}
void AsebaLite::AsebaBuffer::messageBootloaderPageDataWrite(std::vector<uint8> data) {
  buffer_pos = 0;
  buffer_add_uint16(data.size()); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x8003); // message type
  for(unsigned int i=0; i<data.size(); i++) {
    buffer_add_uint8(data[i]);
  }
}
void AsebaLite::AsebaBuffer::messageBootloaderDescription(uint16 pageSize, uint16 pageStart, uint16 pageCount) {
  buffer_pos = 0;
  buffer_add_uint16(2 + 2 + 2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x8004); // message type
  buffer_add_uint16(pageSize);
  buffer_add_uint16(pageStart);
  buffer_add_uint16(pageCount);
}
void AsebaLite::AsebaBuffer::messageBootloaderPageDataRead(std::vector<uint8> data) {
  buffer_pos = 0;
  buffer_add_uint16(data.size()); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x8005); // message type
  for(unsigned int i=0; i<data.size(); i++) {
    buffer_add_uint8(data[i]);
  }
}
void AsebaLite::AsebaBuffer::messageBootloaderAck(uint16 errorCode, uint16 errorAddress) {
  buffer_pos = 0;
  buffer_add_uint16(2 + 2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x8006); // message type
  buffer_add_uint16(errorCode);
  buffer_add_uint16(errorAddress);
}
void AsebaLite::AsebaBuffer::messageDescription(String name, uint16 protocolVersion, uint16 bytecodeSize, uint16 stackSize, uint16 variablesSize, uint16 num_namedVariables, uint16 num_localEvents, uint16 num_nativeFunctions) {
  buffer_pos = 0;
  buffer_add_uint16(name.length()+1 + 2 + 2 + 2 + 2 + 2 + 2 + 2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9000); // message type
  name.toCharArray(string_buffer,name.length()+1);
  buffer_add_string(string_buffer);
  buffer_add_uint16(protocolVersion);
  buffer_add_uint16(bytecodeSize);
  buffer_add_uint16(stackSize);
  buffer_add_uint16(variablesSize);
  buffer_add_uint16(num_namedVariables);
  buffer_add_uint16(num_localEvents);
  buffer_add_uint16(num_nativeFunctions);
}
void AsebaLite::AsebaBuffer::messageNamedVariableDescription(uint16 size, String name) {
  buffer_pos = 0;
  buffer_add_uint16(2 + name.length()+1); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9001); // message type
  buffer_add_uint16(size);
  name.toCharArray(string_buffer,name.length()+1);
  buffer_add_string(string_buffer);
}
void AsebaLite::AsebaBuffer::messageLocalEventDescription(String name, String description) {
  buffer_pos = 0;
  buffer_add_uint16(name.length()+1 + description.length()+1); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9002); // message type
  name.toCharArray(string_buffer,name.length()+1);
  buffer_add_string(string_buffer);
  description.toCharArray(string_buffer,description.length()+1);
  buffer_add_string(string_buffer);
}
void AsebaLite::AsebaBuffer::messageNativeFunctionDescription(String name, String description, uint16 num_params, std::vector<sint16> psize, std::vector<String> pname) {
  buffer_pos = 0;
  uint16 payload = name.length()+1 + description.length()+1 + 2 + psize.size()*2;
  for (std::vector<String>::iterator it = pname.begin(); it != pname.end(); ++it)
    payload += it->length() + 1;
  buffer_add_uint16(payload); // payload length
  buffer_add_uint16(0x0009);  // source node
  buffer_add_uint16(0x9003);   // message type
  name.toCharArray(string_buffer,name.length()+1);
  buffer_add_string(string_buffer);
  description.toCharArray(string_buffer,description.length()+1);
  buffer_add_string(string_buffer);
  buffer_add_uint16(num_params);
  for(unsigned int i=0; i<psize.size(); i++) {
    buffer_add_sint16(psize[i]);
    pname[i].toCharArray(string_buffer,pname[i].length()+1);
    buffer_add_string(string_buffer);
  }
}
void AsebaLite::AsebaBuffer::messageDisconnected() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9004); // message type
}
void AsebaLite::AsebaBuffer::messageVariables(uint16 start, std::vector<sint16> data) {
  buffer_pos = 0;
  buffer_add_uint16(2 + data.size()*2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9005); // message type
  buffer_add_sint16(start);
  for(unsigned int i=0; i<data.size(); i++) {
    buffer_add_sint16(data[i]);
  }
}
void AsebaLite::AsebaBuffer::messageArrayAccessOutOfBounds(uint16 pc, uint16 size, uint16 index) {
  buffer_pos = 0;
  buffer_add_uint16(2 + 2 + 2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9006); // message type
  buffer_add_uint16(pc);
  buffer_add_uint16(size);
  buffer_add_uint16(index);
}
void AsebaLite::AsebaBuffer::messageDivisionByZero(uint16 pc) {
  buffer_pos = 0;
  buffer_add_uint16(2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9007); // message type
  buffer_add_uint16(pc);
}
void AsebaLite::AsebaBuffer::messageEventExecutionKilled(uint16 pc) {
  buffer_pos = 0;
  buffer_add_uint16(2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9008); // message type
  buffer_add_uint16(pc);
}
void AsebaLite::AsebaBuffer::messageNodeSpecificError(uint16 pc, String message) {
  buffer_pos = 0;
  buffer_add_uint16(2 + message.length()+1); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x9009); // message type
  buffer_add_uint16(pc);
  message.toCharArray(string_buffer,message.length()+1);
  buffer_add_string(string_buffer);
}
void AsebaLite::AsebaBuffer::messageExecutionStateChanged(uint16 pc, uint16 flags) {
  buffer_pos = 0;
  buffer_add_uint16(2 + 2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x900a); // message type
  buffer_add_uint16(pc);
  buffer_add_uint16(flags);
}
void AsebaLite::AsebaBuffer::messageBreakpointSetResult(uint16 pc, uint16 success) {
  buffer_pos = 0;
  buffer_add_uint16(2 + 2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0x900b); // message type
  buffer_add_uint16(pc);
  buffer_add_uint16(success);
}
void AsebaLite::AsebaBuffer::messageGetDescription(uint16 version) {
  buffer_pos = 0;
  buffer_add_uint16(2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa000); // message type
  buffer_add_uint16(version);
}
void AsebaLite::AsebaBuffer::messageSetBytecode(uint16 start, std::vector<uint16> data) {
  buffer_pos = 0;
  buffer_add_uint16(2 + data.size()*2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa001); // message type
  buffer_add_uint16(start);
  for(unsigned int i=0; i<data.size(); i++) {
    buffer_add_uint16(data[i]);
  }
}
void AsebaLite::AsebaBuffer::messageReset() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa002); // message type
}
void AsebaLite::AsebaBuffer::messageRun() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa003); // message type
}
void AsebaLite::AsebaBuffer::messagePause() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa004); // message type
}
void AsebaLite::AsebaBuffer::messageStep() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa005); // message type
}
void AsebaLite::AsebaBuffer::messageStop() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa006); // message type
}
void AsebaLite::AsebaBuffer::messageGetExecutionState() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa007); // message type
}
void AsebaLite::AsebaBuffer::messageBreakpointSet(uint16 pc) {
  buffer_pos = 0;
  buffer_add_uint16(2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa008); // message type
  buffer_add_uint16(pc);
}
void AsebaLite::AsebaBuffer::messageBreakpointClear(uint16 pc) {
  buffer_pos = 0;
  buffer_add_uint16(2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa009); // message type
  buffer_add_uint16(pc);
}
void AsebaLite::AsebaBuffer::messageBreakpointClearAll() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa00a); // message type
}
void AsebaLite::AsebaBuffer::messageGetVariables(uint16 start, uint16 length) {
  buffer_pos = 0;
  buffer_add_uint16(2 + 2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa00b); // message type
  buffer_add_uint16(start);
  buffer_add_uint16(length);
}
void AsebaLite::AsebaBuffer::messageSetVariables(uint16 start, std::vector<sint16> data) {
  buffer_pos = 0;
  buffer_add_uint16(2 + data.size()*2); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa00c); // message type
  buffer_add_uint16(start);
  for(unsigned int i=0; i<data.size(); i++) {
    buffer_add_sint16(data[i]);
  }
}
void AsebaLite::AsebaBuffer::messageWriteBytecode() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa00d); // message type
}
void AsebaLite::AsebaBuffer::messageReboot() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa00e); // message type
}
void AsebaLite::AsebaBuffer::messageSuspendToRam() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xa00f); // message type
}
void AsebaLite::AsebaBuffer::messageInvalid() {
  buffer_pos = 0;
  buffer_add_uint16(0); // payload length
  buffer_add_uint16(0x0009); // source node
  buffer_add_uint16(0xffff); // message type
}
