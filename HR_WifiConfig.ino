
// Access point connection information and default base host name

#ifndef WIFI_INIT_DEFINED
#define WIFI_INIT_DEFINED
void initWifiConfig (void) {
  ssid     = "mySSIDhere";			// Change this to your wifi SSID
  password = "myPasswordHere";			// Change this to your wifi password
  hostNameBase = "heatReg";
  webhooksURL  = "http://192.168.1.123:51830";  // IP and port of homebridge-HTTPWebhooks instance.  If NULL, nothing is sent.  accessoryID is the full hostname (ie: "heatReg8")
}
#endif
