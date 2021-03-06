# Copyright (C) 2012-2014  (see AUTHORS file for a list of contributors)
#
# This file is part of GNSS-SDR.
#
# GNSS-SDR is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GNSS-SDR is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNSS-SDR. If not, see <http://www.gnu.org/licenses/>.
#

if(OPENCL_FOUND)
    set(ACQ_ADAPTER_SOURCES
         gps_l1_ca_pcps_acquisition.cc
         gps_l1_ca_pcps_multithread_acquisition.cc
         gps_l1_ca_pcps_assisted_acquisition.cc
         gps_l1_ca_pcps_acquisition_fine_doppler.cc
         gps_l1_ca_pcps_tong_acquisition.cc
         gps_l1_ca_pcps_opencl_acquisition.cc
         galileo_e1_pcps_ambiguous_acquisition.cc
         galileo_e1_pcps_cccwsr_ambiguous_acquisition.cc
         galileo_e1_pcps_tong_ambiguous_acquisition.cc
         galileo_e1_pcps_8ms_ambiguous_acquisition.cc
    )
else(OPENCL_FOUND)
    set(ACQ_ADAPTER_SOURCES
         gps_l1_ca_pcps_acquisition.cc
         gps_l1_ca_pcps_multithread_acquisition.cc
         gps_l1_ca_pcps_assisted_acquisition.cc
         gps_l1_ca_pcps_acquisition_fine_doppler.cc
         gps_l1_ca_pcps_tong_acquisition.cc
         galileo_e1_pcps_ambiguous_acquisition.cc
         galileo_e1_pcps_cccwsr_ambiguous_acquisition.cc
         galileo_e1_pcps_tong_ambiguous_acquisition.cc
         galileo_e1_pcps_8ms_ambiguous_acquisition.cc
    )
endif(OPENCL_FOUND)

include_directories(
     $(CMAKE_CURRENT_SOURCE_DIR)
     ${CMAKE_SOURCE_DIR}/src/core/system_parameters
     ${CMAKE_SOURCE_DIR}/src/core/interfaces
     ${CMAKE_SOURCE_DIR}/src/core/receiver
     ${CMAKE_SOURCE_DIR}/src/algorithms/acquisition/gnuradio_blocks
     ${CMAKE_SOURCE_DIR}/src/algorithms/libs
     ${Boost_INCLUDE_DIRS}
     ${GLOG_INCLUDE_DIRS}
     ${GFlags_INCLUDE_DIRS}
     ${GNURADIO_RUNTIME_INCLUDE_DIRS}
)

file(GLOB ACQ_ADAPTER_HEADERS "*.h")
add_library(acq_adapters ${ACQ_ADAPTER_SOURCES} ${ACQ_ADAPTER_HEADERS})
source_group(Headers FILES ${ACQ_ADAPTER_HEADERS}) 
target_link_libraries(acq_adapters gnss_sp_libs acq_gr_blocks ${Boost_LIBRARIES} ${GNURADIO_RUNTIME_LIBRARIES} ${GNURADIO_BLOCKS_LIBRARIES})
