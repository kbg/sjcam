if(BUILD_SERVER)
  add_subdirectory(server)
endif()

if(BUILD_CLIENT)
  add_subdirectory(client)
endif()

if (BUILD_STARTER)
  add_subdirectory(starter)
endif()
