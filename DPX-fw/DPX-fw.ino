/* 
  WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WiFi library for Arduino environment.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

extern "C" {
#include "eagle_soc.h"
#include "uart_register.h"
}

const char* ssid = "progressbar.sk 2G-unifi";
const char* password = "parketbar";

WiFiServer server(23);
WiFiClient serverClient;

void setup() {
  Serial1.begin(74880);
  WiFi.hostname("DPX-3300");
  WiFi.begin(ssid, password);
  //Serial1.print("\nConnecting to "); Serial1.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    //Serial1.print("Could not connect to"); Serial1.println(ssid);
    while(1) delay(500);
  }
  //start UART and the server
  Serial.begin(9600);
  Serial.swap();
  
  // setup serial RTS/CTS hardware handshake
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
  SET_PERI_REG_BITS(UART_CONF1(0), UART_RX_FLOW_THRHD, 63, UART_RX_FLOW_THRHD_S);
  SET_PERI_REG_MASK(UART_CONF1(0), UART_RX_FLOW_EN);
  
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
  SET_PERI_REG_MASK(UART_CONF0(0), UART_TX_FLOW_EN);

  //Serial.write(27);
  //Serial.print(".@;1:");
  
  server.begin();
  
  Serial1.println(WiFi.localIP());
  
  ArduinoOTA.onStart([]() {
    //Serial1.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    //Serial1.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial1.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    /*Serial1.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial1.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial1.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial1.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial1.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial1.println("End Failed");*/
  });
  
  ArduinoOTA.setHostname("DPX-3300");
  ArduinoOTA.begin();
}

void loop() {
  
  //check if there are any new clients
  while(server.hasClient())
  {
    if (serverClient)
    {
      if (serverClient.connected())
      {
        WiFiClient tmp_client = server.available();
        tmp_client.stop();
        continue;
      }
      else
      {
        serverClient.stop();
      }
    }
    serverClient = server.available();
    Serial1.print("client connected");
  }
  
  
  
  //check client for data
  if (serverClient && serverClient.connected())
  {
    if(serverClient.available())
    {
      //get data from the telnet client and push it to the UART
      while (Serial.availableForWrite())
      {
        if (serverClient.available())
          Serial.write(serverClient.read());
        else
          break;
      }
    }
  }
  
  //check UART for data
  if(size_t len = Serial.available())
  {
    #define BUFF_LEN 64
    uint8_t sbuf[BUFF_LEN];
    len = _min(len, BUFF_LEN);
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    
    if (serverClient && serverClient.connected())
    {
      serverClient.write(const_cast<const uint8_t*>(sbuf), len);
    }
  }
  ArduinoOTA.handle();
}
