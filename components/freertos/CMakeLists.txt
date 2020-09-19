idf_component_register(
    SRC_DIRS "FreeRTOS-Labs/FreeRTOS/Source" "port/esp8266"
    INCLUDE_DIRS "FreeRTOS-Labs/FreeRTOS/Source/include" "include" "port/esp8266/include/freertos"
    REQUIRES libgloss xtensa_rtos_sdk
    EXCLUDE_SRCS "port/esp8266/impure.c"
    #    PRIV_INCLUDE_DIRS "./src/include"
)