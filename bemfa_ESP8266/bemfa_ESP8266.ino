#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//巴法云服务器地址默认即可
#define TCP_SERVER_ADDR "bemfa.com"
//服务器端口，tcp创客云端口8344
#define TCP_SERVER_PORT "8344"
//********************需要修改的部分*******************//
#define DEFAULT_STASSID  "TP-LINK_103"     //WIFI名称，区分大小写，不要写错
#define DEFAULT_STAPSW   "11111111"  //WIFI密码
String UID = "fdb2ee423ac348439c1b554abe94cd28";  //用户私钥，可在控制台获取,修改为自己的UID
String TOPIC =   "lee";         //主题名字，可在控制台新建
//**************************************************//
//最大字节数
#define MAX_PACKETSIZE 512
//设置心跳值30s
#define KEEPALIVEATIME 2*1000
//tcp客户端相关初始化，默认即可
WiFiClient TCPclient;
String upstr="";
String textstr="";
unsigned long preHeartTick = 0;//心跳
unsigned long preTCPStartTick = 0;//连接
bool preTCPConnected = false;
//相关函数初始化
//连接WIFI
void doWiFiTick();
void startSTA();
//TCP初始化连接
void doTCPClientTick();
void startTCPClient();
void sendtoTCPServer(String p);
/*
  *发送数据到TCP服务器
 */
void sendtoTCPServer(String p){
  if (!TCPclient.connected()) 
  {
    Serial.println("Client is not readly");
    return;
  }
  TCPclient.print(p);
  Serial.println("[Send to TCPServer]:String");
  Serial.println(p);
}

/*
  *初始化和服务器建立连接
*/
void startTCPClient(){
  if(TCPclient.connect(TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT))){
    Serial.print("\nConnected to server:");
    Serial.printf("%s:%d\r\n",TCP_SERVER_ADDR,atoi(TCP_SERVER_PORT));
    String tcpTemp="";  //初始化字符串
    tcpTemp = "cmd=1&uid="+UID+"&topic="+TOPIC+"\r\n"; //构建订阅指令
    sendtoTCPServer(tcpTemp); //发送订阅指令
    tcpTemp="";//清空
    preTCPConnected = true;
    preHeartTick = millis();
    TCPclient.setNoDelay(true);
  }
  else{
    Serial.print("Failed connected to server:");
    Serial.println(TCP_SERVER_ADDR);
    TCPclient.stop();
    preTCPConnected = false;
  }
  preTCPStartTick = millis();
}
/*
  *检查数据，发送心跳
*/
void doTCPClientTick(){
 //检查是否断开，断开后重连
   if(WiFi.status() != WL_CONNECTED) return;
  if (!TCPclient.connected()) {//断开重连
  if(preTCPConnected == true){
    preTCPConnected = false;
    preTCPStartTick = millis();
    Serial.println();
    Serial.println("TCP Client disconnected.");
    TCPclient.stop();
  }
  else if(millis() - preTCPStartTick > 1*1000)//重新连接
    startTCPClient();
  }
  else
  {
    if(millis() - preHeartTick >= KEEPALIVEATIME){//保持心跳
      preHeartTick = millis();
    }    
    while (Serial.available() > 0)  
    {
     char c =(Serial.read());
     textstr=textstr+c;
     delay(2);
    }
    if (textstr.length() >0)
    {
     upstr ="cmd=2&uid="+UID+"&topic="+TOPIC+"&msg=#"+textstr+"#\r\n";
     sendtoTCPServer(upstr);
     textstr=""; 
    }
    upstr="";  
  }
}
void startSTA(){
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(DEFAULT_STASSID, DEFAULT_STAPSW);
}
/**************************************************************************
                                 WIFI
***************************************************************************/
/*
  WiFiTick
  检查是否需要初始化WiFi
  检查WiFi是否连接上，若连接成功启动TCP Client
  控制指示灯
*/
void doWiFiTick(){
  static bool startSTAFlag = false;
  static bool taskStarted = false;
  static uint32_t lastWiFiCheckTick = 0;

  if (!startSTAFlag) {
    startSTAFlag = true;
    startSTA();
    Serial.printf("Heap size:%d\r\n", ESP.getFreeHeap());
  }
  //未连接1s重连
  if ( WiFi.status() != WL_CONNECTED ) {
    if (millis() - lastWiFiCheckTick > 1000) {
      lastWiFiCheckTick = millis();
    }
  }
  //连接成功建立
  else {
    if (taskStarted == false) {
      taskStarted = true;
      Serial.print("\r\nGet IP Address: ");
      Serial.println(WiFi.localIP());
      startTCPClient();
    }
  }
}
// 初始化，相当于main 函数
void setup() {
  Serial.begin(9600);
}
//循环
void loop() {
  doWiFiTick();
  doTCPClientTick();
}
