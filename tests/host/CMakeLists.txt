cmake_minimum_required(VERSION 3.22)
project(defender_host_tests LANGUAGES C)
enable_testing()

set(camera_component "${CMAKE_CURRENT_LIST_DIR}/../../firmware/components/monitor_camera")
set(camera_driver "${CMAKE_CURRENT_LIST_DIR}/../../firmware/managed_components/espressif__esp32-camera")
if(NOT EXISTS "${camera_driver}/driver/include/esp_camera.h")
    message(FATAL_ERROR "Configure/build firmware first to resolve its locked camera dependency.")
endif()

# Compile the real capture component against the pinned driver's public headers.
# Only SDK functions/types below the component boundary are replaced by fakes.
add_executable(camera_tests
    "${camera_component}/monitor_camera.c"
    camera_tests.c
    fakes/fake_sdk.c
)
target_include_directories(camera_tests PRIVATE
    "${camera_component}/include"
    fakes
    "${camera_driver}/driver/include"
    "${camera_driver}/conversions/include"
    "${CMAKE_CURRENT_LIST_DIR}/../../firmware/managed_components/espressif__esp_jpeg/include"
)
set_target_properties(camera_tests PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO
)
if(MSVC)
    target_compile_options(camera_tests PRIVATE /W4 /WX)
else()
    target_compile_options(camera_tests PRIVATE -Wall -Wextra -Werror)
endif()

set(camera_scenarios
    uninitialized_has_no_frame
    recorded_profile_uses_one_psram_buffer
    missing_psram_is_not_ready
    sdk_initialization_failure_is_not_ready
    gpio_failure_stops_initialization
    wrong_sensor_is_rejected
    missing_sensor_is_rejected
    cleanup_failure_keeps_original_error
    reinitialization_preserves_session_and_progress
    identical_scene_with_new_timestamp_is_new_acquisition
    timeout_preserves_historical_progress
    held_frame_prevents_new_acquisition
    acquiring_into_live_lease_preserves_it
    copied_lease_cannot_release_buffer
    double_release_does_not_return_buffer_again
    source_timestamp_is_not_receive_time
    stale_frame_is_returned_without_delivery
    duplicate_timestamp_is_not_new_acquisition
    backwards_timestamp_is_not_new_acquisition
    future_timestamp_is_rejected
    invalid_timeval_is_rejected
    malformed_frame_is_returned_without_delivery
    unexpected_pixel_format_is_rejected
    missing_image_bytes_are_rejected
    zero_capture_timestamp_is_rejected
    bad_frame_does_not_prevent_fresh_recovery
    maximum_allowed_age_is_inclusive
    invalid_arguments_do_not_acquire
)
foreach(scenario IN LISTS camera_scenarios)
    add_test(NAME "camera.${scenario}" COMMAND camera_tests "${scenario}")
endforeach()
