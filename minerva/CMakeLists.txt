project(MinervaLib)

include_directories(
  ${MinervaLib_SOURCE_DIR}
)

file(GLOB_RECURSE src_file_list "*.cpp")
file(GLOB_RECURSE cuda_src_file_list "*.cu")

if (NOT HAS_PS)
  if (CUDA_FOUND)
    CUDA_ADD_LIBRARY(minerva SHARED ${src_file_list} ${cuda_src_file_list})
    CUDA_ADD_CUBLAS_TO_TARGET(minerva)
  else (CUDA_FOUND)
    add_library(minerva SHARED ${src_file_list})
  endif (CUDA_FOUND)
else (NOT HAS_PS)
  if (CUDA_FOUND)
    CUDA_ADD_LIBRARY(minerva STATIC ${src_file_list} ${cuda_src_file_list})
    CUDA_ADD_CUBLAS_TO_TARGET(minerva)
  else (CUDA_FOUND)
    add_library(minerva STATIC ${src_file_list})
  endif (CUDA_FOUND)

  target_link_libraries(minerva minervaps)
endif (NOT HAS_PS)

target_link_libraries(minerva glog gflags boost_thread boost_system)
if (CUDA_FOUND)
  target_link_libraries(minerva cudnn ${CUDA_curand_LIBRARY})
endif (CUDA_FOUND)
set(LIBRARY_OUTPUT_PATH ${Minerva_BINARY_DIR}/lib)

install(TARGETS minerva DESTINATION lib)
