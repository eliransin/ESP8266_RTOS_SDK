set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS "include")

set(COMPONENT_REQUIRES "freertos_tcp")

register_component()

component_compile_options("-DLWIP_OPEN_SRC")
