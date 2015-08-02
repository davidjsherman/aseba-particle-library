#ifndef _ASEBA_LITE_PARTICLE_LIBRARY
#define _ASEBA_LITE_PARTICLE_LIBRARY

#include "application.h"
#include "common/consts.h"
#include "common/types.h"
#include <vector>

namespace AsebaLite
{
  class AsebaBuffer
  {
    private:
      bool verbose;
      //TCPServer aseba_tcp;
    public:
      AsebaBuffer();
      AsebaBuffer(bool verbose);
      //void setTcpServer(TCPServer tcpserver);

      void resetBuffer(const uint8* data, uint16 length);
      void setBuffer(const uint8* data, uint16 length);
      void appendBuffer(const uint8* data, uint16 length);
      uint16 getBuffer(uint8* data, uint16 maxLength, uint16* source);
      uint16 getBufferPayload(uint8 * data, uint16 maxLength);

      bool messageReady();
      
      void parseBuffer();
      void writeBufferAll(TCPServer aseba_tcp);

      void messageUserMessage(int type, std::vector<sint16> data);
      void messageBootloaderReset();
      void messageBootloaderReadPage(uint16 pageNumber);
      void messageBootloaderWritePage(uint16 pageNumber);
      void messageBootloaderPageDataWrite(std::vector<uint8> data);
      void messageBootloaderDescription(uint16 pageSize, uint16 pageStart, uint16 pageCount);
      void messageBootloaderPageDataRead(std::vector<uint8> data);
      void messageBootloaderAck(uint16 errorCode, uint16 errorAddress);
      void messageDescription(String name, uint16 protocolVersion, uint16 bytecodeSize, uint16 stackSize, uint16 variablesSize, uint16 num_namedVariables, uint16 num_localEvents, uint16 num_nativeFunctions);
      void messageNamedVariableDescription(uint16 size, String name);
      void messageLocalEventDescription(String name, String description);
      void messageNativeFunctionDescription(String name, String description, uint16 num_params, std::vector<sint16> psize, std::vector<String> pname);
      void messageDisconnected();
      void messageVariables(uint16 start, std::vector<sint16> data);
      void messageArrayAccessOutOfBounds(uint16 pc, uint16 size, uint16 index);
      void messageDivisionByZero(uint16 pc);
      void messageEventExecutionKilled(uint16 pc);
      void messageNodeSpecificError(uint16 pc, String message);
      void messageExecutionStateChanged(uint16 pc, uint16 flags);
      void messageBreakpointSetResult(uint16 pc, uint16 success);
      void messageGetDescription(uint16 version);
      void messageSetBytecode(uint16 start, std::vector<uint16> data);
      void messageReset();
      void messageRun();
      void messagePause();
      void messageStep();
      void messageStop();
      void messageGetExecutionState();
      void messageBreakpointSet(uint16 pc);
      void messageBreakpointClear(uint16 pc);
      void messageBreakpointClearAll();
      void messageGetVariables(uint16 start, uint16 length);
      void messageSetVariables(uint16 start, std::vector<sint16> data);
      void messageWriteBytecode();
      void messageReboot();
      void messageSuspendToRam();
      void messageInvalid();
  };
}

#endif
