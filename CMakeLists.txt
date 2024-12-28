find_package(MAVLink REQUIRED)

add_executable(Mavlink telemetry.cpp)

target_link_libraries(Mavlink telemetry PRIVATE MAVLink::mavlink)