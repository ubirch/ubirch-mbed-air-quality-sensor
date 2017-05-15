//
// Created by nirao on 18.04.17.
//

#include <M66Interface.h>
#include <kinetis_lowpower.h>
#include <http_response.h>
#include <http_request.h>
#include <crypto.h>
#include <fsl_wdog.h>
#include "mbed.h"
#include "../Grove_Air_Quality_Sensor_Library/Air_Quality.h"
#include "../config.h"
#include "../BME280/BME280.h"
#include "sensor.h"
#include "response.h"

#include "../dbgutil/dbgutil.h"

#define PRESSURE_SEA_LEVEL    101325
#define TEMPERATURE_THRESHOLD 4000

DigitalOut extPower(PTC8);
DigitalOut led1(LED1);
M66Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER, true);
BME280 bmeSensor(I2C_SDA, I2C_SCL);
AirQuality airqualitysensor(PTC0);

//    WATCHDOG TIMER
#define WDOG_WCT_INSTRUCITON_COUNT (256U)
static WDOG_Type *wdog_base = WDOG;
static RCM_Type *rcm_base = RCM;

//actual payload template
static const char *const payload_template = "{\"t\":%d,\"p\":%d,\"h\":%d,\"a\":%d,\"la\":\"%s\",\"lo\":\"%s\",\"ba\":%d,\"lp\":%d,\"e\":%d,\"aq\":%d,\"aqr\":%d,\"ts\":\"%s\"}";
static const char *const message_template = "{\"v\":\"0.0.3\",\"a\":\"%s\",\"k\":\"%s\",\"s\":\"%s\",\"p\":%s}";
static const char *const timeStamp_template = "%d-%d-%dT%d:%d:%d.%dZ"; //“2017-05-09T10:25:41.836Z”
uint8_t error_flag = 0x00;

static float temperature, pressure, humidity, altitude;
static int temp_threshold = TEMPERATURE_THRESHOLD;
// internal sensor state
static unsigned int interval = DEFAULT_INTERVAL;
int measureIndex = DEFAULT_MEASURE_INTERVAL;
static int16_t loop_counter = 0;
int unsuccessfulSend = -1;
bool aqPolluted = false;
int sendPacket = -1;

void dump_response(HttpResponse* res) {
    printf("Status: %d - %s\n", res->get_status_code(), res->get_status_message().c_str());

    printf("Headers:\n");
    for (size_t ix = 0; ix < res->get_headers_length(); ix++) {
        printf("\t%s: %s\n", res->get_headers_fields()[ix]->c_str(), res->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%d bytes):\n\n%s\n", (int)res->get_body_length(), res->get_body_as_string().c_str());
}

// Interrupt Handler
void AirQualityInterrupt(void) {
    airqualitysensor.last_vol = airqualitysensor.first_vol;
    airqualitysensor.first_vol = airqualitysensor.getAQSensorValue();
    airqualitysensor.timer_index = 1;
    if (airqualitysensor.slope() > 1) {
        aqPolluted = true;
    }
}

// convert a number of characters into an unsigned integer value
static unsigned int to_uint(const char *ptr, size_t len) {
    unsigned int ret = 0;
    for (uint8_t i = 0; i < len; i++) {
        ret = (ret * 10) + (ptr[i] - '0');
    }
    return ret;
}

/*!
 * Process payload and set configuration parameters from it.
 * @param payload the payload to use, should be checked
 */
void process_payload(char *payload) {
    jsmntok_t *token;
    jsmn_parser parser;
    jsmn_init(&parser);

    // identify the number of tokens in our response, we expect 13
    const uint8_t token_count = (const uint8_t) jsmn_parse(&parser, payload, strlen(payload), NULL, 0);
    token = (jsmntok_t *) malloc(sizeof(*token) * token_count);

    // reset parser, parse and store tokens
    jsmn_init(&parser);
    if (jsmn_parse(&parser, payload, strlen(payload), token, token_count) == token_count &&
        token[0].type == JSMN_OBJECT) {
        uint8_t index = 0;
        while (++index < token_count) {
            if (jsoneq(payload, &token[index], P_INTERVAL) == 0 && token[index + 1].type == JSMN_PRIMITIVE) {
                index++;
                interval = to_uint(payload + token[index].start, (size_t) token[index].end - token[index].start);
                PRINTF("Interval: %ds\r\n", interval);
                wait_ms(50);
            } else if (jsoneq(payload, &token[index], P_THRESHOLD) == 0 && token[index + 1].type == JSMN_PRIMITIVE) {
                index++;
                temp_threshold = to_uint(payload + token[index].start, (size_t) token[index].end - token[index].start);
                PRINTF("Threshold: %d\r\n", temp_threshold);
                wait_ms(50);
            } else {
                print_token("unknown key:", payload, &token[index]);
                wait_ms(50);
                index++;
            }
        }
    } else {
        error_flag |= E_JSON_FAILED;
    }

    free(token);
}

int HTTPSession() {

    uint8_t status = 0;
    int voltage = 0;
    char theIP[20];
    // crypto key of the board
    static uc_ed25519_key uc_key;
    //RTC Struct
    rtc_datetime_t date_time;

//    Payload array stuff
    payload_t pVal[measureIndex];

    int newIndex = 0;
    for (int i = 0; i < measureIndex; ++i) {
        bool gotLocation = false;
        int zone = 0;


        pVal[i].temp = (int) (temperature * 100.0f);
        pVal[i].pressure = (int) pressure;
        pVal[i].humidity = (int) ((humidity) * 100.0f);
        pVal[i].altitide = (int) (altitude * 100.0f);
        pVal[i].aq = airqualitysensor.first_vol;
        pVal[i].aqr = airqualitysensor.aqRefVal;
        pVal[i].lp = loop_counter;

        /* Get battery level, latitude, logitude, time stamp*/
        modem.getModemBattery(&status, &pVal[i].batLevel, &voltage);
        PRINTF("the battery status %d, level %d, voltage %d\r\n", status, pVal[i].batLevel, voltage);

        for (int lc = 0; lc < 3 && !gotLocation; lc++) {
            gotLocation = modem.get_location_date(pVal[i].lat, pVal[i].lon, &date_time, &zone);
            PRINTF("setting current time from GSM\r\n");
            PRINTF("%04hd-%02hd-%02hd %02hd:%02hd:%02hd\r\n",
                   date_time.year, date_time.month, date_time.day, date_time.hour, date_time.minute,
                   date_time.second);
            PRINTF("lat is %s lon %s\r\n", pVal[i].lat, pVal[i].lon);
            wait_ms(500);
        }
        if (!gotLocation) return 1;

        sprintf(pVal[i].timeStamp, timeStamp_template, date_time.year,
                date_time.month, date_time.day, date_time.hour, date_time.minute, date_time.second, zone);
        pVal[i].errorFlag = error_flag;
        error_flag = 0x00;

        /* RTC time counter has to be stopped before setting the date & time in the TSR register */
        RTC_StopTimer(RTC);
        /* Set RTC time to default */
        RTC_SetDatetime(RTC, &date_time);
        /* End RTC stuff*/

        loop_counter++;
        newIndex++;

        WDOG_Refresh(wdog_base);

//        if (unsuccessfulSend || aqPolluted) {
            if (((int) (temperature * 100)) > temp_threshold || unsuccessfulSend || aqPolluted) {
            break;
        }

        printf("timer :: %d\r\n", MAX_INTERVAL/interval);
//        wait(MAX_INTERVAL/interval);
    } // Payload into array for loop

    /* Get the total Payload array size*/
    int payloadSize = 0;
    int pValSize[newIndex];
    for (int j = 0; j < newIndex; ++j) {
        pValSize[j] = snprintf(NULL, 0, payload_template,
                               pVal[j].temp,
                               pVal[j].pressure,
                               pVal[j].humidity,
                               pVal[j].altitide,
                               pVal[j].lat,
                               pVal[j].lon,
                               pVal[j].batLevel,
                               pVal[j].lp,
                               pVal[j].errorFlag,
                               pVal[j].aq,
                               pVal[j].aqr,
                               pVal[j].timeStamp);
        payloadSize += pValSize[j];
    }

    /*for , after every payload {....}, {....}, ...*/
    payloadSize += newIndex - 1;
    /*2 chars for [], [{....}, {....}, {....}, ...], in the JSON object to form payload array*/
    payloadSize += 2;

    /* ++++++++++++++++++++++++++++++++++++++++
     * Generate the payload array to be signed
     * Example: '[{"t":22.0,"p":1019.5,"h":40.2,"lat":"12.475886","lon":"51.505264","bat":100,"lps":99999, "aq":121, "aqr":121, "ts":2017-05-09T10:25:41.836Z}, ...]'
     */
    char *payload = (char *) malloc((size_t) payloadSize + 1);
    memset(payload, 0, (size_t) (payloadSize + 1));
    // open [ to form payload array
    strcpy(payload, "[");
    for (int k = 0; k < newIndex; ++k) {
        char *tempPayload = (char *) malloc(size_t(pValSize[k]));
        memset(tempPayload, 0, (size_t) ((pValSize[k])));
        sprintf(tempPayload, payload_template,
                pVal[k].temp,
                pVal[k].pressure,
                pVal[k].humidity,
                pVal[k].altitide,
                pVal[k].lat,
                pVal[k].lon,
                pVal[k].batLevel,
                pVal[k].lp,
                pVal[k].errorFlag,
                pVal[k].aq,
                pVal[k].aqr,
                pVal[k].timeStamp);
        printf("tempPayload::: %s\r\n", tempPayload);
        if (k == 0) {
            strncat(payload, tempPayload, (size_t) pValSize[k]);
        } else {
            strcat(payload, ",");
            strncat(payload, tempPayload, (size_t) pValSize[k]);
        }
        free(tempPayload);
    }
    // cloase the ], end of the payload array
    strcat(payload, "]");

    //payload arrray loop
    printf("PAYLOAD::(%d):: %s\r\n", payloadSize, payload);

    /* Crypto stuff, init and import the ecc key*/
    uc_init();
    uc_import_ecc_key(&uc_key, device_ecc_key, device_ecc_key_len);

    //Get IMEI of the
    const char *imei = modem.get_imei();

    // be aware that you need to free these strings after use
    char *auth_hash = uc_sha512_encoded((const unsigned char *) imei, strnlen(imei, 15));
    char *pub_key_hash = uc_base64_encode(uc_key.p, 32);
    char *payload_hash = uc_ecc_sign_encoded(&uc_key, (const unsigned char *) payload, strlen(payload));

    PRINTF("PUBKEY   : %s\r\n", pub_key_hash);
    PRINTF("AUTH     : %s\r\n", auth_hash);
    PRINTF("SIGNATURE: %s\r\n", payload_hash);

    int message_size = snprintf(NULL, 0, message_template, auth_hash, pub_key_hash, payload_hash, payload);
    char *message = (char *) malloc((size_t) (message_size + 1));

    sprintf(message, message_template, auth_hash, pub_key_hash, payload_hash, payload);
    PRINTF("message_size%d\r\n", message_size);

    // free hashes
    delete (auth_hash);
    delete (pub_key_hash);
    delete (payload_hash);
    free(payload);

    printf("\r\nMESSAGE::(%d):: %s\r\n", (int)strlen(message), message);

//    if(!sendPacket){
//        free(message);
//        return 0;
//    }

    // Create a TCP socket
    if (!modem.queryIP(UTCP_HOST, theIP)) {
        PRINTF("Get IP failed\r\n");
        return 1;
    }

    printf("\r\nOpen the TCP Socket\r\n");

    TCPSocket *socket = new TCPSocket();
    // TODO make sure you close the socket or delet socket before returning error handlers
    nsapi_error_t open_result = socket->open(&modem);

    if (open_result != 0) {
        printf("Opening TCPSocket failed... %d\n", open_result);
        delete socket;
        return 1;
    }

    nsapi_error_t connect_result = socket->connect(theIP, UTCP_PORT);
    if (connect_result != 0) {
        printf("Connecting over TCPSocket failed... %d\n", connect_result);
        delete socket;
        return 1;
    }

    // POST HTTP request
    {
        HttpRequest *post_req = new HttpRequest(socket, HTTP_POST, UHTTP_URL);
        post_req->set_header("Content-Type", "application/json");

        HttpResponse *post_res = post_req->send(message, strlen(message));
        free(message);

        if (!post_res) {
            PRINTF("HttpRequest failed (error code %d)\n", post_req->get_error());
            unsuccessfulSend = true;
            delete socket;
            return 1;
        }

        // verify the HTTP response
        uc_ed25519_pub_pkcs8 response_key;
        unsigned char response_signature[SHA512_HASH_SIZE];
        memset(&response_key, 0xff, sizeof(uc_ed25519_pub_pkcs8));
        memset(response_signature, 0xf7, SHA512_HASH_SIZE);

        char *response_payload = process_response((char *) post_res->get_body_as_string().c_str(), &response_key,
                                                  response_signature);

        hex_dump("KEY:", (unsigned char *) &response_key, sizeof(uc_ed25519_pub_pkcs8));
        hex_dump("SIG:", response_signature, sizeof(response_signature));
        PRINTF("PAYLOAD: %s\r\n", response_payload);
        // Process the verified HTTP response
        process_payload(response_payload);

        free(response_payload);
        delete post_req;
    }
    delete socket;

    sendPacket = false;
//    modem.powerDown();
//    powerDownWakeupOnRtc(5 * 60);
    return 0;
}

void ledBlink(void const *args){
    while(1) {
        led1 = !led1;
        Thread::wait(800);
    }
}

void bme_thread(void const *args) {
    while (true) {
        temperature = bmeSensor.getTemperature();
        pressure = bmeSensor.getPressure();
        humidity = bmeSensor.getHumidity();
        altitude = 44330.0f * (1.0f - (float) pow(pressure / (float) PRESSURE_SEA_LEVEL, 1 / 5.255));

        Thread::wait(10000);
    }
}

osThreadDef(ledBlink, osPriorityNormal, DEFAULT_STACK_SIZE);
osThreadDef(bme_thread, osPriorityNormal, DEFAULT_STACK_SIZE);

static void WaitWctClose(WDOG_Type *base)
{
    /* Accessing register by bus clock */
    for (uint32_t i = 0; i < WDOG_WCT_INSTRUCITON_COUNT; i++)
    {
        (void)base->RSTCNT;
    }
}

// Main loop
int main() {

    printf("Fire up the sensors\r\n");
    extPower.write(1);
    int connectFail = 0;

    if ((RCM_GetPreviousResetSources(rcm_base) & kRCM_SourceWdog)) {
        WDOG_ClearResetCount(wdog_base);
        error_flag = E_WDOG_RESET;
        printf("\r\nWatchDog reset the board\r\n");
    } else error_flag = (uint8_t) RCM_GetPreviousResetSources(rcm_base);

    osThreadCreate(osThread(ledBlink), NULL);
    osThreadCreate(osThread(bme_thread), NULL);

//    airqualitysensor.init(AirQualityInterrupt);

    rtc_config_t rtcConfig;
    /* Init RTC and update the RYC date and time*/
    RTC_GetDefaultConfig(&rtcConfig);
    RTC_Init(RTC, &rtcConfig);
    /* Select RTC clock source */
    /* Enable the RTC 32KHz oscillator */
    RTC->CR |= RTC_CR_OSCE_MASK;

    wdog_config_t config;
    WDOG_GetDefaultConfig(&config);
    config.timeoutValue = 0x2BF20U; //180 seconds ;

    WDOG_Init(wdog_base, &config);
    WaitWctClose(wdog_base);

    while (1) {
//        if (((int) (temperature * 100)) > temp_threshold || unsuccessfulSend || aqPolluted) {
//            (loop_counter % (MAX_MEASURE_INTERVAL / measureIndex) == 0) ||
//            unsuccessfulSend) {
        int ret = modem.checkGPRS();
        printf("gprs %d\r\n", ret);
        if (ret != 1) {
            ret = modem.connect(CELL_APN, CELL_USER, CELL_PWD);
//            if (ret != 0) {
//                connectFail++;
//                PRINTF("Cannot connect to the network, see serial output");
            }
//        }
        //    Payload array stuff
//            payload_t pVal[measureIndex];
//        if (ret == 1) {
//                if (loop_counter % (MAX_INTERVAL / interval) == 0) {
//                    sendPacket = true;
//                }
        unsuccessfulSend = HTTPSession();
        if (!unsuccessfulSend) {
            connectFail = 0;
        } else {
            connectFail++;
            modem.powerDown();
        }
//        }
//        }
        //Feed the DOG
        WDOG_Refresh(wdog_base);

        if (connectFail >= 5) {
            connectFail = 0;
            modem.powerDown();
//            NVIC_SystemReset();
        }

        printf("%d, loop:%d..\r\n", airqualitysensor.first_vol, loop_counter);
//        if (!connectFail) {
//            wait(10);
//            loop_counter++;
//        }
    }
}


