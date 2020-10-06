idf_component_register(
    SRC_DIRS "FreeRTOS-Kernel" "port/esp8266"
    INCLUDE_DIRS "FreeRTOS-Kernel/include" "include" "port/esp8266/include/freertos" "port/esp8266/include/"
    REQUIRES libgloss xtensa_rtos_sdk
)
 