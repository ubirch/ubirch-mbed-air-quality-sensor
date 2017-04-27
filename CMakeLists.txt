cmake_minimum_required(VERSION 3.5)
set(CMAKE_BUILD_TYPE Debug)

project(MBED_AIR_QUALITY C CXX)

# == MBED OS 5 settings ==
set(MCU MK82FN256VLL15)
set(TARGET K82F)
set(BOARD UBRIDGE)
#set(MCU MKL82Z128VLK7)
#set(TARGET KL82Z)
#set(BOARD USENSE)

set(FEATURES netsocket)

add_definitions(
        -DTOOLCHAIN_GCC
        -DCPU_${MCU}
        -DTARGET_${TARGET}
        -DTARGET_${BOARD}
        -DMBED_CONF_RTOS_PRESENT
        -DMBED_CONF_NSAPI_PRESENT
        -DMBED_CONF_EVENTS_PRESENT
        -D__MBED__
        -DDEVICE_RTC
        -DDEVICE_I2C
        -DDEVICE_SPI
        -DDEVICE_SERIAL
        -DDEVICE_TRNG
        -DDEVICE_INTERRUPTIN
        -DDEVICE_ANALOGIN
        -DDEVICE_ANALOGOUT
        -DMBED_CONF_EVENTS_PRESENT=1
        -DMBED_CONF_RTOS_PRESENT=1
        -DMBED_CONF_PLATFORM_STDIO_BAUD_RATE=115200
        -DMBED_CONF_NSAPI_PRESENT=1
        -DMBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE=115200
        -DMBED_CONF_PLATFORM_STDIO_FLUSH_AT_EXIT=0
        -DMBED_CONF_PLATFORM_STDIO_CONVERT_NEWLINES=0
)

set(MBED_OS
        mbed-os/targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/TARGET_${TARGET}/TARGET_${BOARD}
        mbed-os/targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/TARGET_${TARGET}/device/TOOLCHAIN_GCC_ARM
        mbed-os/targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/TARGET_${TARGET}/drivers
        mbed-os/targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/TARGET_${TARGET}/device
        mbed-os/targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/TARGET_${TARGET}
        mbed-os/targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/api
        mbed-os/targets/TARGET_Freescale
        mbed-os/rtos/rtx/TARGET_CORTEX_M/TARGET_RTOS_M4_M7
        mbed-os/rtos/rtx/TARGET_CORTEX_M
        mbed-os/rtos
        mbed-os/features
        mbed-os/platform
        mbed-os/hal/storage_abstraction
        mbed-os/hal
        mbed-os/events/enqueue
        mbed-os/events
        mbed-os/drivers
        mbed-os/cmsis
        mbed-os
        )

set(MBED_OS_SRCS "")
foreach (DIR in ${MBED_OS})
    if (DIR MATCHES "mbed-os/features")
        foreach(FEATURE in ${FEATURES})
            file(GLOB_RECURSE includes
                    ${DIR}/${FEATURE}/*.cpp ${DIR}/${FEATURE}/*.c ${DIR}/${FEATURE}/*.s ${DIR}/${FEATURE}/*.S)
            list(APPEND MBED_OS_SRCS ${includes})
        endforeach()
    else ()
        file(GLOB includes ${DIR}/*.cpp ${DIR}/*.c ${DIR}/*.s ${DIR}/*.S)
        list(APPEND MBED_OS_SRCS ${includes})
    endif ()
endforeach ()

add_library(mbed-os ${MBED_OS_SRCS})
include_directories(${MBED_OS})
# == END MBED OS 5 ==

add_library(AirQualitySensor Grove_Air_Quality_Sensor_Library/Air_Quality.cpp)
include_directories(AirQualitySensor PUBLIC
                    Grove_Air_Quality_Sensor_Library)

add_library(mbed-os-quectelM66-driver
        mbed-os-quectelM66-driver/M66Interface.cpp
        mbed-os-quectelM66-driver/M66ATParser/M66ATParser.cpp
        mbed-os-quectelM66-driver/M66ATParser/BufferedSerial/BufferedPrint.c
        mbed-os-quectelM66-driver/M66ATParser/BufferedSerial/BufferedSerial.cpp
        mbed-os-quectelM66-driver/M66ATParser/BufferedSerial/Buffer/MyBuffer.cpp
        )
target_include_directories(mbed-os-quectelM66-driver PUBLIC
        mbed-os-quectelM66-driver
        mbed-os-quectelM66-driver/M66ATParser
        mbed-os-quectelM66-driver/M66ATParser/BufferedSerial
        mbed-os-quectelM66-driver/M66ATParser/BufferedSerial/Buffer
        )

add_library(MQTT
        MQTT/MQTTPacket/MQTTConnectClient.c
        MQTT/MQTTPacket/MQTTConnectServer.c
        MQTT/MQTTPacket/MQTTPacket.c
        MQTT/MQTTPacket/MQTTSerializePublish.c
        MQTT/MQTTPacket/MQTTSubscribeClient.c
        MQTT/MQTTPacket/MQTTSubscribeServer.c
        MQTT/MQTTPacket/MQTTUnsubscribeClient.c
        MQTT/MQTTPacket/MQTTUnsubscribeServer.c
        )
target_include_directories(MQTT PUBLIC
        MQTT
        MQTT/MQTTPacket
        )

add_library(wolfSSL
        wolfSSL/src/crl.c
        wolfSSL/src/internal.c
        wolfSSL/src/io.c
        wolfSSL/src/keys.c
        wolfSSL/src/ocsp.c
        wolfSSL/src/sniffer.c
        wolfSSL/src/ssl.c
        wolfSSL/src/tls.c
        wolfSSL/wolfcrypt/src/aes.c
        wolfSSL/wolfcrypt/src/arc4.c
        wolfSSL/wolfcrypt/src/asm.c
        wolfSSL/wolfcrypt/src/asn.c
        wolfSSL/wolfcrypt/src/blake2b.c
        wolfSSL/wolfcrypt/src/camellia.c
        wolfSSL/wolfcrypt/src/chacha.c
        wolfSSL/wolfcrypt/src/chacha20_poly1305.c
        wolfSSL/wolfcrypt/src/coding.c
        wolfSSL/wolfcrypt/src/compress.c
        wolfSSL/wolfcrypt/src/curve25519.c
        wolfSSL/wolfcrypt/src/des3.c
        wolfSSL/wolfcrypt/src/dh.c
        wolfSSL/wolfcrypt/src/dsa.c
        wolfSSL/wolfcrypt/src/ecc.c
        wolfSSL/wolfcrypt/src/ecc_fp.c
        wolfSSL/wolfcrypt/src/ed25519.c
        wolfSSL/wolfcrypt/src/error.c
        wolfSSL/wolfcrypt/src/fe_low_mem.c
        wolfSSL/wolfcrypt/src/fe_operations.c
        wolfSSL/wolfcrypt/src/ge_low_mem.c
        wolfSSL/wolfcrypt/src/ge_operations.c
        wolfSSL/wolfcrypt/src/hash.c
        wolfSSL/wolfcrypt/src/hc128.c
        wolfSSL/wolfcrypt/src/hmac.c
        wolfSSL/wolfcrypt/src/idea.c
        wolfSSL/wolfcrypt/src/integer.c
        wolfSSL/wolfcrypt/src/logging.c
        wolfSSL/wolfcrypt/src/md2.c
        wolfSSL/wolfcrypt/src/md4.c
        wolfSSL/wolfcrypt/src/md5.c
        wolfSSL/wolfcrypt/src/memory.c
        wolfSSL/wolfcrypt/src/misc.c
        wolfSSL/wolfcrypt/src/pkcs7.c
        wolfSSL/wolfcrypt/src/poly1305.c
        wolfSSL/wolfcrypt/src/pwdbased.c
        wolfSSL/wolfcrypt/src/rabbit.c
        wolfSSL/wolfcrypt/src/random.c
        wolfSSL/wolfcrypt/src/ripemd.c
        wolfSSL/wolfcrypt/src/rsa.c
        wolfSSL/wolfcrypt/src/sha.c
        wolfSSL/wolfcrypt/src/sha256.c
        wolfSSL/wolfcrypt/src/sha512.c
        wolfSSL/wolfcrypt/src/signature.c
        wolfSSL/wolfcrypt/src/srp.c
        wolfSSL/wolfcrypt/src/tfm.c
        wolfSSL/wolfcrypt/src/wc_encrypt.c
        wolfSSL/wolfcrypt/src/wc_port.c
        )
target_compile_definitions(wolfSSL PUBLIC -DWOLFSSL_BASE64_ENCODE)
target_include_directories(wolfSSL PUBLIC wolfSSL)

add_library(CRYPTO crypto/crypto.c)
target_link_libraries(CRYPTO PUBLIC wolfSSL)
target_include_directories(CRYPTO PUBLIC crypto)

add_library(JSMN jsmn/jsmn.c)
target_include_directories(JSMN PUBLIC jsmn)

add_library(VLLS mbed-kinetis-lowpower/kinetis_lowpower.cpp)
target_include_directories(VLLS PUBLIC mbed-kinetis-lowpower)

add_library(HTTP mbed-http/http_parser/http_parser.c)
target_include_directories(HTTP PUBLIC mbed-http/http_parser
                                       mbed-http/source
                                       mbed-http)

add_library(DBGUTILS dbgutil/dbgutil.c)
target_include_directories(DBGUTILS PUBLIC dbgutil)

add_library(BME280 BME280/BME280.cpp)
target_include_directories(BME280 PUBLIC BME280)

add_executable(airquality src/main.cpp src/response.c)

target_link_libraries(airquality mbed-os AirQualitySensor
        wolfSSL
        mbed-os-quectelM66-driver
        VLLS
        MQTT
        JSMN
        CRYPTO
        HTTP
        BME280
        DBGUTILS
        )

add_custom_target(airquality-compile ALL
        COMMAND mbed compile --profile mbed-os/tools/profiles/debug.json
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})