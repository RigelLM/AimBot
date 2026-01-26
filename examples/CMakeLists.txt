# Collect all example mains
file(GLOB EXP_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
)

foreach(src ${EXP_SOURCES})
  get_filename_component(exe_name ${src} NAME_WE)

  add_executable(${exe_name} ${src})
  target_link_libraries(${exe_name} PRIVATE aimbot_core)

  # Copy OpenCV DLLs so the exe can run on Windows
  copy_opencv_dlls_to_target(${exe_name})

  # group in IDE
  set_target_properties(${exe_name} PROPERTIES FOLDER "examples")
endforeach()
