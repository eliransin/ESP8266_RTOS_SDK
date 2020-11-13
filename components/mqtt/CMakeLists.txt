set(COMPONENT_ADD_INCLUDEDIRS "esp-mqtt/include")
set(COMPONENT_PRIV_INCLUDEDIRS "esp-mqtt/lib/include")
set(COMPONENT_SRCS "esp-mqtt/mqtt_client.c"
                   "esp-mqtt/lib/mqtt_msg.c"
                   "esp-mqtt/lib/mqtt_outbox.c"
                   "esp-mqtt/lib/platform_idf.c")

set(COMPONENT_REQUIRES freertos_tcp http_parser tcp_transport freertos freertos_tcp mbedtls openssl)

register_component()
