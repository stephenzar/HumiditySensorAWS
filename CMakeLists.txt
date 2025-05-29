cmake_minimum_required(VERSION 3.16.0)

# Ruta al SDK AWS IoT que debe estar en esta ubicación
list(APPEND EXTRA_COMPONENT_DIRS "common/components/esp-aws-iot")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Humidity_IOT_AWS)


FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
set(COMPONENT_ADD_INCLUDEDIRS ".")

idf_component_register(SRCS ${app_sources})

# Incluir archivos criptográficos en el binario
target_add_binary_data(${COMPONENT_TARGET} "../tmp/AmazonRootCA1.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "../tmp/my_sensor1.cert.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "../tmp/my_sensor1.private.key" TEXT)
