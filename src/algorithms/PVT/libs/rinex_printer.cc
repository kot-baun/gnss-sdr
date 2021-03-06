/*!
 * \file rinex_printer.cc
 * \brief Implementation of a RINEX 2.11 / 3.01 printer
 * See http://igscb.jpl.nasa.gov/igscb/data/format/rinex301.pdf
 * \author Carles Fernandez Prades, 2011. cfernandez(at)cttc.es
 * -------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2014  (see AUTHORS file for a list of contributors)
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

#include "rinex_printer.h"
#include <algorithm> // for min and max
#include <cmath>     // for floor
#include <cstdlib>   // for getenv()
#include <ostream>
#include <utility>
#include <vector>
#include <boost/date_time/time_zone_base.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "sbas_telemetry_data.h"
#include "gps_navigation_message.h"
#include "gps_ephemeris.h"
#include "gps_iono.h"
#include "gps_utc_model.h"


using google::LogMessage;

DEFINE_string(RINEX_version, "2.11", "Specifies the RINEX version (2.11 or 3.01)");


Rinex_Printer::Rinex_Printer()
{
    navfilename = Rinex_Printer::createFilename("RINEX_FILE_TYPE_GPS_NAV");
    obsfilename = Rinex_Printer::createFilename("RINEX_FILE_TYPE_OBS");
    sbsfilename = Rinex_Printer::createFilename("RINEX_FILE_TYPE_SBAS");

    Rinex_Printer::navFile.open(navfilename, std::ios::out | std::ios::app);
    Rinex_Printer::obsFile.open(obsfilename, std::ios::out | std::ios::app);
    Rinex_Printer::sbsFile.open(sbsfilename, std::ios::out | std::ios::app);

    // RINEX v3.00 codes
    satelliteSystem["GPS"] = "G";
    satelliteSystem["GLONASS"] = "R";
    satelliteSystem["SBAS payload"] = "S";
    satelliteSystem["Galileo"] = "E";
    satelliteSystem["Compass"] = "C";

    observationCode["GPS_L1_CA"] = "1C";              // "1C" GPS L1 C/A
    observationCode["GPS_L1_P"] = "1P";               // "1P" GPS L1 P
    observationCode["GPS_L1_Z_TRACKING"] = "1W";      // "1W" GPS L1 Z-tracking and similar (AS on)
    observationCode["GPS_L1_Y"] = "1Y";               // "1Y" GPS L1 Y
    observationCode["GPS_L1_M "] = "1M";              // "1M" GPS L1 M
    observationCode["GPS_L1_CODELESS"] = "1N";        // "1N" GPS L1 codeless
    observationCode["GPS_L2_CA"] = "2C";              // "2C" GPS L2 C/A
    observationCode["L2_SEMI_CODELESS"] = "2D";       // "2D" GPS L2 L1(C/A)+(P2-P1) semi-codeless
    observationCode["GPS_L2_L2CM"] = "2S";            // "2S" GPS L2 L2C (M)
    observationCode["GPS_L2_L2CL"] = "2L";            // "2L" GPS L2 L2C (L)
    observationCode["GPS_L2_L2CML"] = "2X";           // "2X" GPS L2 L2C (M+L)
    observationCode["GPS_L2_P"] = "2P";               // "2P" GPS L2 P
    observationCode["GPS_L2_Z_TRACKING"] = "2W";      // "2W" GPS L2 Z-tracking and similar (AS on)
    observationCode["GPS_L2_Y"] = "2Y";               // "2Y" GPS L2 Y
    observationCode["GPS_L2_M"] = "2M";               // "2M" GPS GPS L2 M
    observationCode["GPS_L2_codeless"] = "2N";        // "2N" GPS L2 codeless
    observationCode["GPS_L5_I"] = "5I";               // "5I" GPS L5 I
    observationCode["GPS_L5_Q"] = "5Q";               // "5Q" GPS L5 Q
    observationCode["GPS_L5_IQ"] = "5X";              // "5X" GPS L5 I+Q
    observationCode["GLONASS_G1_CA"] = "1C";          // "1C" GLONASS G1 C/A
    observationCode["GLONASS_G1_P"] = "1P";           // "1P" GLONASS G1 P
    observationCode["GLONASS_G2_CA"] = "2C";          // "2C" GLONASS G2 C/A  (Glonass M)
    observationCode["GLONASS_G2_P"] = "2P";           // "2P" GLONASS G2 P
    observationCode["GALILEO_E1_A"] = "1A";           // "1A" GALILEO E1 A (PRS)
    observationCode["GALILEO_E1_B"] = "1B";           // "1B" GALILEO E1 B (I/NAV OS/CS/SoL)
    observationCode["GALILEO_E1_C"] = "1C";           // "1C" GALILEO E1 C (no data)
    observationCode["GALILEO_E1_BC"] = "1X";          // "1X" GALILEO E1 B+C
    observationCode["GALILEO_E1_ABC"] = "1Z";         // "1Z" GALILEO E1 A+B+C
    observationCode["GALILEO_E5a_I"] = "5I";          // "5I" GALILEO E5a I (F/NAV OS)
    observationCode["GALILEO_E5a_Q"] = "5Q";          // "5Q" GALILEO E5a Q  (no data)
    observationCode["GALILEO_E5aIQ"] = "5X";          // "5X" GALILEO E5a I+Q
    observationCode["GALILEO_E5b_I"] = "7I";          // "7I" GALILEO E5b I
    observationCode["GALILEO_E5b_Q"] = "7Q";          // "7Q" GALILEO E5b Q
    observationCode["GALILEO_E5b_IQ"] = "7X";         // "7X" GALILEO E5b I+Q
    observationCode["GALILEO_E5_I"] = "8I";           // "8I" GALILEO E5 I
    observationCode["GALILEO_E5_Q"] = "8Q";           // "8Q" GALILEO E5 Q
    observationCode["GALILEO_E5_IQ"] = "8X";          // "8X" GALILEO E5 I+Q
    observationCode["GALILEO_E56_A"] = "6A";          // "6A" GALILEO E6 A
    observationCode["GALILEO_E56_B"] = "6B";          // "6B" GALILEO E6 B
    observationCode["GALILEO_E56_B"] = "6C";          // "6C" GALILEO E6 C
    observationCode["GALILEO_E56_BC"] = "6X";         // "6X" GALILEO E6 B+C
    observationCode["GALILEO_E56_ABC"] = "6Z";        // "6Z" GALILEO E6 A+B+C
    observationCode["SBAS_L1_CA"] = "1C";             // "1C" SBAS L1 C/A
    observationCode["SBAS_L5_I"] = "5I";              // "5I" SBAS L5 I
    observationCode["SBAS_L5_Q"] = "5Q";              // "5Q" SBAS L5 Q
    observationCode["SBAS_L5_IQ"] = "5X";             // "5X" SBAS L5 I+Q
    observationCode["COMPASS_E2_I"] = "2I";
    observationCode["COMPASS_E2_Q"] = "2Q";
    observationCode["COMPASS_E2_IQ"] = "2X";
    observationCode["COMPASS_E5b_I"] = "7I";
    observationCode["COMPASS_E5b_Q"] = "7Q";
    observationCode["COMPASS_E5b_IQ"] = "7X";
    observationCode["COMPASS_E6_I"] = "6I";
    observationCode["COMPASS_E6_Q"] = "6Q";
    observationCode["COMPASS_E6_IQ"] = "6X";

    observationType["PSEUDORANGE"] = "C";
    observationType["CARRIER_PHASE"] = "L";
    observationType["DOPPLER"] = "D";
    observationType["SIGNAL_STRENGTH"] = "S";

    // RINEX v2.10 and v2.11 codes
    observationType["PSEUDORANGE_CA_v2"] = "C";
    observationType["PSEUDORANGE_P_v2"] = "P";
    observationType["CARRIER_PHASE_CA_v2"] = "L";
    observationType["DOPPLER_v2"] = "D";
    observationType["SIGNAL_STRENGTH_v2"] = "S";
    observationCode["GPS_L1_CA_v2"] = "1";

    if ( FLAGS_RINEX_version.compare("3.01") == 0 )
        {
            version = 3;
            stringVersion = "3.01";
        }
    else if ( FLAGS_RINEX_version.compare("2.11") == 0 )
        {
            version = 2;
            stringVersion = "2.10";
        }
    else if ( FLAGS_RINEX_version.compare("2.10") == 0 )
           {
               version = 2;
               stringVersion = "2.10";
           }
    else
        {
            LOG(ERROR) << "Unknown RINEX version " << FLAGS_RINEX_version << " (must be 2.11 or 3.01)" << std::endl;
        }

    numberTypesObservations = 2; // Number of available types of observable in the system
}



Rinex_Printer::~Rinex_Printer()
{
    // close RINEX files
    long posn, poso, poss;
    posn = navFile.tellp();
    poso = obsFile.tellp();
    poss = obsFile.tellp();
    Rinex_Printer::navFile.close();
    Rinex_Printer::obsFile.close();
    Rinex_Printer::sbsFile.close();
    // If nothing written, erase the files.
    if (posn == 0)
        {
            remove(navfilename.c_str());
        }
    if (poso == 0)
        {
            remove(obsfilename.c_str());
        }
    if (poss == 0)
        {
            remove(sbsfilename.c_str());
        }
}




void Rinex_Printer::lengthCheck(std::string line)
{
    if (line.length() != 80)
        {
            LOG(ERROR) << "Bad defined RINEX line: "
                    << line.length() << " characters (must be 80)" << std::endl
                    << line << std::endl
                    << "----|---1|0---|---2|0---|---3|0---|---4|0---|---5|0---|---6|0---|---7|0---|---8|" << std::endl;
        }
}



std::string Rinex_Printer::createFilename(std::string type)
{
    const std::string stationName = "GSDR"; // 4-character station name designator
    boost::gregorian::date today = boost::gregorian::day_clock::local_day();
    const int dayOfTheYear = today.day_of_year();
    std::stringstream strm0;
    if (dayOfTheYear < 100) strm0 << "0"; // three digits for day of the year
    if (dayOfTheYear < 10) strm0 << "0"; // three digits for day of the year
    strm0 << dayOfTheYear;
    std::string dayOfTheYearTag=strm0.str();

    std::map<std::string, std::string> fileType;
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_OBS", "O"));        // O - Observation file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_GPS_NAV", "N"));    // N - GPS navigation message file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_MET", "M"));        // M - Meteorological data file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_GLO_NAV", "G"));    // G - GLONASS navigation file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_GAL_NAV", "L"));    // L - Galileo navigation message file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_MIXED_NAV", "P"));  // P - Mixed GNSS navigation message file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_GEO_NAV", "H"));    // H - SBAS Payload navigation message file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_SBAS", "B"));       // B - SBAS broadcast data file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_CLK", "C"));        // C - Clock file.
    fileType.insert(std::pair<std::string, std::string>("RINEX_FILE_TYPE_SUMMARY", "S"));    // S - Summary file (used e.g., by IGS, not a standard!).

    boost::posix_time::ptime pt = boost::posix_time::second_clock::local_time();
    tm pt_tm = boost::posix_time::to_tm(pt);
    int local_hour = pt_tm.tm_hour;
    std::stringstream strm;
    strm << local_hour;

    std::map<std::string, std::string> Hmap;
    Hmap.insert(std::pair<std::string, std::string>("0", "a"));
    Hmap.insert(std::pair<std::string, std::string>("1", "b"));
    Hmap.insert(std::pair<std::string, std::string>("2", "c"));
    Hmap.insert(std::pair<std::string, std::string>("3", "d"));
    Hmap.insert(std::pair<std::string, std::string>("4", "e"));
    Hmap.insert(std::pair<std::string, std::string>("5", "f"));
    Hmap.insert(std::pair<std::string, std::string>("6", "g"));
    Hmap.insert(std::pair<std::string, std::string>("7", "h"));
    Hmap.insert(std::pair<std::string, std::string>("8", "i"));
    Hmap.insert(std::pair<std::string, std::string>("9", "j"));
    Hmap.insert(std::pair<std::string, std::string>("10", "k"));
    Hmap.insert(std::pair<std::string, std::string>("11", "l"));
    Hmap.insert(std::pair<std::string, std::string>("12", "m"));
    Hmap.insert(std::pair<std::string, std::string>("13", "n"));
    Hmap.insert(std::pair<std::string, std::string>("14", "o"));
    Hmap.insert(std::pair<std::string, std::string>("15", "p"));
    Hmap.insert(std::pair<std::string, std::string>("16", "q"));
    Hmap.insert(std::pair<std::string, std::string>("17", "r"));
    Hmap.insert(std::pair<std::string, std::string>("18", "s"));
    Hmap.insert(std::pair<std::string, std::string>("19", "t"));
    Hmap.insert(std::pair<std::string, std::string>("20", "u"));
    Hmap.insert(std::pair<std::string, std::string>("21", "v"));
    Hmap.insert(std::pair<std::string, std::string>("22", "w"));
    Hmap.insert(std::pair<std::string, std::string>("23", "x"));

    std::string hourTag = Hmap[strm.str()];

    int local_minute = pt_tm.tm_min;
    std::stringstream strm2;
    if (local_minute<10) strm2 << "0"; // at least two digits for minutes
    strm2 << local_minute;

    std::string minTag = strm2.str();

    int local_year = pt_tm.tm_year - 100; // 2012 is 112
    std::stringstream strm3;
    strm3 << local_year;
    std::string yearTag = strm3.str();

    std::string typeOfFile = fileType[type];

    std::string filename(stationName + dayOfTheYearTag + hourTag + minTag + "." + yearTag + typeOfFile);
    return filename;
}


std::string Rinex_Printer::getLocalTime()
{
    std::string line;
    line += std::string("GNSS-SDR");
    line += std::string(12, ' ');
    line += Rinex_Printer::leftJustify("CTTC", 20); //put a flag to let the user change this
    boost::gregorian::date today = boost::gregorian::day_clock::local_day();

    boost::local_time::time_zone_ptr zone(new boost::local_time::posix_time_zone("UTC"));
    boost::local_time::local_date_time pt = boost::local_time::local_sec_clock::local_time(zone);
    tm pt_tm = boost::local_time::to_tm(pt);

    std::stringstream strmHour;
    int utc_hour = pt_tm.tm_hour;
    if (utc_hour < 10) strmHour << "0"; //  two digits for hours
    strmHour << utc_hour;

    std::stringstream strmMin;
    int utc_minute = pt_tm.tm_min;
    if (utc_minute < 10) strmMin << "0"; //  two digits for minutes
    strmMin << utc_minute;

    if (version == 2)
        {
            int day = pt_tm.tm_mday;
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(day), 2);
            line += std::string("-");

            std::map<int, std::string> months;
            months[0] = "JAN";
            months[1] = "FEB";
            months[2] = "MAR";
            months[3] = "APR";
            months[4] = "MAY";
            months[5] = "JUN";
            months[6] = "JUL";
            months[7] = "AUG";
            months[8] = "SEP";
            months[9] = "OCT";
            months[10] = "NOV";
            months[11] = "DEC";

            line += months[pt_tm.tm_mon];
            line += std::string("-");
            line += boost::lexical_cast<std::string>(pt_tm.tm_year - 100);
            line += std::string(1, ' ');
            line += strmHour.str();
            line += std::string(":");
            line += strmMin.str();
            line += std::string(5, ' ');
        }

    if (version == 3)
        {
            line += std::string(1, ' ');
            line += boost::gregorian::to_iso_string(today);
            line += strmHour.str();
            line += strmMin.str();

            std::stringstream strm2;
            int utc_seconds = pt_tm.tm_sec;
            if (utc_seconds < 10) strm2 << "0"; //  two digits for seconds
            strm2 << utc_seconds;
            line += strm2.str();
            line += std::string(1, ' ');
            line += std::string("UTC");
            line += std::string(1, ' ');
        }
    return line;
}


void Rinex_Printer::rinex_nav_header(std::ofstream& out, Gps_Iono iono, Gps_Utc_Model utc_model)
{

    std::string line;

    // -------- Line 1
    line = std::string(5, ' ');
    line += stringVersion;
    line += std::string(11, ' ');

    if (version == 2)
        {
            line += std::string("N: GPS NAV DATA");
            line += std::string(25, ' ');
        }

    if (version == 3 )
        {
            line += std::string("N: GNSS NAV DATA");
            line += std::string(4, ' ');
            //! \todo Add here other systems...
            line += std::string("G: GPS");
            line += std::string(14, ' ');
            // ...

        }

    line += std::string("RINEX VERSION / TYPE");
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line 2
    line.clear();
    line += Rinex_Printer::getLocalTime();
    line += std::string("PGM / RUN BY / DATE");
    line += std::string(1, ' ');
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line 3
    line.clear();
    line += Rinex_Printer::leftJustify("GPS NAVIGATION MESSAGE FILE GENERATED BY GNSS-SDR", 60);
    line += Rinex_Printer::leftJustify("COMMENT", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line COMMENT
    line.clear();
    line += Rinex_Printer::leftJustify("See http://gnss-sdr.org", 60);
    line += Rinex_Printer::leftJustify("COMMENT", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line ionospheric info 1
    line.clear();
    if (version == 2)
        {
            line += std::string(2, ' ');
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_alpha0, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_alpha1, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_alpha2, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_alpha3, 10, 2), 12);
            line += std::string(10, ' ');
            line += Rinex_Printer::leftJustify("ION ALPHA", 20);
        }
    if (version == 3)
        {
            line += std::string("GPSA");
            line += std::string(1, ' ');
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_alpha0, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_alpha1, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_alpha2, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_alpha3, 10, 2), 12);
            line += std::string(7, ' ');
            line += Rinex_Printer::leftJustify("IONOSPHERIC CORR", 20);
        }
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line ionospheric info 2
    line.clear();
    if (version == 2)
        {
            line += std::string(2, ' ');
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_beta0, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_beta1, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_beta2, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_beta3, 10, 2), 12);
            line += std::string(10, ' ');
            line += Rinex_Printer::leftJustify("ION BETA", 20);
        }

    if (version == 3)
        {
            line += std::string("GPSB");
            line += std::string(1, ' ');
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_beta0, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_beta1, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_beta2, 10, 2), 12);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(iono.d_beta3, 10, 2), 12);
            line += std::string(7, ' ');
            line += Rinex_Printer::leftJustify("IONOSPHERIC CORR", 20);
        }
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line 5 system time correction
    line.clear();
    if (version == 2)
        {
            line += std::string(3, ' ');
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(utc_model.d_A0, 18, 2), 19);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(utc_model.d_A1, 18, 2), 19);
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(utc_model.d_t_OT), 9);
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(utc_model.i_WN_T + 1024), 9); // valid until 2019
            line += std::string(1, ' ');
            line += Rinex_Printer::leftJustify("DELTA-UTC: A0,A1,T,W", 20);
        }

    if (version == 3)
        {
            line += std::string("GPUT");
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(utc_model.d_A0, 16, 2), 18);
            line += Rinex_Printer::rightJustify(Rinex_Printer::doub2for(utc_model.d_A1, 15, 2), 16);
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(utc_model.d_t_OT), 7);
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(utc_model.i_WN_T + 1024), 5);  // valid until 2019
            /*  if ( SBAS )
        {
          line += string(1, ' ');
          line += leftJustify(asString(d_t_OT_SBAS),5);
          line += string(1, ' ');
          line += leftJustify(asString(d_WN_T_SBAS),2);
          line += string(1, ' ');
               }
          else
             */
            line += std::string(10, ' ');
            line += Rinex_Printer::leftJustify("TIME SYSTEM CORR", 20);
        }
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line 6 leap seconds
    // For leap second information, see http://www.endruntechnologies.com/leap.htm
    line.clear();
    line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(utc_model.d_DeltaT_LS), 6);
    if (version == 2)
        {
            line += std::string(54, ' ');
        }
    if (version == 3)
        {
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(utc_model.d_DeltaT_LSF), 6);
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(utc_model.i_WN_LSF), 6);
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(utc_model.i_DN), 6);
            line += std::string(36, ' ');
        }
    line += Rinex_Printer::leftJustify("LEAP SECONDS", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;


    // -------- End of Header
    line.clear();
    line += std::string(60, ' ');
    line += Rinex_Printer::leftJustify("END OF HEADER", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;
}



void Rinex_Printer::rinex_sbs_header(std::ofstream& out)
{
    std::string line;

    // -------- Line 1
    line.clear();
    line = std::string(5, ' ');
    line += std::string("2.10");
    line += std::string(11, ' ');
    line += Rinex_Printer::leftJustify("B SBAS DATA",20);
    line += std::string(20, ' ');
    line += std::string("RINEX VERSION / TYPE");

    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line 2
    line.clear();
    line += Rinex_Printer::leftJustify("GNSS-SDR", 20);
    line += Rinex_Printer::leftJustify("CTTC", 20);
    // Date of file creation (dd-mmm-yy hhmm)
    boost::local_time::time_zone_ptr zone(new boost::local_time::posix_time_zone("UTC"));
    boost::local_time::local_date_time pt = boost::local_time::local_sec_clock::local_time(zone);
    tm pt_tm = boost::local_time::to_tm(pt);
    std::stringstream strYear;
    int utc_year = pt.date().year();
    utc_year -= 2000; //  two digits for year
    strYear << utc_year;
    std::stringstream strMonth;
    int utc_month = pt.date().month().as_number();
    if (utc_month < 10) strMonth << "0"; //  two digits for months
    strMonth << utc_month;
    std::stringstream strmDay;
    int utc_day = pt.date().day().as_number();
    if (utc_day < 10) strmDay << "0"; //  two digits for days
    strmDay << utc_day;
    std::stringstream strmHour;
    int utc_hour = pt_tm.tm_hour;
    if (utc_hour < 10) strmHour << "0"; //  two digits for hours
    strmHour << utc_hour;
    std::stringstream strmMin;
    int utc_minute = pt_tm.tm_min;
    if (utc_minute < 10) strmMin << "0"; //  two digits for minutes
    strmMin << utc_minute;
    std::string time_str;
    time_str += strmDay.str();
    time_str += "-";
    time_str += strMonth.str();
    time_str += "-";
    time_str += strYear.str();
    time_str += " ";
    time_str += strmHour.str();
    time_str += strmMin.str();
    line += Rinex_Printer::leftJustify(time_str, 20);
    line += Rinex_Printer::leftJustify("PGM / RUN BY / DATE", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line 3
    line.clear();
    line += std::string(60, ' ');
    line += Rinex_Printer::leftJustify("REC INDEX/TYPE/VERS", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line COMMENT 1
    line.clear();
    line += Rinex_Printer::leftJustify("BROADCAST DATA FILE FOR GEO SV, GENERATED BY GNSS-SDR", 60);
    line += Rinex_Printer::leftJustify("COMMENT", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line COMMENT 2
    line.clear();
    line += Rinex_Printer::leftJustify("See http://gnss-sdr.org", 60);
    line += Rinex_Printer::leftJustify("COMMENT", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- End of Header
    line.clear();
    line += std::string(60, ' ');
    line += Rinex_Printer::leftJustify("END OF HEADER", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;
}




void Rinex_Printer::log_rinex_nav(std::ofstream& out, std::map<int,Gps_Ephemeris> eph_map)
{
    std::string line;
	std::map<int,Gps_Ephemeris>::iterator gps_ephemeris_iter;

    for(gps_ephemeris_iter = eph_map.begin();
    		gps_ephemeris_iter != eph_map.end();
    		gps_ephemeris_iter++)
    {
            // -------- SV / EPOCH / SV CLK
            boost::posix_time::ptime p_utc_time = Rinex_Printer::compute_GPS_time(gps_ephemeris_iter->second,gps_ephemeris_iter->second.d_TOW);
            std::string timestring = boost::posix_time::to_iso_string(p_utc_time);
            std::string month (timestring, 4, 2);
            std::string day (timestring, 6, 2);
            std::string hour (timestring, 9, 2);
            std::string minutes (timestring, 11, 2);
            std::string seconds (timestring, 13, 2);
            if (version == 2)
                {
                    line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(gps_ephemeris_iter->second.i_satellite_PRN), 2);
                    line += std::string(1, ' ');
                    std::string year (timestring, 2, 2);
                    line += year;
                    line += std::string(1, ' ');
                    line += month;
                    line += std::string(1, ' ');
                    line += day;
                    line += std::string(1, ' ');
                    line += hour;
                    line += std::string(1, ' ');
                    line += minutes;
                    line += std::string(1, ' ');
                    line += seconds;
                    line += std::string(1, '.');
                    std::string decimal = std::string("0");
                    if (timestring.size() > 16)
                        {
                            std::string decimal (timestring, 16, 1);
                        }
                    line += decimal;
                    line += std::string(1, ' ');
                    line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_A_f0, 18, 2);
                    line += std::string(1, ' ');
                    line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_A_f1, 18, 2);
                    line += std::string(1, ' ');
                    line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_A_f2, 18, 2);
                    line += std::string(1, ' ');
                }
            if (version == 3)
                {
                    line += satelliteSystem["GPS"];
                    if (gps_ephemeris_iter->second.i_satellite_PRN < 10)  line += std::string("0");
                    line += boost::lexical_cast<std::string>(gps_ephemeris_iter->second.i_satellite_PRN);
                    std::string year (timestring, 0, 4);
                    line += std::string(1, ' ');
                    line += year;
                    line += std::string(1, ' ');
                    line += month;
                    line += std::string(1, ' ');
                    line += day;
                    line += std::string(1, ' ');
                    line += hour;
                    line += std::string(1, ' ');
                    line += minutes;
                    line += std::string(1, ' ');
                    line += seconds;
                    line += std::string(1, ' ');
                    line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_A_f0, 18, 2);
                    line += std::string(1, ' ');
                    line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_A_f1, 18, 2);
                    line += std::string(1, ' ');
                    line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_A_f2, 18, 2);
                }
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;


            // -------- BROADCAST ORBIT - 1
            line.clear();

            if (version == 2)
                {
                    line += std::string(4, ' ');
                }
            if (version == 3)
                {
                    line += std::string(5, ' ');
                }
            // IODE is not present in ephemeris data
            // If there is a discontinued reception the ephemeris is not validated
            //if (gps_ephemeris_iter->second.d_IODE_SF2 == gps_ephemeris_iter->second.d_IODE_SF3)
            //    {
            //        line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_IODE_SF2, 18, 2);
            //    }
            //else
            //    {
            //        LOG(ERROR) << "Discontinued reception of Frame 2 and 3 " << std::endl;
            //    }
            double d_IODE_SF2 = 0;
            line += Rinex_Printer::doub2for(d_IODE_SF2, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_Crs, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_Delta_n, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_M_0, 18, 2);
            if (version == 2)
                {
                    line += std::string(1, ' ');
                }
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;


            // -------- BROADCAST ORBIT - 2
            line.clear();
            if (version == 2)
                {
                    line += std::string(4, ' ');
                }
            if (version == 3)
                {
                    line += std::string(5, ' ');
                }
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_Cuc, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_e_eccentricity, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_Cus, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_sqrt_A, 18, 2);
            if (version == 2)
                {
                    line += std::string(1, ' ');
                }
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;



            // -------- BROADCAST ORBIT - 3
            line.clear();
            if (version == 2)
                {
                    line += std::string(4, ' ');
                }
            if (version == 3)
                {
                    line += std::string(5, ' ');
                }
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_Toe, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_Cic, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_OMEGA0, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_Cis, 18, 2);
            if (version == 2)
                {
                    line += std::string(1, ' ');
                }
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;




            // -------- BROADCAST ORBIT - 4
            line.clear();
            if (version == 2)
                {
                    line += std::string(4, ' ');
                }
            if (version == 3)
                {
                    line += std::string(5, ' ');
                }
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_i_0, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_Crc, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_OMEGA, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_OMEGA_DOT, 18, 2);
            if (version == 2)
                {
                    line += std::string(1, ' ');
                }
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;



            // -------- BROADCAST ORBIT - 5
            line.clear();
            if (version == 2)
                {
                    line += std::string(4, ' ');
                }
            if (version == 3)
                {
                    line += std::string(5, ' ');
                }
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_IDOT, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for((double)(gps_ephemeris_iter->second.i_code_on_L2), 18, 2);
            line += std::string(1, ' ');
            double GPS_week_continuous_number = (double)(gps_ephemeris_iter->second.i_GPS_week + 1024); // valid until April 7, 2019 (check http://www.colorado.edu/geography/gcraft/notes/gps/gpseow.htm)
            line += Rinex_Printer::doub2for(GPS_week_continuous_number, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for((double)(gps_ephemeris_iter->second.i_code_on_L2), 18, 2);
            if (version == 2)
                {
                    line += std::string(1, ' ');
                }
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;


            // -------- BROADCAST ORBIT - 6
            line.clear();
            if (version == 2)
                {
                    line += std::string(4, ' ');
                }
            if (version == 3)
                {
                    line += std::string(5, ' ');
                }
            line += Rinex_Printer::doub2for((double)(gps_ephemeris_iter->second.i_SV_accuracy), 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for((double)(gps_ephemeris_iter->second.i_SV_health), 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_TGD, 18, 2);
            line += std::string(1, ' ');
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_IODC, 18, 2);
            if (version == 2)
                {
                    line += std::string(1, ' ');
                }
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;


            // -------- BROADCAST ORBIT - 7
            line.clear();
            if (version == 2)
                {
                    line += std::string(4, ' ');
                }
            if (version == 3)
                {
                    line += std::string(5, ' ');
                }
            line += Rinex_Printer::doub2for(gps_ephemeris_iter->second.d_TOW, 18, 2);
            line += std::string(1, ' ');
            double curve_fit_interval = 4;

            if (gps_ephemeris_iter->second.satelliteBlock[gps_ephemeris_iter->second.i_satellite_PRN].compare("IIA"))
                {
                    // Block II/IIA (Table 20-XI IS-GPS-200E )
                    if ( (gps_ephemeris_iter->second.d_IODC > 239) && (gps_ephemeris_iter->second.d_IODC < 248) )  curve_fit_interval = 8;
                    if ( ( (gps_ephemeris_iter->second.d_IODC > 247) && (gps_ephemeris_iter->second.d_IODC < 256) ) || (gps_ephemeris_iter->second.d_IODC == 496) ) curve_fit_interval = 14;
                    if ( (gps_ephemeris_iter->second.d_IODC > 496) && (gps_ephemeris_iter->second.d_IODC < 504) ) curve_fit_interval = 26;
                    if ( (gps_ephemeris_iter->second.d_IODC > 503) && (gps_ephemeris_iter->second.d_IODC < 511) )  curve_fit_interval = 50;
                    if ( ( (gps_ephemeris_iter->second.d_IODC > 751) && (gps_ephemeris_iter->second.d_IODC < 757) ) || (gps_ephemeris_iter->second.d_IODC == 511) ) curve_fit_interval = 74;
                    if ( gps_ephemeris_iter->second.d_IODC == 757 ) curve_fit_interval = 98;
                }

            if ((gps_ephemeris_iter->second.satelliteBlock[gps_ephemeris_iter->second.i_satellite_PRN].compare("IIR") == 0) ||
                    (gps_ephemeris_iter->second.satelliteBlock[gps_ephemeris_iter->second.i_satellite_PRN].compare("IIR-M") == 0) ||
                    (gps_ephemeris_iter->second.satelliteBlock[gps_ephemeris_iter->second.i_satellite_PRN].compare("IIF") == 0) ||
                    (gps_ephemeris_iter->second.satelliteBlock[gps_ephemeris_iter->second.i_satellite_PRN].compare("IIIA") == 0) )
                {
                    // Block IIR/IIR-M/IIF/IIIA (Table 20-XII IS-GPS-200E )
                    if ( (gps_ephemeris_iter->second.d_IODC > 239) && (gps_ephemeris_iter->second.d_IODC < 248))  curve_fit_interval = 8;
                    if ( ( (gps_ephemeris_iter->second.d_IODC > 247) && (gps_ephemeris_iter->second.d_IODC < 256)) || (gps_ephemeris_iter->second.d_IODC == 496) ) curve_fit_interval = 14;
                    if ( ( (gps_ephemeris_iter->second.d_IODC > 496) && (gps_ephemeris_iter->second.d_IODC < 504)) || ( (gps_ephemeris_iter->second.d_IODC > 1020) && (gps_ephemeris_iter->second.d_IODC < 1024) ) ) curve_fit_interval = 26;
                }
            line += Rinex_Printer::doub2for(curve_fit_interval, 18, 2);
            line += std::string(1, ' ');
            line += std::string(18, ' '); // spare
            line += std::string(1, ' ');
            line += std::string(18, ' '); // spare
            if (version == 2)
                {
                    line += std::string(1, ' ');
                }
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;
            line.clear();
        }
}




void Rinex_Printer::rinex_obs_header(std::ofstream& out, Gps_Ephemeris eph, double d_TOW_first_observation)
{

    std::string line;

    // -------- Line 1
    line = std::string(5, ' ');
    line += stringVersion;
    line += std::string(11, ' ');
    line += Rinex_Printer::leftJustify("OBSERVATION DATA", 20);
    line += satelliteSystem["GPS"];
    line += std::string(19, ' ');
    line += std::string("RINEX VERSION / TYPE");
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line 2
    line.clear();
    if (version == 2)
        {
            line += Rinex_Printer::leftJustify("BLANK OR G = GPS,  R = GLONASS,  E = GALILEO,  M = MIXED", 60);
        }
    if (version == 3)
        {
            line += Rinex_Printer::leftJustify("G = GPS  R = GLONASS  E = GALILEO  S = GEO  M = MIXED", 60);
        }
    line += Rinex_Printer::leftJustify("COMMENT", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line 3
    line.clear();
    line += Rinex_Printer::getLocalTime();
    line += std::string("PGM / RUN BY / DATE");
    line += std::string(1, ' ');
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line COMMENT
    line.clear();
    line += Rinex_Printer::leftJustify("GPS OBSERVATION DATA FILE GENERATED BY GNSS-SDR", 60);
    line += Rinex_Printer::leftJustify("COMMENT", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line COMMENT
    line.clear();
    line += Rinex_Printer::leftJustify("See http://gnss-sdr.org", 60);
    line += Rinex_Printer::leftJustify("COMMENT", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line MARKER NAME
    line.clear();
    line += Rinex_Printer::leftJustify("DEFAULT MARKER NAME", 60); // put a flag or a property,
    line += Rinex_Printer::leftJustify("MARKER NAME", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line MARKER TYPE
    //line.clear();
    //line += Rinex_Printer::leftJustify("GROUND_CRAFT", 20); // put a flag or a property
    //line += std::string(40, ' ');
    //line += Rinex_Printer::leftJustify("MARKER TYPE", 20);
    //Rinex_Printer::lengthCheck(line);
    //out << line << std::endl;

    // -------- Line OBSERVER / AGENCY
    line.clear();
    std::string username=getenv("USER");
    line += leftJustify(username, 20);
    line += Rinex_Printer::leftJustify("CTTC", 40); // add flag and property
    line += Rinex_Printer::leftJustify("OBSERVER / AGENCY", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- Line  REC / TYPE VERS
    line.clear();
    line += Rinex_Printer::leftJustify("GNSS-SDR", 20); // add flag and property
    line += Rinex_Printer::leftJustify("Software Receiver", 20); // add flag and property
    //line += Rinex_Printer::leftJustify(google::VersionString(), 20); // add flag and property
    line += Rinex_Printer::leftJustify("0.1", 20);
    line += Rinex_Printer::leftJustify("REC # / TYPE / VERS", 20);
    lengthCheck(line);
    out << line << std::endl;

    // -------- ANTENNA TYPE
    line.clear();
    line += Rinex_Printer::leftJustify("Antenna number", 20);  // add flag and property
    line += Rinex_Printer::leftJustify("Antenna type", 20);  // add flag and property
    line += std::string(20, ' ');
    line += Rinex_Printer::leftJustify("ANT # / TYPE", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- APPROX POSITION  (optional for moving platforms)
    // put here real data!
    double antena_x = 0.0;
    double antena_y = 0.0;
    double antena_z = 0.0;
    line.clear();
    line += Rinex_Printer::rightJustify(Rinex_Printer::asString(antena_x, 4), 14);
    line += Rinex_Printer::rightJustify(Rinex_Printer::asString(antena_y, 4), 14);
    line += Rinex_Printer::rightJustify(Rinex_Printer::asString(antena_z, 4), 14);
    line += std::string(18, ' ');
    line += Rinex_Printer::leftJustify("APPROX POSITION XYZ", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- ANTENNA: DELTA H/E/N
    // put here real data!
    double antena_h = 0.0;
    double antena_e = 0.0;
    double antena_n = 0.0;
    line.clear();
    line += Rinex_Printer::rightJustify(Rinex_Printer::asString(antena_h, 4), 14);
    line += Rinex_Printer::rightJustify(Rinex_Printer::asString(antena_e, 4), 14);
    line += Rinex_Printer::rightJustify(Rinex_Printer::asString(antena_n, 4), 14);
    line += std::string(18, ' ');
    line += Rinex_Printer::leftJustify("ANTENNA: DELTA H/E/N", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    if (version==2)
        {
            // --------- WAVELENGHT FACTOR
            // put here real data!
            line.clear();
            line +=Rinex_Printer::rightJustify("1",6);
            line +=Rinex_Printer::rightJustify("1",6);
            line += std::string(48, ' ');
            line += Rinex_Printer::leftJustify("WAVELENGTH FACT L1/2", 20);
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;
        }

    if (version==3)
        {
            // -------- SYS / OBS TYPES
            // one line per available system
            line.clear();
            line += satelliteSystem["GPS"];
            line += std::string(2, ' ');
            //int numberTypesObservations=2; // Count the number of available types of observable in the system
            std::stringstream strm;
            strm << numberTypesObservations;
            line += Rinex_Printer::rightJustify(strm.str(), 3);
            // per type of observation
            line += std::string(1, ' ');
            line += observationType["PSEUDORANGE"];
            line += observationCode["GPS_L1_CA"];
            line += std::string(1, ' ');
            line += observationType["SIGNAL_STRENGTH"];
            line += observationCode["GPS_L1_CA"];

            line += std::string(60-line.size(), ' ');
            line += Rinex_Printer::leftJustify("SYS / # / OBS TYPES", 20);
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;
        }

    if (version==2)
        {
            // -------- SYS / OBS TYPES
            line.clear();
            std::stringstream strm;
            strm << numberTypesObservations;
            line += Rinex_Printer::rightJustify(strm.str(), 6);
            // per type of observation
            // GPS L1 PSEUDORANGE
            line += Rinex_Printer::rightJustify(observationType["PSEUDORANGE_CA_v2"], 5);
            line += observationCode["GPS_L1_CA_v2"];
            // GPS L1 PHASE
            line += Rinex_Printer::rightJustify(observationType["CARRIER_PHASE_CA_v2"], 5);
            line += observationCode["GPS_L1_CA_v2"];
            // GPS DOPPLER L1
            line += Rinex_Printer::rightJustify(observationType["DOPPLER_v2"], 5);
            line += observationCode["GPS_L1_CA_v2"];
            // GPS L1 SIGNAL STRENGTH
            line += Rinex_Printer::rightJustify(observationType["SIGNAL_STRENGTH_v2"], 5);
            line += observationCode["GPS_L1_CA_v2"];
            line += std::string(60-line.size(), ' ');
            line += Rinex_Printer::leftJustify("# / TYPES OF OBSERV", 20);
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;
        }

    if (version==3)
        {
            // -------- Signal Strength units
            line.clear();
            line += Rinex_Printer::leftJustify("DBHZ", 20);
            line += std::string(40, ' ');
            line += Rinex_Printer::leftJustify("SIGNAL STRENGTH UNIT", 20);
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;
        }

    // -------- TIME OF FIRST OBS
    line.clear();
    boost::posix_time::ptime p_gps_time = Rinex_Printer::compute_GPS_time(eph,d_TOW_first_observation);
    std::string timestring=boost::posix_time::to_iso_string(p_gps_time);
    std::string year (timestring, 0, 4);
    std::string month (timestring, 4, 2);
    std::string day (timestring, 6, 2);
    std::string hour (timestring, 9, 2);
    std::string minutes (timestring, 11, 2);
    double gps_t = d_TOW_first_observation;
    double seconds = fmod(gps_t, 60);
    line += Rinex_Printer::rightJustify(year, 6);
    line += Rinex_Printer::rightJustify(month, 6);
    line += Rinex_Printer::rightJustify(day, 6);
    line += Rinex_Printer::rightJustify(hour, 6);
    line += Rinex_Printer::rightJustify(minutes, 6);
    line += Rinex_Printer::rightJustify(asString(seconds, 7), 13);
    line += Rinex_Printer::rightJustify(std::string("GPS"), 8);
    line += std::string(9, ' ');
    line += Rinex_Printer::leftJustify("TIME OF FIRST OBS", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;

    // -------- SYS /PHASE SHIFTS

    // -------- end of header
    line.clear();
    line += std::string(60, ' ');
    line += Rinex_Printer::leftJustify("END OF HEADER", 20);
    Rinex_Printer::lengthCheck(line);
    out << line << std::endl;
}




void Rinex_Printer::log_rinex_obs(std::ofstream& out, Gps_Ephemeris eph, double obs_time, std::map<int,Gnss_Synchro> pseudoranges)
{
	// RINEX observations timestamps are GPS timestamps.

    std::string line;

    boost::posix_time::ptime p_gps_time = Rinex_Printer::compute_GPS_time(eph,obs_time);
    std::string timestring = boost::posix_time::to_iso_string(p_gps_time);
    //double utc_t = nav_msg.utc_time(nav_msg.sv_clock_correction(obs_time));
    //double gps_t = eph.sv_clock_correction(obs_time);
    double gps_t = obs_time;

    std::string month (timestring, 4, 2);
    std::string day (timestring, 6, 2);
    std::string hour (timestring, 9, 2);
    std::string minutes (timestring, 11, 2);

    if (version == 2)
        {
            line.clear();
            std::string year (timestring, 2, 2);
            line += std::string(1, ' ');
            line += year;
            line += std::string(1, ' ');
            if (month.compare(0, 1 , "0") == 0)
                {
                    line += std::string(1, ' ');
                    line += month.substr(1, 1);
                }
            else
                {
                    line += month;
                }
            line += std::string(1, ' ');
            if (day.compare(0, 1 , "0") == 0)
                {
                    line += std::string(1, ' ');
                    line += day.substr(1, 1);
                }
            else
                {
                    line += day;
                }
            line += std::string(1, ' ');
            line += hour;
            line += std::string(1, ' ');
            line += minutes;
            line += std::string(1, ' ');
            line += Rinex_Printer::asString(fmod(gps_t, 60), 7);
            line += std::string(2, ' ');
            // Epoch flag 0: OK     1: power failure between previous and current epoch   <1: Special event
            line += std::string(1, '0');
            //Number of satellites observed in current epoch
            int numSatellitesObserved = 0;
            std::map<int,Gnss_Synchro>::iterator pseudoranges_iter;
            for(pseudoranges_iter = pseudoranges.begin();
                    pseudoranges_iter != pseudoranges.end();
                    pseudoranges_iter++)
                {
                    numSatellitesObserved++;
                }
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(numSatellitesObserved), 3);
            for(pseudoranges_iter = pseudoranges.begin();
                    pseudoranges_iter != pseudoranges.end();
                    pseudoranges_iter++)
                {
                    line += satelliteSystem["GPS"];
                    if ((int)pseudoranges_iter->first < 10) line += std::string(1, '0');
                    line += boost::lexical_cast<std::string>((int)pseudoranges_iter->first);
                }
            // Receiver clock offset (optional)
            //line += rightJustify(asString(clockOffset, 12), 15);
            line += std::string(80 - line.size(), ' ');
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;


            for(pseudoranges_iter = pseudoranges.begin();
                    pseudoranges_iter != pseudoranges.end();
                    pseudoranges_iter++)
                {
                    std::string lineObs;
                    lineObs.clear();
                    line.clear();
                    // GPS L1 PSEUDORANGE
                    line += std::string(2, ' ');
                    lineObs += Rinex_Printer::rightJustify(asString(pseudoranges_iter->second.Pseudorange_m, 3), 14);

                    //Loss of lock indicator (LLI)
                    int lli = 0; // Include in the observation!!
                    if (lli == 0)
                        {
                            lineObs += std::string(1, ' ');
                        }
                    else
                        {
                            lineObs += Rinex_Printer::rightJustify(Rinex_Printer::asString<short>(lli), 1);
                        }
                    // GPS L1 CA PHASE
                    lineObs += Rinex_Printer::rightJustify(asString(pseudoranges_iter->second.Carrier_phase_rads/GPS_TWO_PI, 3), 14);
                    // GPS L1 CA DOPPLER
                    lineObs += Rinex_Printer::rightJustify(asString(pseudoranges_iter->second.Carrier_Doppler_hz, 3), 14);
                    //GPS L1 SIGNAL STRENGTH
                    //int ssi=signalStrength(54.0); // The original RINEX 2.11 file stores the RSS in a tabulated format 1-9. However, it is also valid to store the CN0 using dB-Hz units
                    lineObs += Rinex_Printer::rightJustify(asString(pseudoranges_iter->second.CN0_dB_hz, 3), 14);
                    if (lineObs.size() < 80) lineObs += std::string(80 - lineObs.size(), ' ');
                    out << lineObs << std::endl;
                }
        }

    if (version == 3)
        {
            std::string year (timestring, 0, 4);
            line += std::string(1, '>');
            line += std::string(1, ' ');
            line += year;
            line += std::string(1, ' ');
            line += month;
            line += std::string(1, ' ');
            line += day;
            line += std::string(1, ' ');
            line += hour;
            line += std::string(1, ' ');
            line += minutes;

            line += std::string(1, ' ');
            double seconds=fmod(gps_t, 60);
            // Add extra 0 if seconds are < 10
            if (seconds<10)
            {
            	line +=std::string(1, '0');
            }
            line += Rinex_Printer::asString(seconds, 7);
            line += std::string(2, ' ');
            // Epoch flag 0: OK     1: power failure between previous and current epoch   <1: Special event
            line += std::string(1, '0');

            //Number of satellites observed in current epoch
            int numSatellitesObserved = 0;
            std::map<int,Gnss_Synchro>::iterator pseudoranges_iter;
            for(pseudoranges_iter = pseudoranges.begin();
                    pseudoranges_iter != pseudoranges.end();
                    pseudoranges_iter++)
                {
                    numSatellitesObserved++;
                }
            line += Rinex_Printer::rightJustify(boost::lexical_cast<std::string>(numSatellitesObserved), 3);


            // Receiver clock offset (optional)
            //line += rightJustify(asString(clockOffset, 12), 15);

            line += std::string(80 - line.size(), ' ');
            Rinex_Printer::lengthCheck(line);
            out << line << std::endl;

            for(pseudoranges_iter = pseudoranges.begin();
                    pseudoranges_iter != pseudoranges.end();
                    pseudoranges_iter++)
                {
                    std::string lineObs;
                    lineObs.clear();
                    lineObs += satelliteSystem["GPS"];
                    if ((int)pseudoranges_iter->first < 10) lineObs += std::string(1, '0');
                    lineObs += boost::lexical_cast<std::string>((int)pseudoranges_iter->first);
                    //lineObs += std::string(2, ' ');
                    lineObs += Rinex_Printer::rightJustify(asString(pseudoranges_iter->second.Pseudorange_m, 3), 14);

                    //Loss of lock indicator (LLI)
                    int lli = 0; // Include in the observation!!
                    if (lli == 0)
                        {
                            lineObs += std::string(1, ' ');
                        }
                    else
                        {
                            lineObs += Rinex_Printer::rightJustify(Rinex_Printer::asString<short>(lli), 1);
                        }
                    int ssi=signalStrength(54.0); // TODO: include estimated signal strength
                    if (ssi == 0)
                        {
                            lineObs += std::string(1, ' ');
                        }
                    else
                        {
                            lineObs += Rinex_Printer::rightJustify(Rinex_Printer::asString<short>(ssi), 1);
                        }
                    if (lineObs.size() < 80) lineObs += std::string(80 - lineObs.size(), ' ');
                    out << lineObs << std::endl;
                }
        }
}



// represents GPS time (week, TOW) in the date time format of the Gregorian calendar.
// -> Leap years are considered, but leap seconds not.
void Rinex_Printer::to_date_time(int gps_week, int gps_tow, int &year, int &month, int &day, int &hour, int &minute, int &second)
{
    //std::cout << "to_date_time(): gps_week=" << gps_week << " gps_tow=" << gps_tow << std::endl;

    int days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // seconds in a not leap year
    const int secs_per_day = 24*60*60;
    const int secs_per_week = 7*secs_per_day;
    const int secs_per_normal_year = 365*secs_per_day;
    const int secs_per_leap_year = secs_per_normal_year + secs_per_day;

    // the GPS epoch is 06.01.1980 00:00, i.e. midnight 5. / 6. January 1980
    // -> seconds since then
    int secs_since_gps_epoch = gps_week*secs_per_week + gps_tow;

    // find year, consider leap years
    bool is_leap_year;
    int remaining_secs = secs_since_gps_epoch + 5*secs_per_day;
    for (int y = 1980; true ; y++)
        {
            is_leap_year = y%4 == 0 && (y%100 != 0 || y%400 == 0);
            int secs_in_year_y = is_leap_year ? secs_per_leap_year : secs_per_normal_year;

            if (secs_in_year_y <= remaining_secs)
                {
                    remaining_secs -= secs_in_year_y;
                }
            else
                {
                    year = y;
                    //std::cout << "year: year=" << year << " secs_in_year_y="<< secs_in_year_y << " remaining_secs="<< remaining_secs << std::endl;
                    break;
                }
            //std::cout << "year: y=" << y << " secs_in_year_y="<< secs_in_year_y << " remaining_secs="<< remaining_secs << std::endl;
        }

    // find month
    for (int m = 1; true ; m++)
        {
            int secs_in_month_m = days_per_month[m-1]*secs_per_day;
            if (is_leap_year && m == 2 ) // consider February of leap year
                {
                    secs_in_month_m += secs_per_day;
                }

            if (secs_in_month_m <= remaining_secs)
                {
                    remaining_secs -= secs_in_month_m;
                }
            else
                {
                    month = m;
                    //std::cout << "month: month=" << month << " secs_in_month_m="<< secs_in_month_m << " remaining_secs="<< remaining_secs << std::endl;
                    break;
                }
            //std::cout << "month: m=" << m << " secs_in_month_m="<< secs_in_month_m << " remaining_secs="<< remaining_secs << std::endl;
        }

    day = remaining_secs/secs_per_day+1;
    remaining_secs = remaining_secs%secs_per_day;

    hour = remaining_secs/(60*60);
    remaining_secs = remaining_secs%(60*60);

    minute = remaining_secs/60;
    second = remaining_secs%60;
}



void Rinex_Printer::log_rinex_sbs(std::ofstream& out, Sbas_Raw_Msg sbs_message)
{
    // line 1: PRN / EPOCH / RCVR
    std::stringstream line1;

    // SBAS PRN
    line1 << sbs_message.get_prn();
    line1 << " ";

    // gps time of reception
    int gps_week;
    double gps_sec;
    if(sbs_message.get_rx_time_obj().get_gps_time(gps_week, gps_sec))
    {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;

        double gps_sec_one_digit_precicion = round(gps_sec *10)/10; // to prevent rounding towards 60.0sec in the stream output
        int gps_tow = trunc(gps_sec_one_digit_precicion);
        double sub_sec = gps_sec_one_digit_precicion - double(gps_tow);

        to_date_time(gps_week, gps_tow, year, month, day, hour, minute, second);
        line1 << asFixWidthString(year, 2, '0') <<  " " << asFixWidthString(month, 2, '0') <<  " " << asFixWidthString(day, 2, '0') <<  " " << asFixWidthString(hour, 2, '0') <<  " " << asFixWidthString(minute, 2, '0') <<  " " << rightJustify(asString(double(second)+sub_sec,1),4,' ');
    }
    else
    {
        line1 << std::string(19, ' ');
    }
    line1 << "  ";

    // band
    line1 << "L1";
    line1 << "   ";

    // Length of data message (bytes)
    line1 <<  asFixWidthString(sbs_message.get_msg().size(), 3, ' ');
    line1 << "   ";
    // File-internal receiver index
    line1 << "  0";
    line1 << "   ";
    // Transmission System Identifier
    line1 << "SBA";
    line1 << std::string(35, ' ');
    lengthCheck(line1.str());
    out << line1.str() << std::endl;

    // DATA RECORD - 1
    std::stringstream line2;
    line2 << " ";
    // Message frame identifier
    if (sbs_message.get_msg_type() < 10) line2 << " ";
    line2 << sbs_message.get_msg_type();
    line2 << std::string(4, ' ');
    // First 18 bytes of message (hex)
    std::vector<unsigned char> msg = sbs_message.get_msg();
    for (size_t i = 0; i < 18 && i <  msg.size(); ++i)
    {
        line2 << std::hex << std::setfill('0') << std::setw(2);
        line2 << int(msg[i]) << " ";
    }
    line2 << std::string(19, ' ');
    lengthCheck(line2.str());
    out << line2.str() << std::endl;

    // DATA RECORD - 2
    std::stringstream line3;
    line3 << std::string(7, ' ');
    // Remaining bytes of message (hex)
    for (size_t i = 18; i < 36 && i <  msg.size(); ++i)
    {
        line3 << std::hex << std::setfill('0') << std::setw(2);
        line3 << int(msg[i]) << " ";
    }
    line3 << std::string(31, ' ');
    lengthCheck(line3.str());
    out << line3.str() << std::endl;
}




int Rinex_Printer::signalStrength(double snr)
{
    int ss;
    ss = int ( std::min( std::max( int (floor(snr/6)) , 1), 9) );
    return ss;
}


boost::posix_time::ptime Rinex_Printer::compute_UTC_time(Gps_Navigation_Message nav_msg)
{
    // if we are processing a file -> wait to leap second to resolve the ambiguity else take the week from the local system time
    //: idea resolve the ambiguity with the leap second  http://www.colorado.edu/geography/gcraft/notes/gps/gpseow.htm
    double utc_t = nav_msg.utc_time(nav_msg.d_TOW);
    boost::posix_time::time_duration t = boost::posix_time::millisec((utc_t + 604800*(double)(nav_msg.i_GPS_week))*1000);
    boost::posix_time::ptime p_time(boost::gregorian::date(1999, 8, 22), t);
    return p_time;
}

boost::posix_time::ptime Rinex_Printer::compute_GPS_time(Gps_Ephemeris eph, double obs_time)
{
    // The RINEX v2.11 v3.00 format uses GPS time for the observations epoch, not UTC time, thus, no leap seconds needed here.
    // (see Section 3 in http://igscb.jpl.nasa.gov/igscb/data/format/rinex211.txt)
    // (see Pag. 17 in http://igscb.jpl.nasa.gov/igscb/data/format/rinex300.pdf)
    // --??? No time correction here, since it will be done in the RINEX processor
    double gps_t = obs_time;
    boost::posix_time::time_duration t = boost::posix_time::millisec((gps_t + 604800*(double)(eph.i_GPS_week%1024))*1000);
    boost::posix_time::ptime p_time(boost::gregorian::date(1999, 8, 22), t);
    return p_time;
}

/*

enum RINEX_enumMarkerType {
    GEODETIC,      //!< GEODETIC Earth-fixed, high-precision monumentation
    NON_GEODETIC,  //!< NON_GEODETIC Earth-fixed, low-precision monumentation
    SPACEBORNE,    //!< SPACEBORNE Orbiting space vehicle
    AIRBORNE ,     //!< AIRBORNE Aircraft, balloon, etc.
    WATER_CRAFT,   //!< WATER_CRAFT Mobile water craft
    GROUND_CRAFT,  //!< GROUND_CRAFT Mobile terrestrial vehicle
    FIXED_BUOY,    //!< FIXED_BUOY "Fixed" on water surface
    FLOATING_BUOY, //!< FLOATING_BUOY Floating on water surface
    FLOATING_ICE,  //!< FLOATING_ICE Floating ice sheet, etc.
    GLACIER,       //!< GLACIER "Fixed" on a glacier
    BALLISTIC,     //!< BALLISTIC Rockets, shells, etc
    ANIMAL,        //!< ANIMAL Animal carrying a receiver
    HUMAN          //!< HUMAN Human being
};

 */

