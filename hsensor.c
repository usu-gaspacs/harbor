/** @file hsensor.c
    @brief
    Convert Harbor sensor data into simple column format.  Perform averaging if
    requested.

    @details
    The hsensor program converts a Harbor sensor data file in CVS format to
    columns of data (space-separated).  Records may be averaged over a given
    number of seconds.
    @verbatim
    Compile: gcc -Wall -o hsensor hsensor.c
    Use: hsensor input.csv avg_secs > output.txt
    @endverbatim
    @arg @c input.csv is the Harbor sensor CSV data file to process
    @arg @c avg_secs is the number of seconds for averaging; zero for no
    averaging

    Any record that does not begin with a digit 0-9 is written as a comment
    starting with '#'.

    @author Don Rice
    @date 2014-Nov-15 Initial version
    @verbatim
    History:
    2014-Nov-15 Initial version
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
   Sensor record data structure.  All times are UT.  Note that at this
   point most of the units are unknown and the values are assumed to be
   ADC counts.  Conversion factors need to be added.
*/
typedef struct
{
  float tsecs;            ///< Seconds since start of dataset
  float tmpi;             ///< Internal temperature
  float a1x, a1y, a1z;    ///< 3D Acceleration sensor 1
  float a2x, a2y, a2z;    ///< 3D Acceleration sensor 2
  float magx, magy, magz; ///< 3D Magnetometer
  float gyrx, gyry, gyrz; ///< 3D Gyroscope
  float humid;            ///< Humidity
  float prss;             ///< Pressure
  float tmpx;             ///< External pressure
  float vbat;             ///< Battery voltage
} SENSORDATA;

void showAverage( int navg, SENSORDATA *avg );
int updateAverage( int navg, SENSORDATA *raw, SENSORDATA *avg );

int main( int argc, char **argv )
{
  int navg;
  float avgSecs, t0;
  char str[256];
  FILE *in;
  SENSORDATA raw, avg;

  if( argc != 3 )
    {
      fprintf( stderr, "Use: %s input.csv avg_secs > output.txt\n", argv[0] );
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
  printf( "# %s %s %s\n", argv[0], argv[1], argv[2] );

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
      if( sscanf( str, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
		  &raw.tsecs, &raw.tmpi, &raw.a1x, &raw.a1y, &raw.a1z,
		  &raw.a2x, &raw.a2y, &raw.a2z, &raw.magx, &raw.magy,
		  &raw.magz, &raw.gyrx, &raw.gyry, &raw.gyrz, &raw.humid,
		  &raw.prss, &raw.tmpx, &raw.vbat ) != 18 )
	{ /* Insufficient data, treat at somment */
	  printf( "# %s", str );
	  continue;
	}
      /* Write out as fixed length columns */
      if( avgSecs <= 0.0 ) /* No averaging */
	printf( "%6.1f %5.1f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
		raw.tsecs, raw.tmpi, raw.a1x, raw.a1y, raw.a1z,
		raw.a2x, raw.a2y, raw.a2z, raw.magx, raw.magy,
		raw.magz, raw.gyrx, raw.gyry, raw.gyrz, raw.humid,
		raw.prss, raw.tmpx, raw.vbat );
      else
	{ /* Average measurements for given number of seconds */
	  if( raw.tsecs-t0 > avgSecs || t0 < 0.0 )
	    { /* Compute and display average for last period */
	      showAverage( navg, &avg );
	      navg = updateAverage( 0, &raw, &avg );
	      t0 = raw.tsecs;
	    }
	  else /* Accumulate data for next average */
	    navg = updateAverage( navg, &raw, &avg );
	}
    }
  showAverage( navg, &avg );
  fclose( in );
  exit(EXIT_SUCCESS);
} /* main */

/**
   If one or more data points are available, calculate and display
   averages to stdout.
   @param[in] navg Number of points in average.
   @param[in,out] avg Sums to be converted to averages for display.
*/
void showAverage( int navg, SENSORDATA *avg )
{
  if( navg < 1 ) return; /* Nothing to do */

  avg->tsecs /= (float) navg;
  avg->tmpi /= (float) navg;
  avg->a1x /= (float) navg;
  avg->a1y /= (float) navg;
  avg->a1z /= (float) navg;
  avg->a2x /= (float) navg;
  avg->a2y /= (float) navg;
  avg->a2z /= (float) navg;
  avg->magx /= (float) navg;
  avg->magy /= (float) navg;
  avg->magz /= (float) navg;
  avg->gyrx /= (float) navg;
  avg->gyry /= (float) navg;
  avg->gyrz /= (float) navg;
  avg->humid /= (float) navg;
  avg->prss /= (float) navg;
  avg->tmpx /= (float) navg;
  avg->vbat /= (float) navg;
  printf( "%6.1f %5.1f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
	  avg->tsecs, avg->tmpi, avg->a1x, avg->a1y, avg->a1z,
	  avg->a2x, avg->a2y, avg->a2z, avg->magx, avg->magy,
	  avg->magz, avg->gyrx, avg->gyry, avg->gyrz, avg->humid,
	  avg->prss, avg->tmpx, avg->vbat );
} /* showAverage */

/**
   Adds values from raw to avg.
   @param[in] navg Number of points in current average.
   @param[in] raw Raw data values.
   @param[in,out] avg Sums to be converted to averages by showAverage().
   @return navg+1.
*/
int updateAverage( int navg, SENSORDATA *raw, SENSORDATA *avg )
{
  if( navg < 1 )
    { /* First data for new average */
      avg->tsecs = raw->tsecs;
      avg->tmpi = raw->tmpi;
      avg->a1x = raw->a1x;
      avg->a1y = raw->a1y;
      avg->a1z = raw->a1z;
      avg->a2x = raw->a2x;
      avg->a2y = raw->a2y;
      avg->a2z = raw->a2z;
      avg->magx = raw->magx;
      avg->magy = raw->magy;
      avg->magz = raw->magz;
      avg->gyrx = raw->gyrx;
      avg->gyry = raw->gyry;
      avg->gyrz = raw->gyrz;
      avg->humid = raw->humid;
      avg->prss = raw->prss;
      avg->tmpx = raw->tmpx;
      avg->vbat = raw->vbat;
      navg = 0;
    }
  else
    { /* Additional data for average */
      avg->tsecs += raw->tsecs;
      avg->tmpi += raw->tmpi;
      avg->a1x += raw->a1x;
      avg->a1y += raw->a1y;
      avg->a1z += raw->a1z;
      avg->a2x += raw->a2x;
      avg->a2y += raw->a2y;
      avg->a2z += raw->a2z;
      avg->magx += raw->magx;
      avg->magy += raw->magy;
      avg->magz += raw->magz;
      avg->gyrx += raw->gyrx;
      avg->gyry += raw->gyry;
      avg->gyrz += raw->gyrz;
      avg->humid += raw->humid;
      avg->prss += raw->prss;
      avg->tmpx += raw->tmpx;
      avg->vbat += raw->vbat;
    }
  return navg+1;
} /* updateAverage */
