//
// Created by nirao on 18.04.17.
//

#include <M66Interface.h>
#include <kinetis_lowpower.h>
#include <http_response.h>
#include <http_request.h>
#include <crypto.h>
#include "mbed.h"
#include "../Grove_Air_Quality_Sensor_Library/Air_Quality.h"
#include "../config.h"
#include "../BME280/BME280.h"
#include "sensor.h"
#include "response.h"

#include "../dbgutil/dbgutil.h"

#define PRESSURE_SEA_LEVEL 101325
#define TEMPERATURE_THRESHOLD 4000

DigitalOut extPower(PTC8);
DigitalOut led1(LED1);
M66Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER, true);
BME280 bmeSensor(I2C_SDA, I2C_SCL);

AirQuality airqualitysensor(PTC0);

//actual payload template
static const char *const payload_template = "{\"t\":%d,\"p\":%d,\"h\":%d,\"a\":%d,\"la\":\"%s\",\"lo\":\"%s\",\"ba\":%d,\"lp\":%d,\"e\":%d,\"aq\":%d,\"aqr\":%d}";
static const char *const message_template = "{\"v\":\"0.0.2\",\"a\":\"%s\",\"k\":\"%s\",\"s\":\"%s\",\"p\":%s}";

uint8_t error_flag = 0x00;
int unsuccessfulSend = -1;
static float temperature, pressure, humidity, altitude;
static int currentAirQuality  = -1;
static int temp_threshold = TEMPERATURE_THRESHOLD;
// internal sensor state
static unsigned int interval = DEFAULT_INTERVAL;
static int16_t loop_counter = 0;

void dump_response(HttpResponse* res) {
    printf("Status: %d - %s\n", res->get_status_code(), res->get_status_message().c_str());

    printf("Headers:\n");
    for (size_t ix = 0; ix < res->get_headers_length(); ix++) {
        printf("\t%s: %s\n", res->get_headers_fields()[ix]->c_str(), res->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%d bytes):\n\n%s\n", (int)res->get_body_length(), res->get_body_as_string().c_str());
}

// Interrupt Handler
void AirQualityInterrupt(void)
{
    airqualitysensor.last_vol = airqualitysensor.first_vol;
    airqualitysensor.first_vol = airqualitysensor.getAQSensorValue();
    airqualitysensor.timer_index = 1;
    currentAirQuality = airqualitysensor.slope();
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

    int aqVal = airqualitysensor.first_vol;
    int aqRefVal = airqualitysensor.aqRefVal;

    uint8_t status = 0;
    bool gotLocation = false;
    int rc;
    int ret;
    int level = 0;
    int voltage = 0;
    static char lat[32], lon[32];
    char theIP[20];
    // crypto key of the board
    static uc_ed25519_key uc_key;

    rtc_datetime_t date_time;

    // Create a TCP socket
    printf("\n----- Setting up TCP connection -----\r\n");

    if (!modem.queryIP("api.demo.dev.ubirch.com", theIP)) {
        PRINTF("Get IP failed\r\n");
        return 1;
    }

    TCPSocket *socket = new TCPSocket();
    // TODO make sure you close the socket or delet socket before returning error handlers
    nsapi_error_t open_result = socket->open(&modem);

    if (open_result != 0) {
        printf("Opening TCPSocket failed... %d\n", open_result);
        delete socket;
        return 1;
    }

    nsapi_error_t connect_result = socket->connect(theIP, 8080);
    if (connect_result != 0) {
        printf("Connecting over TCPSocket failed... %d\n", connect_result);
        delete socket;
        return 1;
    }


    /* Get battery level, latitude, logitude, time stamp*/
    modem.getModemBattery(&status, &level, &voltage);
    printf("the battery status %d, level %d, voltage %d\r\n", status, level, voltage);

    for (int lc = 0; lc < 3 && !gotLocation; lc++) {
        gotLocation = modem.get_location_date(lat, lon, &date_time);
        PRINTF("setting current time from GSM\r\n");
        PRINTF("%04hd-%02hd-%02hd %02hd:%02hd:%02hd\r\n",
               date_time.year, date_time.month, date_time.day, date_time.hour, date_time.minute, date_time.second);
        PRINTF("lat is %s lon %s\r\n", lat, lon);
    }
    if (!gotLocation) {
        delete socket;
        return 1;
    }

    /* RTC time counter has to be stopped before setting the date & time in the TSR register */
    RTC_StopTimer(RTC);
    /* Set RTC time to default */
    RTC_SetDatetime(RTC, &date_time);
    /* End RTC stuff*/

    /* Crypto stuff, init and import the ecc key*/
    uc_init();
    uc_import_ecc_key(&uc_key, device_ecc_key, device_ecc_key_len);

    //++++++++++++++++++++++++++++++++++++++++
    // payload structure to be signed
    // Example: '{"t":22.0,"p":1019.5,"h":40.2,"lat":"12.475886","lon":"51.505264","bat":100,"lps":99999}'
    int payload_size = snprintf(NULL, 0, payload_template,
                                (int) (temperature * 100.0f), (int) pressure, (int) ((humidity) * 100.0f),
                                (int) (altitude * 100.0f),
                                lat, lon, level, loop_counter, error_flag, aqVal, aqRefVal);
    char *payload = (char *) malloc((size_t) payload_size + 1);
    sprintf(payload, payload_template,
            (int) (temperature * 100.0f), (int) (pressure), (int) ((humidity) * 100.0f), (int) (altitude * 100.0f),
            lat, lon, level, loop_counter, error_flag, aqVal, aqRefVal);

    error_flag = 0x00;

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

    PRINTF("--MESSAGE (%d)\r\n", strlen(message));
    printf("\r\n--MESSAGE %s\r\n", message);
    wait_ms(100);

    // POST HTTP request
    {
        HttpRequest *post_req = new HttpRequest(socket, HTTP_POST,
                                                "http://api.demo.dev.ubirch.com/api/avatarService/v1/device/update");
        post_req->set_header("Content-Type", "application/json");

        HttpResponse *post_res = post_req->send(message, strlen(message));
        free(message);

        if (!post_res) {
            PRINTF("HttpRequest failed (error code %d)\n", post_req->get_error());
            unsuccessfulSend = true;
            delete socket;
            return 1;
        }

        PRINTF("\n----- HTTP POST response -----\n");
//        dump_response(post_res);

        uc_ed25519_pub_pkcs8 response_key;
        unsigned char response_signature[SHA512_HASH_SIZE];
        memset(&response_key, 0xff, sizeof(uc_ed25519_pub_pkcs8));
        memset(response_signature, 0xf7, SHA512_HASH_SIZE);

        char *response_payload = process_response((char *) post_res->get_body_as_string().c_str(), &response_key,
                                                  response_signature);
        wait(1);

        hex_dump("KEY:", (unsigned char *) &response_key, sizeof(uc_ed25519_pub_pkcs8));
        hex_dump("SIG:", response_signature, sizeof(response_signature));
        PRINTF("PAYLOAD: %s\r\n", response_payload);
        wait_ms(50);
        process_payload(response_payload);

        free(response_payload);

        delete post_req;
    }
    delete socket;

    modem.powerDown();
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

// Main loop
int main() {

    int connectFail = 0;
    printf("Fire  up the sensors\r\n");
    extPower.write(1);

    osThreadCreate(osThread(ledBlink), NULL);
    osThreadCreate(osThread(bme_thread), NULL);

    airqualitysensor.init(AirQualityInterrupt);

    rtc_config_t rtcConfig;
    /* Init RTC and update the RYC date and time*/
    RTC_GetDefaultConfig(&rtcConfig);
    RTC_Init(RTC, &rtcConfig);
    /* Select RTC clock source */
    /* Enable the RTC 32KHz oscillator */
    RTC->CR |= RTC_CR_OSCE_MASK;

    while (1) {
        if (((int) (temperature * 100)) > temp_threshold || (loop_counter % (MAX_INTERVAL / interval) == 0) ||
            unsuccessfulSend) {
            const int r = modem.connect(CELL_APN, CELL_USER, CELL_PWD);
            if (r != 0) {
                connectFail++;
                PRINTF("Cannot connect to the network, see serial output");
            } else {
                unsuccessfulSend = HTTPSession();
                if(!unsuccessfulSend) connectFail = 0;
                else connectFail++;
            }
        }
        if (connectFail >= 5){
            NVIC_SystemReset();
        }

        printf("%d..\r\n", airqualitysensor.first_vol);
        if(!connectFail) {
            wait(10);
            loop_counter++;
        }
    }
}
