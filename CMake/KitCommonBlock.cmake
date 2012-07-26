
if(NOT IBAMR_BUILD_SINGLE_LIBRARY)
  add_library(ibamr${KIT}3D ${kit_SRCS3D})
  set_target_properties(ibamr${KIT}3D PROPERTIES COMPILE_DEFINITIONS NDIM=3)

  if(IBAMR_BUILD_2D_LIBS)
    add_library(ibamr${KIT}2D ${kit_SRCS2D})
    set_target_properties(ibamr${KIT}2D PROPERTIES COMPILE_DEFINITIONS NDIM=2)
  endif() 
else()
  add_library(ibamr${KIT}3D OBJECT ${kit_SRCS3D})
  if(BUILD_SHARED_LIBS)
    set_target_properties(ibamr${KIT}3D PROPERTIES COMPILE_FLAGS "-fPIC")
  endif()
  set_target_properties(ibamr${KIT}3D PROPERTIES COMPILE_DEFINITIONS NDIM=3)
  if(IBAMR_BUILD_2D_LIBS)
    add_library(ibamr${KIT}2D OBJECT ${kit_SRCS2D})
    if(BUILD_SHARED_LIBS)
      set_target_properties(ibamr${KIT}2D PROPERTIES COMPILE_FLAGS "-fPIC")
    endif()
    set_target_properties(ibamr${KIT}2D PROPERTIES COMPILE_DEFINITIONS NDIM=2)
  endif()
endif()
