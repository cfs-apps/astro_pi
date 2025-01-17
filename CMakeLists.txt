project(CFS_ASTRO_PI C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src)
include_directories(${app_c_fw_MISSION_DIR}/fsw/app_inc)
include_directories(${app_c_fw_MISSION_DIR}/fsw/platform_inc)
include_directories(${app_c_fw_MISSION_DIR}/fsw/mission_inc)
include_directories(${jmsg_lib_MISSION_DIR}/fsw/platform_inc)
include_directories(${jmsg_lib_MISSION_DIR}/fsw/mission_inc)
include_directories(${jmsg_lib_MISSION_DIR}/fsw/src)
include_directories(${jmsg_lib_MISSION_DIR}/fsw/topic_plugins)

aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(astro_pi ${APP_SRC_FILES})