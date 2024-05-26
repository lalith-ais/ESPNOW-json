#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define	SENSORID 101
#define GPIO2 2
String jsondata; 

hw_timer_t * timer = NULL;
int  num_int = 0;

esp_now_peer_info_t peerInfo;

// PMK and LMK keys
static const char* PMK_KEY_STR = "ohJ7ui8rah9oj8ei";
static const char* LMK_KEY_STR = "coo5sheJiu8Jo2mo";

char myData [256];
uint8_t gatewayAddress[] = {0xC8,0xF0,0x9E,0xF2,0x66,0x04};
//bool Peered = false;
int count =0;

// timer0 ISR
void IRAM_ATTR onTimer() {
	num_int++ ;

}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {


}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
	JsonDocument doc;
	memcpy(&myData, incomingData, sizeof(myData));
	myData[len]='\0';
	deserializeJson(doc,myData ); 

	if (doc["id"] == SENSORID )  { 
		if (doc["gpio2"] == 1) { digitalWrite(GPIO2, HIGH);}
		if (doc["gpio2"] == 0) { digitalWrite(GPIO2, LOW);}
		serializeJsonPretty(doc, Serial);
	}
}



void sendHeartbeat(void){
	JsonDocument doc;
	jsondata = "";  //clearing String after data is being sent
	doc["type"] = "HEARTBEAT"; 
	doc["id"] = SENSORID ;
	serializeJson(doc, jsondata);  //Serilizing JSON
	uint8_t length = measureJson(doc);
	esp_now_send(NULL, (uint8_t *) jsondata.c_str(), length);	
}

void setup() {

	Serial.begin(115200);
	pinMode(GPIO2, OUTPUT);
	digitalWrite(GPIO2, LOW);
	WiFi.mode(WIFI_STA);
	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	// Set PMK key
	esp_now_set_pmk((uint8_t *)PMK_KEY_STR);

	// Register the receiver board as peer
	esp_now_peer_info_t peerInfo = {};
	memcpy(peerInfo.peer_addr, gatewayAddress, 6);
	peerInfo.channel = 0;
	//Set the receiver device LMK key
	for (uint8_t k = 0; k < 16; k++) {
		peerInfo.lmk[k] = LMK_KEY_STR[k];
	}
	// Set encryption to true
	peerInfo.encrypt = true;
	peerInfo.channel = 0;

	if (esp_now_add_peer(&peerInfo) != ESP_OK){
		Serial.println("Failed to add peer");
		return;
	}
	esp_now_register_recv_cb(OnDataRecv);
	esp_now_register_send_cb(OnDataSent);



	digitalWrite(GPIO2, LOW);

	timer = timerBegin(0, 80, true);
	timerAttachInterrupt(timer, &onTimer,  true);
	timerAlarmWrite(timer, 1000000, true); // count in uSECs
	timerAlarmEnable(timer); // make sure this is the last instruction in setup()
	sendHeartbeat();

}

void loop() {

	if ( num_int > 0) {
		num_int--;
		count ++ ;
		//if (!Peered) {sendPeeringRequest();}
		if (count > 15) {
			count = 0; 

			sendHeartbeat();

		}

	}


}
