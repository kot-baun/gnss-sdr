/*!
 * \file gps_l1_ca_observables_cc.h
 * \brief Pseudorange computation module for GPS L1 C/A
 * \author Javier Arribas, 2011. jarribas(at)cttc.es
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2011  (see AUTHORS file for a list of contributors)
 *
 * GNSS-SDR is a software defined Global Navigation
 *          Satellite Systems receiver
 *
 * This file is part of GNSS-SDR.
 *
 * GNSS-SDR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option) any later version.
 *
 * GNSS-SDR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNSS-SDR. If not, see <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------
 */


#ifndef GPS_L1_CA_OBSERVABLES_CC_H
#define	GPS_L1_CA_OBSERVABLES_CC_H

#include <fstream>

#include <gnuradio/gr_block.h>
#include <gnuradio/gr_msg_queue.h>

#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include "concurrent_queue.h"

#include "gps_navigation_message.h"

#include "rinex_2_1_printer.h"

#include "GPS_L1_CA.h"

class gps_l1_ca_observables_cc;
typedef boost::shared_ptr<gps_l1_ca_observables_cc> gps_l1_ca_observables_cc_sptr;
gps_l1_ca_observables_cc_sptr
gps_l1_ca_make_observables_cc(unsigned int n_channels, gr_msg_queue_sptr queue, bool dump, std::string dump_filename, int output_rate_ms, bool flag_averaging);


class gps_l1_ca_observables_cc : public gr_block {

private:

  friend gps_l1_ca_observables_cc_sptr
  gps_l1_ca_make_observables_cc(unsigned int nchannels, gr_msg_queue_sptr queue, bool dump, std::string dump_filename, int output_rate_ms, bool flag_averaging);
  gps_l1_ca_observables_cc(unsigned int nchannels, gr_msg_queue_sptr queue, bool dump, std::string dump_filename, int output_rate_ms, bool flag_averaging);

  // class private vars
  gr_msg_queue_sptr d_queue;
  bool d_dump;
  bool d_flag_averaging;
  long int d_sample_counter;
  unsigned int d_nchannels;
  unsigned long int d_fs_in;
  int d_output_rate_ms;
  std::string d_dump_filename;
  std::ofstream d_dump_file;

  std::deque<double> *d_history_prn_delay_ms;

  concurrent_queue<gps_navigation_message> *d_nav_queue; // Navigation ephemeris queue

public:

  ~gps_l1_ca_observables_cc ();

  void set_navigation_queue(concurrent_queue<gps_navigation_message> *nav_queue){d_nav_queue=nav_queue;}

  void set_fs_in(unsigned long int fs_in) {d_fs_in=fs_in;};

  int general_work (int noutput_items, gr_vector_int &ninput_items,
      gr_vector_const_void_star &input_items, gr_vector_void_star &output_items);

};

#endif