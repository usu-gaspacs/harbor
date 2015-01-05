/** @file hgps.c
    @brief
    Convert Harbor GPS data into simple column format.  Perform averaging if
    requested.

    @details
    The hgps program converts a Harbor GPS data file in CVS format to
    columns of data (space-separated).  Records may be filtered by the
    number of satellites in view, and averaged over a given number of
    seconds.
    @verbatim
    Compile: gcc -Wall -o hgps hgps.c
    Use: hgps input.csv avg_secs min_sats > output.txt
    @endverbatim
    @arg @c input.csv is the Harbor GPS CSV data file to process
    @arg @c avg_secs is the number of seconds for averaging; zero for no
    averaging
    @arg @c min_sats is the minimum number of satellites for a valid record

    Any record that does not begin with a digit 0-9 is written as a comment
    starting with '#'.  Records with less than min_sats are also written as
    comments.  While 4 satellites are needed for a 4-D fix, positions may be
    obtained with fewer satellites once the GPS obtains lock.  Records with 0
    satellites may be assumed to be invalid.

    @author Don Rice
    @date 2014-Nov-15 Added doxygen markup.
    @verbatim
    History:
    2014-Nov-14 Initial version
    @endverbatim
*/
/**
   Code modification date
*/
#define CODE_MOD_DATE "Mod_Date:2014-Nov-15"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

/**
   GPS record data structure.  All times are UT.
*/
typedef struct
{
  int mday;      ///< Day of month, 1-31
  int month;     ///< Month, 1-12
  int year;      ///< Year, >2000
  int hour;      ///< Hour, 0-23
  int minute;    ///< Minute, 0-59
  int second;    ///< Second, 0-59
  int todaySecs; ///< Seconds since midnight, 0-86399
  int nsats;     ///< Number of GPS satellites in view
  float tsecs;   ///< Seconds since start of dataset
  float lat;     ///< Latitude, deg north
  float lon;     ///< Longitude, deg east
  float alt;     ///< Altitude, m above MSL
} GPSDATA;

void showAverage( int navg, GPSDATA *raw, GPSDATA *avg );
int updateAverage( int navg, GPSDATA *raw, GPSDATA *avg );

int main( int argc, char **argv )
{
  int minSats, navg;
  float avgSecs, t0;
  char str[256];
  FILE *in;
  GPSDATA raw, avg;

  if( argc != 4 )
    {
      fprintf( stderr, "Use: %s input.csv avg_secs min_sats > output.txt\n",
	       argv[0] );
      fprintf( stderr, "[%s]\n", CODE_MOD_DATE );
      exit(EXIT_FAILURE);
    }
  /* Open input file, CSV format */
  if( (in = fopen( argv[1], "r" )) == NULL )
    {
      perror( argv[1] );
      exit(EXIT_FAILURE);
    }
  avgSecs = atof( argv[2] );
  minSats = atoi( argv[3] );
  printf( "# %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3] );

  /* Read through input file; process lines that start with a digit. */
  t0 = -1.0;
  navg = 0;
  while( fgets( str, 256, in ) )
    {
      if( !isdigit( *str ) ) /* Not a data record */
	{
	  printf( "# %s", str );
	  continue;
	}
      if( sscanf( str, "%f,%d/%d/%d,%d:%d:%d,%f,%f,%f,%d", &raw.tsecs,
		  &raw.mday, &raw.month, &raw.year, &raw.hour, &raw.minute,
		  &raw.second, &raw.lat, &raw.lon, &raw.alt,
		  &raw.nsats ) != 11 )
	{ /* Insufficient data, treat at somment */
	  printf( "# %s", str );
	  continue;
	}
      if( raw.nsats < minSats )
	{ /* No GPS lock, ignore data */
	  printf( "# %s", str );
	  continue;
	}
      raw.year += 2000; /* Convert to full year */
      /* Write out as fixed length columns */
      if( avgSecs <= 0.0 ) /* No averaging */
	printf( "%6.1f %2d %2d %4d %2d %2d %2d %11.7f %12.7f %8.1f %2d\n",
		raw.tsecs, raw.mday, raw.month, raw.year, raw.hour, raw.minute,
		raw.second, raw.lat, raw.lon, raw.alt, raw.nsats );
      else
	{ /* Average measurements for given number of seconds */
	  if( raw.tsecs-t0 > avgSecs || t0 < 0.0 )
	    { /* Compute and display average for last period */
	      showAverage( navg, &raw, &avg );
	      navg = updateAverage( 0, &raw, &avg );
	      t0 = raw.tsecs;
	    }
	  else /* Accumulate data for next average */
	    navg = updateAverage( navg, &raw, &avg );
	}
    }
  showAverage( navg, &raw, &avg );
  fclose( in );
  exit(EXIT_SUCCESS);
} /* main */

/**
   If one or more data points are available, calculate and display
   averages to stdout.
   @param[in] navg Number of points in average.
   @param[in] raw Last data record, used for time information.
   @param[in,out] avg Sums to be converted to averages for display.
*/
void showAverage( int navg, GPSDATA *raw, GPSDATA *avg )
{
  int hh, mm, ss, md, mon, yr;

  if( navg < 1 ) return; /* Nothing to do */

  avg->tsecs /= (float) navg;
  avg->lat /= (float) navg;
  avg->lon /= (float) navg;
  avg->alt /= (float) navg;
  avg->todaySecs /= navg;
  avg->nsats /= navg;
  hh = avg->todaySecs/3600;
  mm = (avg->todaySecs-hh*3600)/60;
  ss = avg->todaySecs%60;
  if( hh >= 24 )
    { /* Use date from latest record */
      hh -= 24;
      yr = raw->year;
      mon = raw->month;
      md = raw->mday; /* could have a weird day at end of month */
    }
  else
    { /* Use date from first record in avg */
      yr = avg->year;
      mon = avg->month;
      md = avg->mday;
    }
  printf( "%6.1f %2d %2d %4d %2d %2d %2d %11.7f %12.7f %8.1f %2d %3d\n",
	  avg->tsecs, md, mon, yr, hh, mm, ss, avg->lat, avg->lon, avg->alt,
	  avg->nsats, navg );
} /* showAverage */

/**
   Adds values from raw to avg.
   @param[in] navg Number of points in current average.
   @param[in] raw Raw data values.
   @param[in,out] avg Sums to be converted to averages by showAverage().
   @return navg+1.
*/
int updateAverage( int navg, GPSDATA *raw, GPSDATA *avg )
{
  if( navg < 1 )
    { /* First data for new average */
      avg->tsecs = raw->tsecs;
      avg->mday = raw->mday;    /* Save time of first data point */
      avg->month = raw->month;
      avg->year = raw->year;
      avg->hour = raw->hour;
      avg->minute = raw->minute;
      avg->second = raw->second;
      avg->todaySecs = raw->hour*3600+raw->minute*60+raw->second;
      avg->lat = raw->lat;
      avg->lon = raw->lon;
      avg->alt = raw->alt;
      avg->nsats = raw->nsats;
      navg = 0;
    }
  else
    { /* Additional data for average */
      int hh;

      avg->tsecs += raw->tsecs;
      if( raw->hour < avg->hour ) hh = raw->hour+24; /* rollover */
      else hh = raw->hour;
      avg->todaySecs += hh*3600+raw->minute*60+raw->second;
      avg->lat += raw->lat;
      avg->lon += raw->lon;
      avg->alt += raw->alt;
      avg->nsats += raw->nsats;
    }
  return navg+1;
} /* updateAverage */
