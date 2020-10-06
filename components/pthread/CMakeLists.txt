

idf_component_register(
    SRC_DIRS "Lab-Project-FreeRTOS-POSIX/FreeRTOS-Plus-POSIX/source"
    INCLUDE_DIRS    "include"                           
                    "Lab-Project-FreeRTOS-POSIX/FreeRTOS-Plus-POSIX/include"
                    "Lab-Project-FreeRTOS-POSIX/FreeRTOS-Plus-POSIX/include/portable" 
                    "Lab-Project-FreeRTOS-POSIX/include"
                    "Lab-Project-FreeRTOS-POSIX/include/private"
    REQUIRES freertos        
)