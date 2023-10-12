// #define RH_TEST_NETWORK 1 // activate Forced Topology

#include <RHMesh.h>
#include <RH_RF95.h>
#include <SPI.h>
// #include <esp_task_wdt.h>

#include <string.h>

#define RF95_FREQ 433.0
#define WDT_TIMEOUT 15

#define SENDING_MODE 0
#define RECEIVING_MODE 1
#define ENDNODE_ADDRESS 2  // purposefully using the last namber

#if defined(SELF_ADDRESS) && defined(TARGET_ADDRESS)
const uint8_t selfAddress_ = SELF_ADDRESS;
const uint8_t targetAddress_ = TARGET_ADDRESS;
#else
// Topology
#define NODE1_ADDRESS 1
#define NODE2_ADDRESS 2
#define NODE3_ADDRESS 3

const uint8_t selfAddress_ = NODE1_ADDRESS;  // CHANGE THIS!!!
const uint8_t targetAddress_ = ENDNODE_ADDRESS;
#endif

//radio driver & message mesh delivery/receipt manager
RH_RF95 RFM95Modem_;
RHMesh RHMeshManager_(RFM95Modem_, selfAddress_);
uint8_t mode_ = SENDING_MODE;

// these are expected to be global/externally exposed variables, if you plan to
// make a class to wrap this
String msgSend =
    String("Hello from node #" + String(selfAddress_) + "!");
String msgRcv;

// void rhSetup();

void setup() {
  Serial.begin(9600);
  while (!Serial) ;
  //esp_task_wdt_init(WDT_TIMEOUT, true);  // enable panic so ESP32 restarts
  //esp_task_wdt_add(NULL);                // add current thread to WDT watch

  rhSetup();
  
  Serial.println(" ---------------- LORA NODE " + String(selfAddress_) + " INIT ---------------- ");
}

long _lastSend = 0, sendInterval_ = 3000;  // send every 10 seconds
uint8_t _msgRcvBuf[RH_MESH_MAX_MESSAGE_LEN];
char buf_[RH_MESH_MAX_MESSAGE_LEN];

void loop() {
  uint8_t _msgFrom;
  uint8_t _msgRcvBufLen = sizeof(_msgRcvBuf);
  
  if ((millis() - _lastSend > sendInterval_) &&
      selfAddress_ != ENDNODE_ADDRESS) {
    mode_ = SENDING_MODE;
  }

  if (mode_ == SENDING_MODE) {
    
    // Send a message to another rhmesh node
    Serial.print("Sending msg ");
    Serial.print(msgSend); 
    Serial.print(" to node #");
    Serial.println(String(targetAddress_));
    uint8_t _err =
        RHMeshManager_.sendtoWait(reinterpret_cast<uint8_t *>(&msgSend[0]), msgSend.length(), targetAddress_);
    if (_err == RH_ROUTER_ERROR_NONE) {
      // message successfully be sent to the target node, or next neighboring
      // expecting to recieve a simple reply from the target node
      //esp_task_wdt_reset();
      Serial.println("Successfull! Awaiting for Reply");

      if (RHMeshManager_.recvfromAckTimeout(_msgRcvBuf, &_msgRcvBufLen, 4000,
                                            &_msgFrom)) {
        char buf_[RH_MESH_MAX_MESSAGE_LEN];

        //Serial.println(buf_);
        sprintf(buf_, "%s", reinterpret_cast<char *>(_msgRcvBuf));
        Serial.print("Message was from node #");
        Serial.println(String(_msgFrom));
        Serial.println(buf_);
        // Serial.printf("[%d] \"%s\" (%d). Sending a reply...\n", _msgFrom, msgRcv.c_str(), RFM95Modem_.lastRssi());

      } else {
        Serial.println("No reply, is the target node running?");
      }

     // esp_task_wdt_reset();
    } else {
      Serial.println(
          "sendtoWait failed. No response from intermediary node, are they "
          "running?");
    }
    _lastSend = millis();
    mode_ = RECEIVING_MODE;
    //delay(100);
  }

  if (mode_ == RECEIVING_MODE) {
    // while at it, wait for a message from other nodes
    Serial.println("Receiving mode active");

    if (RHMeshManager_.recvfromAckTimeout(_msgRcvBuf, &_msgRcvBufLen, 3000,
                                          &_msgFrom)) {
      char buf_[RH_MESH_MAX_MESSAGE_LEN];

//      esp_task_wdt_reset();
      Serial.println("Received a message");
      printf(buf_, "%s", reinterpret_cast<char *>(_msgRcvBuf));
      Serial.println(buf_);
      msgRcv = String(buf_);

      // do something with message, for example pass it through a callback
      // printf("[%d] \"%s\" (%d). Sending a reply...\n", _msgFrom,
      //               msgRcv.c_str(), RFM95Modem_.lastRssi());

      String _msgRply =
          String("Hi node " + String(_msgFrom) + ", got the message!").c_str();
      uint8_t _err = RHMeshManager_.sendtoWait(
          reinterpret_cast<uint8_t *>(&_msgRply[0]), _msgRply.length(), _msgFrom);
      if (_err == RH_ROUTER_ERROR_NONE) {
        // message successfully received by either final target node, or next
        // neighboring node. do nothing...
      } else {
        Serial.println("Fail to send reply...");
      }

     // esp_task_wdt_reset();
    }
  }

 // esp_task_wdt_reset();
}

void rhSetup() {
  if (!RHMeshManager_.init()) Serial.println("init failed");
  RFM95Modem_.setTxPower(23, false);
  RFM95Modem_.setFrequency(RF95_FREQ);
  RFM95Modem_.setCADTimeout(500);
}
