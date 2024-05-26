/*  ESP32-2432 ESPnow gateway */


#include <ArduinoJson.h>
#include "WiFi.h"
#include <esp_now.h>
#include "TFT_eSPI.h"
#include "OpenFontRender.h"
#include "binaryttf.h"
#include <SPI.h>

#define ADDPEER 1001

JsonDocument doc;
char myData [256];
String jsondata;
String macaddr;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
OpenFontRender render;
hw_timer_t * timer = NULL;
bool newPeeradded = false ;

esp_now_peer_info_t slave = {};
int chan;
int  num_int = 0;
int i;

// PMK and LMK keys
static const char* PMK_KEY_STR = "ohJ7ui8rah9oj8ei";
static const char* LMK_KEY_STR = "coo5sheJiu8Jo2mo";

// timer0 ISR
void IRAM_ATTR onTimer() {
	num_int++ ;

}

void printMAC(const uint8_t * mac_addr){
	char macStr[18];
	snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
			mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	Serial.println(macStr);
}



void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {

	memcpy(&myData, incomingData, sizeof(myData));
	myData[len]='\0';
	deserializeJson(doc,myData );
	serializeJson(doc, Serial);
}

void setup() {
	Serial.begin(115200);
	tft.begin();
	tft.setRotation(0);
	tft.fillScreen(TFT_BLACK);
	tft.setSwapBytes(true);
	render.loadFont(binaryttf, sizeof(binaryttf));
	render.setDrawer(tft);
	WiFi.mode(WIFI_MODE_STA);
	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	// Set PMK key
	esp_now_set_pmk((uint8_t *)PMK_KEY_STR);
	chan = WiFi.channel();
	esp_now_register_recv_cb(OnDataRecv);
	macaddr = WiFi.macAddress();
	//Serial.println(	macaddr);
	render.setCursor(160, 0);
	render.setFontSize(12);
	render.setFontColor(TFT_WHITE);
	render.printf(macaddr.c_str());
	render.setDrawer(spr);

	timer = timerBegin(0, 80, true);
	timerAttachInterrupt(timer, &onTimer,  true);
	timerAlarmWrite(timer, 1000000, true); // count in uSECs
	timerAlarmEnable(timer); // make sure this is the last instruction in setup()

}

void loop() {
	if (Serial.available()) {
		String name = Serial.readStringUntil('}\n');
	DeserializationError error = deserializeJson(doc, name);

	if (!error){

		if (doc["id"] == ADDPEER )  { 
			const char * macaddr = doc["mac"];
			Serial.print("adding peer ");
			Serial.println(macaddr);

			uint8_t newpeer[6];
			sscanf(macaddr, "%X:%X:%X:%X:%X:%X", &newpeer[0], &newpeer[1], &newpeer[2], &newpeer[3], &newpeer[4], &newpeer[5]);

			//memset(&slave, 0, sizeof(slave));
			const esp_now_peer_info_t *peer = &slave;
			memcpy(slave.peer_addr, newpeer, 6);
			slave.channel = chan; 
			//Set the receiver device LMK key
			for (uint8_t k = 0; k < 16; k++) {
				slave.lmk[k] = LMK_KEY_STR[k];
			}
			// Set encryption to true
			slave.encrypt = true;
			bool exists = esp_now_is_peer_exist(slave.peer_addr);
			if (!exists) {
				esp_err_t addStatus = esp_now_add_peer(peer);
				if (addStatus == ESP_OK) {
					newPeeradded = true ;
				}
			}

		} else {
			serializeJson(doc, name);
			uint8_t length = measureJson(doc);
			esp_now_send(NULL, (uint8_t *) name.c_str(), length);  //Sending "jsondata"  
		}
	} else {
		Serial.println("malformed json");
	}

}

if ( num_int > 0) {
	num_int--;


	esp_now_peer_num_t pn;
	esp_now_get_peer_num(&pn);


	if(newPeeradded) {       
		spr.createSprite(240, 30);
		render.setCursor(0, 0);
		render.setFontSize(18);
		render.setFontColor(TFT_GREEN);

		for(i=0; i < pn.total_num; i++){
			//printMAC(&slave.peer_addr[i]);
			const uint8_t * mac_addr = &slave.peer_addr[i];
			char macStr[18];
			snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
					mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
			render.printf(macStr);

		}
		spr.pushSprite(0, 100);
		spr.deleteSprite();
		newPeeradded = false;
	}
}
}
