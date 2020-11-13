set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_EXCLUDE_SRCS "tcpip_adapter_lwip.c")
set(COMPONENT_REQUIRES "freertos_tcp" "esp8266")

register_component()

#component_compile_options("-DLWIP_OPEN_SRC")
