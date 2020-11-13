

idf_component_register(
    SRC_DIRS "FreeRTOS-Plus-TCP" "FreeRTOS-Plus-TCP/portable/BufferManagement"
    INCLUDE_DIRS    "FreeRTOS-Plus-TCP/include"
                    "FreeRTOS-Plus-TCP/portable/Compiler/GCC"
                    "include"
                    "posix/include"         
    EXCLUDE_SRCS "FreeRTOS-Plus-TCP/FreeRTOS_DHCP_IPv6.c" "FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_1.c"
    REQUIRES freertos        
)