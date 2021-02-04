/*
    This code makes esp32 as TouchScreen
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LSB(_x) ((_x) & 0xFF)
#define MSB(_x) ((_x) >> 8)

BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
bool connected = false;
int screenX0 = 0;
int screenY0 = 0;
int screenX1 = 1000;
int screenY1 = 1000;
int logicalMinX = 0;
int logicalMinY = 0;
int logicalMaxX = 32767;
int logicalMaxY = 32767;
int logicalX, logicalY;
//uint8_t _switch = 1;

class MyCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      connected = true;
      BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(true);
    }

    void onDisconnect(BLEServer* pServer) {
      connected = false;
      BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(false);
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init("TestTestAiueo");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());

  hid = new BLEHIDDevice(pServer);
  input = hid->inputReport(4); // <-- input REPORTID from report map

  std::string name = "ElectronicAiueo";
  hid->manufacturer()->setValue(name);

  hid->pnp(0x01, 0x0430, 0x0508, 0x00); //pnp(uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version)
  hid->hidInfo(0x00, 0x01); //hidInfo(uint8_t country, uint8_t flags)

  BLESecurity *pSecurity = new BLESecurity();
  //  pSecurity->setKeySize();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  const uint8_t report[] = {
    0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
    0x09, 0x04,                         // USAGE (Touch Screen)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, 0x04,                         //   REPORT_ID (4)
    0x09, 0x22,                         //   USAGE (Finger)
    0xa1, 0x00,                         //   COLLECTION (Physical)
    // tipawitch & untouch
    0x09, 0x42,                         //     USAGE (Tip Switch)
    0x09, 0x32,                         //     USAGE (InRange)
    0x09, 0x33,                         //     USAGE (Touch)
    0x09, 0x34,                         //     USAGE (Untouch)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x04,                         //     REPORT_COUNT (4)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    // dummy for untouch
    0x95, 0x04,                         //     REPORT_COUNT (4)
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)
    // contactID
    0x09, 0x51,                         //     USAGE (Contact Identifier) *added line
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (9)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    // dummy for ContactID
    0x95, 0x07,                         //     REPORT_COUNT (7)
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)
    // X and Y on Desktop
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop)
    0x75, 0x10,                         //     REPORT_SIZE (16)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x55, 0x0d,                         //     UNIT_EXPONENT (-3)
    0x65, 0x33,                         //     UNIT (Inch,EngLinear)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x7f,                   //     LOGICAL_MAXIMUM (32767)
    0x09, 0x30,                         //     USAGE (X)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x09, 0x31,                         //     USAGE (Y)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0xc0,                               //   END_COLLECTION
    0xc0,                               // END_COLLECTION
  };

  hid->reportMap((uint8_t*)report, sizeof(report));
  hid->startServices();

  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue("Hello World says Neil");
  pService->start();
  //   BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAppearance(HID_TABLET);
  pAdvertising->addServiceUUID(SERVICE_UUID);

  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();

}

void loop() {
  // put your main code here, to run repeatedly:
  int untouched = 0;
  if (connected) {
    if (Serial.available() > 0) {
      String func;
      int x;
      int y;
      int u; //untouch
      int c; //contactID
      func = Serial.readStringUntil(';');
      x = func.substring(0, 4).toInt();
      y = func.substring(4, 8).toInt();
      u = func.substring(8, 9).toInt();
      c = func.substring(9).toInt();
      Serial.println(x);
      Serial.println(y);
      Serial.println(u);
      Serial.println(c);
      if (u == 2 and c == 2) {
        zoomin(x, y);
      } else if (u == 3 and c == 3) {
        zoomout(x, y);
      } else if (screenX0 < x and x <= screenX1) {
        if (screenY0 < y and y <= screenY1) {
          if (c >= 0 or c <= 9) {
            if (u == 0 or u == 1) {
              send_multi(x, y, u, c);
            }
          }
        } else {
          Serial.println("y-value: out of range");
        }
      } else {
        Serial.println("x-value: out of range");
      }
      untouched = 0;
    } else {
      delay(500);
      if (untouched == 0) {
        send_null();
        Serial.println("Clearing Touch");
      }
      untouched = 1;
    }
  } else {
    Serial.println("Waiting for Connection");
    delay(500);
  }
}

void send_null() {
  uint8_t m[6];
  m[0] = 0; // untouched
  m[1] = 0; // contactID
  m[2] = 0; // LSB(X)
  m[3] = 0; // MSB(X)
  m[4] = 0; // LSB(Y)
  m[5] = 0; // MSB(Y)
  input->setValue(m, 6);
  input->notify();
}

void send_multi(int paramX, int paramY, int _untouch, int _cid) {
  logicalX = map(paramX, screenX0, screenX1, logicalMinX, logicalMaxX);
  logicalY = map(paramY, screenY0, screenY1, logicalMinY, logicalMaxY);
  uint8_t m[6];
  if (_untouch == 0){
    m[0] = 0x05; // s=1,i=0,t=1,u=0 => 1010 invert 0101
  }else{
    m[0] = 0x08; // s=0,i=0,t=0,u=1 => 0001 invert 1000
  }
  m[1] = _cid; // contactID
  m[2] = LSB(logicalX); // LSB(X)
  m[3] = MSB(logicalX); // MSB(X)
  m[4] = LSB(logicalY); // LSB(Y)
  m[5] = MSB(logicalY); // MSB(Y)
  input->setValue(m, 6);
  input->notify();
}
void zoomin(int paramX, int paramY) {
  if (screenX0 < paramX and paramX <= screenX1) {
    if (screenY0 < paramY and paramY <= screenY1) {
      int x0, x1;
      for (int i = 0; i < 100; i++) {

        x0 = paramX - i * 2 - 100;
        x1 = paramX + i * 2 + 100;
        if (x0 < screenX0) {
          x0 = screenX0;
        }
        if (x1 > screenX1) {
          x1 = screenX1;
        }
        send_multi(x0, paramY, 0, 0);
        send_multi(x1, paramY, 0, 1);
      }
      send_multi(x0, paramY, 1, 0);
      send_multi(x1, paramY, 1, 1);
    } else {
      Serial.println("y-value: out of range");
    }
  } else {
    Serial.println("x-value: out of range");
  }
}
void zoomout(int paramX, int paramY) {
  if (screenX0 < paramX and paramX <= screenX1) {
    if (screenY0 < paramY and paramY <= screenY1) {
      int x0, x1;
      for (int i = 0; i < 100; i++) {

        x0 = paramX + i * 2 - 200;
        x1 = paramX - i * 2 + 200;
        if (x0 < screenX0) {
          x0 = screenX0;
        }
        if (x1 > screenX1) {
          x1 = screenX1;
        }
        send_multi(x0, paramY, 0, 0);
        send_multi(x1, paramY, 0, 1);
      }
      delay(5);
      send_multi(x0, paramY, 1, 0);
      send_multi(x1, paramY, 1, 1);
    } else {
      Serial.println("y-value: out of range");
    }
  } else {
    Serial.println("x-value: out of range");
  }
}
