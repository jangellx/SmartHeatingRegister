
// Access point connection information and default base host name

#ifndef WIFI_INIT_DEFINED
#define WIFI_INIT_DEFINED
void initWifiConfig (void) {
  ssid     = "mySSIDhere";			// Change this to your wifi SSID
  password = "myPasswordHere";			// Change this to your wifi password
  hostNameBase = "heatReg_";
}
#endif
