#ifndef _ENV_SENSOR_H_
#define _ENV_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define TIMEOUT 5000

//Default values
#define DEFAULT_SEND_INTERVAL 3*60 //Seconds
#define DEFAULT_READ_INTERVAL 12   //Seconds
#define DEFAULT_MEASURE_INDEX 5  //12
//Max values
#define MAX_SEND_INTERVAL     30*60 //Seconds
#define MAX_READ_INTERVAL     128   //Seconds
#define MAX_MEASURE_INDEX     9 //12

// protocol version check
#define PROTOCOL_VERSION_MIN "0.0"
// json keys
#define P_SIGNATURE        "s"
#define P_VERSION          "v"
#define P_KEY              "k"
#define P_PAYLOAD          "p"
#define P_INTERVAL         "i"
#define P_THRESHOLD        "th"
#define P_DEVICE_ID        "id"
#define P_TIMESTAMP        "ts"
#define P_MEASURE_INTERVAL "im"
// error flags
#define E_SENSOR_FAILED 0b00000001 //1
#define E_PROTOCOL_FAIL 0b00000010 //2
#define E_SIG_VRFY_FAIL 0b00000100 //4
#define E_JSON_FAILED   0b00001000 //8
#define E_NO_MEMORY     0b10000000 //128
#define E_NO_CONNECTION 0b01000000 //64
#define E_WDOG_RESET    0b10000001 //129
#define E_POR           0b10000010 //130
#define E_MODEM_RESET   0b10000011 //131
#define E_TCP_FAIL      0b10000100 //132


typedef struct {
    int temp;
    int pressure;
    int humidity;
    int altitide;
    int batLevel;
    int lp;
    int errorFlag;
#if AIR_QUALITY_SENSOR
    int aq;
    int aqr;
#endif
    char lat[32];
    char lon[32];
    char timeStamp[24];
} payload_t;

#ifdef __cplusplus
}
#endif

#endif // _ENV_SENSOR_H_