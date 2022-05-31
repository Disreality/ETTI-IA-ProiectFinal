#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

//This code is released under the MIT license.
//Please check the license before redistributing.
//Disreality, 2022

//Build Check
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//Constants
//Use https://www.uuidgenerator.net/ to get a random UUID.
class Constants{
public:
  String ssid = "Your SSID here.";
  String password = "Your Password here.";
  String btcServerName = "ESP32 Rhapsody";
  String service_uuid = "9d8b3b35-9f20-4a39-81f7-e2357808741c";
  String apiListURL = "http://proiectia.bogdanflorea.ro/api/avatar-the-last-airbender/characters";
  String apiFetchURL = "http://proiectia.bogdanflorea.ro/api/avatar-the-last-airbender/character/";
  String idProperty = "_id";
  String nameProperty = "name";
  String imageProperty = "photoUrl";
  String descriptionProperty = "gender";
};
Constants *constants;

//Classes
class Utils{
public:
  static DynamicJsonDocument fetchJson(String url){
      DynamicJsonDocument document(8096);
      HTTPClient http;
      String payload;
      
      http.begin(url);
      http.GET();
      payload = http.getString();
      deserializeJson(document, payload);

      return document;
  }

  static DynamicJsonDocument createListDocumentJson(JsonObject object){
      DynamicJsonDocument listDocumentJson(8096);

      listDocumentJson["id"] = object[constants->idProperty];
      listDocumentJson["name"] = object[constants->nameProperty];
      listDocumentJson["image"] = object[constants->imageProperty];

      return listDocumentJson;
  }

  static DynamicJsonDocument createDetailsDocumentJson(DynamicJsonDocument document){
      DynamicJsonDocument detailsDocumentJson(8096);

      detailsDocumentJson["id"] = document[constants->idProperty];
      detailsDocumentJson["name"] = document[constants->nameProperty];
      detailsDocumentJson["image"] = document[constants->imageProperty];
      detailsDocumentJson["description"] = document[constants->descriptionProperty];

      return detailsDocumentJson;
  }
};

class WiFiConnection{
protected:
  String ssid;
  String password;
public:
  WiFiConnection(String ssid, String password, bool begin = true) :
                  ssid(ssid), password(password){
    if(begin){
      this->connect();  
    }
  }
  
  void setSSID(String ssid){
    this->ssid = ssid;
  }
  void setPassword(String password){
    this->password = password;  
  }

  String getSSID(){
    return this->ssid;
  }
  
  void connect(){
    WiFi.begin(this->ssid.c_str(), this->password.c_str());
  }
  bool isConnected(){
    if(WiFi.status() == WL_CONNECTED){
      return true;
    }

    return false;
  }
};
WiFiConnection* WiFiInstance;

class BluetoothConnection{
protected:
  BluetoothSerial SerialBT;
  String name;
public:
  BluetoothConnection(String name, bool begin = true, bool alert = true) : name(name) {
    if(begin){
      this->tryPair(alert);  
    }
  }

  void sendJson(DynamicJsonDocument document){
      String returned;
      serializeJson(document, returned);
      this->SerialBT.println(returned);
  }
  void tryPair(bool alert = false){
    this->SerialBT.begin(this->name);

    if(alert){
      Serial.println("The device started, now you can pair it with bluetooth!");  
    }
  }
  bool isAvailable(){
    return this->SerialBT.available();
  }
  String readString(){
    String data;
    while(this->isAvailable()){
        data = this->SerialBT.readStringUntil('\n');
    }
    return data;
  }
};
BluetoothConnection* BluetoothInstance;

//Received Data
void receivedData() {
  DynamicJsonDocument appRequest(8096);
  deserializeJson(appRequest, BluetoothInstance->readString());

  if(appRequest["action"] == "fetchData"){
     DynamicJsonDocument webJSON = Utils::fetchJson(constants->apiListURL);
     for(JsonObject object : webJSON.as<JsonArray>()){
      DynamicJsonDocument returnJSON = Utils::createListDocumentJson(object);
      BluetoothInstance->sendJson(returnJSON);
     }
  }else if(appRequest["action"] == "fetchDetails"){
    DynamicJsonDocument webJSON = Utils::fetchJson(constants->apiFetchURL + appRequest["id"].as<String>());
    DynamicJsonDocument returnJSON = Utils::createDetailsDocumentJson(webJSON);
    BluetoothInstance->sendJson(returnJSON);
  }
}

void setup() {
  Serial.begin(115200);

  constants = new Constants();
  WiFiInstance = new WiFiConnection(constants->ssid, constants->password);
  BluetoothInstance = new BluetoothConnection(constants->btcServerName);
}

void loop() {
  if (BluetoothInstance->isAvailable()) {
    receivedData();
  }
}
