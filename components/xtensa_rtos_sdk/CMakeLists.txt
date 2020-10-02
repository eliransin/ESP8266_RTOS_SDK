idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src/include"
    LDFRAGMENTS "linker.lf"
)
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    -DXTENSA_CONFIG_FREERTOS_ISR_STACKSIZE=${CONFIG_FREERTOS_ISR_STACKSIZE})