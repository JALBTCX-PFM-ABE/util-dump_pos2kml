
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.

*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>


/* Local Includes. */

#include "FilePOSOutput.h"

#include "nvutility.h"

#include "version.h"

#include "unistd.h"

#ifdef NVWIN3X
  #include <windows.h>
#endif

void usage ()
{
  fprintf (stderr, "\nUsage: dump_pos2kml -n START_RECORD_NUMBER [-t] POS_OR_SBET_FILE_NAME\n");
  fprintf (stderr, "Dumps the pos or SBET file to stderr.\n\n");
  fprintf (stderr, "\tWhere -n starts dumping at the requested record number and\n");
  fprintf (stderr, "\t-t dumps only the start and end times.\n\n");
  fflush (stderr);
}


/********************************************************************
 *
 * Module Name : main.c
 *
 * Author/Date : Chris Macon 18 July 2009
 *
 * Description : Dumps the sbet file supplied and writes out 2 kml files
 *               every second. 1 kml file is for an overhead view and 
 *		 the 2nd is for a pilot perspective view.
 *
 *		This program is an adaptation of dump_pos (thanks, Jan)
 *
 ********************************************************************/

int32_t main (int32_t argc, char **argv)
{
  char               pos_file[512];
  FILE               *pfp = NULL, *kml_ovw, *kml_pov;
  POS_OUTPUT_T       pos;
  char               c, ovw_name[512], pov_name[512];
  extern char        *optarg;
  extern int         optind;
  int32_t            year, month, mday, jday, hour, minute, recnum = -1;
  float              second, heading, roll, tilt;
  double	     lon_degs, lat_degs;
  int64_t            start_timestamp, end_timestamp;
  uint8_t            time_only = NVFalse;
  

  printf ("\n\n%s\n\n", VERSION);
  fflush (stdout);

  int32_t option_index = 0;
  while (NVTrue) 
    {
      static struct option long_options[] = {{0, no_argument, 0, 0}};

      c = (char) getopt_long (argc, argv, "tn:", long_options, &option_index);
      if (c == -1) break;

      switch (c) 
        {
        case 0:

          switch (option_index)
            {
            case 0:
              break;
            }
          break;

        case 'n':
          sscanf (optarg, "%d", &recnum);
          break;

        case 't':
          time_only = NVTrue;
          break;

        default:
          usage ();
          exit (-1);
          break;
        }
    }


  /* Make sure we got the mandatory file name.  */
  
  if (optind >= argc)
    {
      usage ();
      exit (-1);
    }

  strcpy (pos_file, argv[optind]);

  if ((pfp = open_pos_file (pos_file)) == NULL)
    {
      perror (pos_file);
      exit (-1);
    }

  strcpy (ovw_name, "overview.kml");
  strcpy (pov_name, "perspective.kml");
  
  if ((kml_ovw = fopen (ovw_name, "w")) == NULL)
	{
	fprintf (stderr, "Error opening overview kml file %s\n", ovw_name);
	fprintf (stderr, "%s\n", strerror (errno));
	exit (-1);
	}
  fclose (kml_ovw);

  if ((kml_pov = fopen (pov_name, "w")) == NULL)
	{
	fprintf (stderr, "Error opening perspective kml file %s\n", pov_name);
	fprintf (stderr, "%s\n", strerror (errno));
	exit (-1);
	}
  fclose (kml_pov);

  start_timestamp = pos_get_start_timestamp ();
  end_timestamp = pos_get_end_timestamp ();

  charts_cvtime (start_timestamp, &year, &jday, &hour, &minute, &second);
  year += 1900;
  jday2mday (year, jday, &month, &mday);
  /*fprintf (stderr, "\n\nStart time = %4d/%02d/%02d (%03d) %02d:%02d:%09.6f\n", year, month + 1, 
    mday, jday, hour, minute, second);*/

  charts_cvtime (end_timestamp, &year, &jday, &hour, &minute, &second);
  year += 1900;
  jday2mday (year, jday, &month, &mday);
  /*fprintf (stderr, "End time   = %4d/%02d/%02d (%03d) %02d:%02d:%09.6f\n\n\n", year, month + 1, 
           mday, jday, hour, minute, second);
           fflush (stderr);*/
	
  if (!time_only)
    {
       {
          while (!pos_read_record (pfp, &pos))
            {
		pos_read_record_num (pfp, &pos, recnum);
              	pos_dump_record (pos);

		lat_degs = pos.latitude * NV_RAD_TO_DEG;
		lon_degs = pos.longitude * NV_RAD_TO_DEG;
		heading = ((pos.platform_heading - pos.wander_angle) * NV_RAD_TO_DEG);
		roll = (pos.roll * NV_RAD_TO_DEG) * (-1.0);
		tilt = (pos.pitch * NV_RAD_TO_DEG) + 90.0;

		kml_ovw = fopen (ovw_name, "w");
		kml_pov = fopen (pov_name, "w");

		/*Write out the ovw kml file*/
		fprintf (kml_ovw, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		fprintf (kml_ovw, "<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\" xmlns:kml=\"http://www.opengis.net/kml/2.2\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n");
		fprintf (kml_ovw, "<Document>\n");
		fprintf (kml_ovw, "	<Camera>\n");
		fprintf (kml_ovw, "		<longitude>%0.9lf</longitude>\n", lon_degs);
		fprintf (kml_ovw, "		<latitude>%0.9lf</latitude>\n", lat_degs);
		fprintf (kml_ovw, "		<altitude>%f</altitude>\n", pos.altitude+10000);
		fprintf (kml_ovw, "		<heading>%f</heading>\n" ,heading);
		fprintf (kml_ovw, "		<tilt>0</tilt>\n");
		fprintf (kml_ovw, "		<altitudeMode>absolute</altitudeMode>\n");
		fprintf (kml_ovw, "	</Camera>\n");
		fprintf (kml_ovw, "	<Style id=\"sn_airports\">\n");
		fprintf (kml_ovw, "		<IconStyle>\n");
		fprintf (kml_ovw, "			<scale>1.4</scale>\n");
		fprintf (kml_ovw, "			<heading>%f</heading>\n", heading);
		fprintf (kml_ovw, "			<Icon>\n");
		fprintf (kml_ovw, "				<href>http://maps.google.com/mapfiles/kml/shapes/airports.png</href>\n");
		fprintf (kml_ovw, "			</Icon>\n");
		fprintf (kml_ovw, "			<hotSpot x=\"0.5\" y=\"0\" xunits=\"fraction\" yunits=\"fraction\"/>\n");
		fprintf (kml_ovw, "			</IconStyle>\n");
		fprintf (kml_ovw, "		<ListStyle>\n");
		fprintf (kml_ovw, "		</ListStyle>\n");
		fprintf (kml_ovw, "	</Style>\n");
		fprintf (kml_ovw, "	<StyleMap id=\"msn_airports\">\n");
		fprintf (kml_ovw, "		<Pair>\n");
		fprintf (kml_ovw, "			<key>normal</key>\n");
		fprintf (kml_ovw, "				<styleUrl>#sn_airports</styleUrl>\n");
		fprintf (kml_ovw, "		</Pair>\n");
		fprintf (kml_ovw, "		<Pair>\n");
		fprintf (kml_ovw, "			<key>highlight</key>\n");
		fprintf (kml_ovw, "			<styleUrl>#sh_airports</styleUrl>\n");
		fprintf (kml_ovw, "		</Pair>\n");
		fprintf (kml_ovw, "	</StyleMap>\n");
		fprintf (kml_ovw, "	<Placemark>\n");
		fprintf (kml_ovw, "		<name>CFBCN</name>\n");
		fprintf (kml_ovw, "			<styleUrl>#msn_airports</styleUrl>\n");
		fprintf (kml_ovw, "			<Point>\n");
		fprintf (kml_ovw, "				<altitudeMode>clamptoground</altitudeMode>\n");
		fprintf (kml_ovw, "				<coordinates>%0.9lf,%0.9lf</coordinates>\n",lon_degs,lat_degs);
		fprintf (kml_ovw, "			</Point>\n");
		fprintf (kml_ovw, "	</Placemark>\n");
		fprintf (kml_ovw, "</Document>\n");
		fprintf (kml_ovw, "</kml>\n");

		/*write out the pov kml file*/
		fprintf (kml_pov, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		fprintf (kml_pov, "<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\" xmlns:kml=\"http://www.opengis.net/kml/2.2\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n");
		fprintf (kml_pov, "<Document>\n");
		fprintf (kml_pov, "	<Camera>\n");
		fprintf (kml_pov, "		<longitude>%0.9lf</longitude>\n", lon_degs);
		fprintf (kml_pov, "		<latitude>%0.9lf</latitude>\n", lat_degs);
		fprintf (kml_pov, "		<altitude>%f</altitude>\n", pos.altitude);
		fprintf (kml_pov, "		<roll>%f</roll>\n",roll);
		fprintf (kml_pov, "		<tilt>%f</tilt>\n", tilt);
		fprintf (kml_pov, "		<heading>%f</heading>\n" ,heading);
		fprintf (kml_pov, "		<altitudeMode>absolute</altitudeMode>\n");
		fprintf (kml_pov, "	</Camera>\n");
		fprintf (kml_pov, "</Document>\n");
		fprintf (kml_pov, "</kml>\n");

		/*Print information to screen for testing*/
		/*fprintf (stderr, "lat:%0.9lf\nlon:%0.9lf\naltitude:%f\nroll:%f\ntilt:%f\nheading:%f\nRecord: %i\n\n", lat_degs, lon_degs, pos.altitude, roll, tilt, heading, recnum);*/

		fclose (kml_ovw);
		fclose (kml_pov);

		/*wait for 1 second before continuing*/

#ifdef NVWIN3X
                Sleep (1000);
#else
                sleep (1);
#endif

		recnum = recnum + 200;
            }
        }
    }


  fclose (pfp);


  return (0);
}
