

idf_component_register(
    SRC_DIRS "FreeRTOS-Plus-TCP"
    INCLUDE_DIRS    "FreeRTOS-Plus-TCP/include"
                    "FreeRTOS-Plus-TCP/portable/Compiler/GCC"
                    "include"                  
    REQUIRES freertos        
)