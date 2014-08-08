/*****************************************************************************
FILE: convert_hdf_to_old_hdf.c
  
PURPOSE: Contains functions for creating the old HDF style data files,
using the existing XML metadata, HDF files and external SDSs.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
8/6/2014     Gail Schmidt     Original development

NOTES:
  1. The XML metadata format written via this library follows the ESPA internal
     metadata format found in ESPA Raw Binary Format v1.0.doc.  The schema for
     the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/

#include <unistd.h>
#include <math.h>
#include "HE2_config.h"
#include "convert_hdf_to_old_hdf.h"

#define OUTPUT_PROVIDER ("DataProvider")
#define OUTPUT_SAT ("Satellite")
#define OUTPUT_INST ("Instrument")
#define OUTPUT_ACQ_DATE ("AcquisitionDate")
#define OUTPUT_L1_PROD_DATE ("Level1ProductionDate")
#define OUTPUT_LPGS_METADATA ("LPGSMetadataFile")
#define OUTPUT_SUN_ZEN ("SolarZenith")
#define OUTPUT_SUN_AZ ("SolarAzimuth")
#define OUTPUT_WRS_SYS ("WRS_System")
#define OUTPUT_WRS_PATH ("WRS_Path")
#define OUTPUT_WRS_ROW ("WRS_Row")
#define OUTPUT_SHORT_NAME ("ShortName")
#define OUTPUT_LOCAL_GRAN_ID ("LocalGranuleID")
#define OUTPUT_PROD_DATE ("ProductionDate")
#define OUTPUT_REFL_GAINS ("ReflGains")
#define OUTPUT_REFL_BIAS ("ReflBias")
#define OUTPUT_THM_GAINS ("ThermalGains")
#define OUTPUT_THM_BIAS ("ThermalBias")
#define OUTPUT_PAN_GAIN ("PanGain")
#define OUTPUT_PAN_BIAS ("PanBias")

#define OUTPUT_WEST_BOUND  ("WestBoundingCoordinate")
#define OUTPUT_EAST_BOUND  ("EastBoundingCoordinate")
#define OUTPUT_NORTH_BOUND ("NorthBoundingCoordinate")
#define OUTPUT_SOUTH_BOUND ("SouthBoundingCoordinate")
#define UL_LAT_LONG ("UpperLeftCornerLatLong")
#define LR_LAT_LONG ("LowerRightCornerLatLong")
#define OUTPUT_HDFEOS_VERSION ("HDFEOSVersion");
#define OUTPUT_HDF_VERSION ("HDFVersion");

#define OUTPUT_LONG_NAME        ("long_name")
#define OUTPUT_UNITS            ("units")
#define OUTPUT_VALID_RANGE      ("valid_range")
#define OUTPUT_FILL_VALUE       ("_FillValue")
#define OUTPUT_SATU_VALUE       ("_SaturateValue")
#define OUTPUT_SCALE_FACTOR     ("scale_factor")
#define OUTPUT_ADD_OFFSET       ("add_offset")
#define OUTPUT_CALIBRATED_NT    ("calibrated_nt")
#define OUTPUT_APP_VERSION      ("app_version")

/* This identifies the mapping of SDS names that we will use for the new
   HDF file to the old HDF file.  Also identifies the order in which the
   SDSs will be written to the old HDF file format. */
#define NOLD_SR 17
const char hdfname_mapping[NOLD_SR][2][STR_SIZE] = {
   {"sr_band1", "band1"},
   {"sr_band2", "band2"},
   {"sr_band3", "band3"},
   {"sr_band4", "band4"},
   {"sr_band5", "band5"},
   {"sr_band7", "band7"},
   {"sr_atmos_opacity", "atmos_opacity"},
   {"sr_fill_qa", "fill_QA"},
   {"sr_ddv_qa", "DDV_QA"},
   {"sr_cloud_qa", "cloud_QA"},
   {"sr_cloud_shadow_qa", "cloud_shadow_QA"},
   {"sr_snow_qa", "snow_QA"},
   {"sr_land_water_qa", "land_water_QA"},
   {"sr_adjacent_cloud_qa", "adjacent_cloud_QA"},
   {"toa_band6", "band6"},
   {"toa_band6_qa", "band6_fill_QA"},
   {"fmask", "fmask_band"}};

/******************************************************************************
MODULE:  write_global_attributes

PURPOSE: Write the global attributes (metadata) for the HDF file, using the
metadata from the XML file.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error writing the global attributes
SUCCESS         Successfully wrote the global attributes

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/6/2014     Gail Schmidt     Original development
6/17/2014    Gail Schmidt     Updated to support L8

NOTES:
******************************************************************************/
int write_global_attributes
(
    int32 hdf_id,               /* I: HDF file ID to write attributes */
    Espa_internal_meta_t *xml_metadata  /* I: pointer to metadata structure */
)
{
    char FUNC_NAME[] = "write_global_attributes";  /* function name */
    char errmsg[STR_SIZE];        /* error message */
    char hdf_version[] = H4_VERSION;  /* version for HDF4 */
    char hdfeos_version[] = PACKAGE_VERSION;  /* version for HDFEOS */
    int i;                        /* looping variable for each SDS */
    int ngain_bias = 0;           /* number of gain/bias values for refl */
    int ngain_bias_thm = 0;       /* number of gain/bias values for thermal */
    int refl_count = 0;           /* count of reflectance bands */
    int thm_count = 0;            /* count of thermal bands */
    double dval[MAX_TOTAL_BANDS]; /* attribute values to be written */
    double gain[MAX_TOTAL_BANDS]; /* gains to be written to the HDF metadata */
    double bias[MAX_TOTAL_BANDS]; /* biases to be written to the HDF metadata */
    double gain_thm[2]; /* thermal gains to be written to the HDF metadata */
    double bias_thm[2]; /* thermal biases to be written to the HDF metadata */
    Espa_hdf_attr_t attr;         /* attribute fields */
    Espa_global_meta_t *gmeta = &xml_metadata->global;
                                  /* pointer to global metadata structure */

    /* Write the global attributes to the HDF file.  Some are required and
       others are optional.  If the optional fields are not defined, then
       they won't be written. */
    attr.type = DFNT_CHAR8;
    attr.nval = strlen (gmeta->data_provider);
    attr.name = OUTPUT_PROVIDER;
    if (put_attr_string (hdf_id, &attr, gmeta->data_provider) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (data provider)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    attr.type = DFNT_CHAR8;
    attr.nval = strlen (gmeta->satellite);
    attr.name = OUTPUT_SAT;
    if (put_attr_string (hdf_id, &attr, gmeta->satellite) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (satellite)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    attr.type = DFNT_CHAR8;
    attr.nval = strlen (gmeta->instrument);
    attr.name = OUTPUT_INST;
    if (put_attr_string (hdf_id, &attr, gmeta->instrument) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (instrument)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    attr.type = DFNT_CHAR8;
    attr.nval = strlen (gmeta->acquisition_date);
    attr.name = OUTPUT_ACQ_DATE;
    if (put_attr_string (hdf_id, &attr, gmeta->acquisition_date) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (acquisition date)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_CHAR8;
    attr.nval = strlen (gmeta->level1_production_date);
    attr.name = OUTPUT_L1_PROD_DATE;
    if (put_attr_string (hdf_id, &attr, gmeta->level1_production_date)
        != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (production date)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_CHAR8;
    attr.nval = strlen (gmeta->lpgs_metadata_file);
    attr.name = OUTPUT_LPGS_METADATA;
    if (put_attr_string (hdf_id, &attr, gmeta->lpgs_metadata_file) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (LPGS metadata file)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_FLOAT32;
    attr.nval = 1;
    attr.name = OUTPUT_SUN_ZEN;
    dval[0] = (double) gmeta->solar_zenith;
    if (put_attr_double(hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (solar zenith)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_FLOAT32;
    attr.nval = 1;
    attr.name = OUTPUT_SUN_AZ;
    dval[0] = (double) gmeta->solar_azimuth;
    if (put_attr_double(hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (solar azimuth)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_INT16;
    attr.nval = 1;
    attr.name = OUTPUT_WRS_SYS;
    dval[0] = (double) gmeta->wrs_system;
    if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (WRS system)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_INT16;
    attr.nval = 1;
    attr.name = OUTPUT_WRS_PATH;
    dval[0] = (double) gmeta->wrs_path;
    if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (WRS path)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_INT16;
    attr.nval = 1;
    attr.name = OUTPUT_WRS_ROW;
    dval[0] = (double) gmeta->wrs_row;
    if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (WRS row)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Set up the reflectance gains and biases, if they are available in
       the XML file.  The gains and biases are written for the reflectance
       bands themselves, in order from b1-b7.  The thermal gains and biases
       are written for band 61 and 62 (if ETM+), and the pan band gains and
       biases are also written. */
    if (!strcmp (gmeta->instrument, "TM"))
    {
        /* Make sure the gain/bias values actually exist.  Use band 1 for this
           check.  If it doesn't exist then assume none exist. */
        if (fabs (xml_metadata->band[0].toa_gain - ESPA_FLOAT_META_FILL) >
            ESPA_EPSILON &&
            fabs (xml_metadata->band[0].toa_bias - ESPA_FLOAT_META_FILL) >
            ESPA_EPSILON)
        {
            /* Loop through the bands and store the gain and bias for the
               reflectance bands */
            refl_count = 0;
            for (i = 0; i < 7; i++)
            {
                if (i != 5 /* band 6 */)
                {   /* populate the reflectance bands */
                    gain[refl_count] = xml_metadata->band[i].toa_gain;
                    bias[refl_count] = xml_metadata->band[i].toa_bias;
                    refl_count++;
                }
                else
                {   /* populate the thermal band */
                    gain_thm[0] = xml_metadata->band[i].toa_gain;
                    bias_thm[0] = xml_metadata->band[i].toa_bias;
                }
            }

            /* Identify the number of values to be written */
            ngain_bias = refl_count;
            ngain_bias_thm = 1;
        }
        else
        {
            ngain_bias = 0;
            ngain_bias_thm = 0;
        }
    }
    else if (!strncmp (gmeta->instrument, "ETM", 3))
    {
        /* Make sure the gain/bias values actually exist.  Use band 1 for this
           check.  If it doesn't exist then assume none exist. */
        if (fabs (xml_metadata->band[0].toa_gain - ESPA_FLOAT_META_FILL) >
            ESPA_EPSILON &&
            fabs (xml_metadata->band[0].toa_bias - ESPA_FLOAT_META_FILL) >
            ESPA_EPSILON)
        {
            /* Loop through the bands and store the gain and bias for the
               reflectance bands */
            refl_count = 0;
            thm_count = 0;
            for (i = 0; i < 8; i++)  /* skip pan for now */
            {
                if (i != 5 /* band 61 */ && i != 6 /* band 62 */)
                {   /* populate the reflectance bands */
                    gain[refl_count] = xml_metadata->band[i].toa_gain;
                    bias[refl_count] = xml_metadata->band[i].toa_bias;
                    refl_count++;
                }
                else
                {   /* populate the thermal band */
                    gain_thm[thm_count] = xml_metadata->band[i].toa_gain;
                    bias_thm[thm_count] = xml_metadata->band[i].toa_bias;
                    thm_count++;
                }
            }

            /* Identify the number of values to be written */
            ngain_bias = refl_count;
            ngain_bias_thm = thm_count;
        }
        else
        {
            ngain_bias = 0;
            ngain_bias_thm = 0;
        }
    }
    else if (!strcmp (gmeta->instrument, "OLI_TIRS"))
    {
        /* Make sure the gain/bias values actually exist.  Use band 1 for this
           check.  If it doesn't exist then assume none exist. */
        if (fabs (xml_metadata->band[0].toa_gain - ESPA_FLOAT_META_FILL) >
            ESPA_EPSILON &&
            fabs (xml_metadata->band[0].toa_bias - ESPA_FLOAT_META_FILL) >
            ESPA_EPSILON)
        {
            /* Loop through the bands and store the gain and bias for the
               reflectance bands */
            refl_count = 0;
            thm_count = 0;
            for (i = 0; i < 11; i++)
            {
                if (i == 9 /* band 10 */ || i == 10 /* band 11 */)
                {   /* populate the thermal band */
                    gain_thm[thm_count] = xml_metadata->band[i].toa_gain;
                    bias_thm[thm_count] = xml_metadata->band[i].toa_bias;
                    thm_count++;
                }
                else if (i != 7 /* skip pan band */)
                {   /* populate the reflectance bands */
                    gain[refl_count] = xml_metadata->band[i].toa_gain;
                    bias[refl_count] = xml_metadata->band[i].toa_bias;
                    refl_count++;
                }
            }

            /* Identify the number of values to be written */
            ngain_bias = refl_count;
            ngain_bias_thm = thm_count;
        }
        else
        {
            ngain_bias = 0;
            ngain_bias_thm = 0;
        }
    }

    /* Write reflectance gain/bias values */
    if (ngain_bias > 0)
    {
        /* Gains */
        attr.type = DFNT_FLOAT64;
        attr.nval = ngain_bias;
        attr.name = OUTPUT_REFL_GAINS;
        for (i = 0; i < ngain_bias; i++)
            dval[i] = (double) gain[i];
        if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing global attribute (reflectance gains)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        /* Biases */
        attr.type = DFNT_FLOAT64;
        attr.nval = ngain_bias;
        attr.name = OUTPUT_REFL_BIAS;
        for (i = 0; i < ngain_bias; i++)
            dval[i] = (double) bias[i];
        if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing global attribute (reflectance biases)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
  
    /* Write thermal gain/bias values */
    if (ngain_bias_thm > 0)
    {
        /* Gains */
        attr.type = DFNT_FLOAT64;
        attr.nval = ngain_bias_thm;
        attr.name = OUTPUT_THM_GAINS;
        for (i = 0; i < ngain_bias_thm; i++)
            dval[i] = (double) gain_thm[i];
        if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing global attribute (thermal gains)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        /* Biases */
        attr.type = DFNT_FLOAT64;
        attr.nval = ngain_bias_thm;
        attr.name = OUTPUT_THM_BIAS;
        for (i = 0; i < ngain_bias_thm; i++)
            dval[i] = (double) bias_thm[i];
        if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing global attribute (thermal biases)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Write pan gain/bias values if this is ETM+ or OLI_TIRS and the
       gains/biases exist in the metadata file */
    if ((!strncmp (gmeta->instrument, "ETM", 3) ||
         !strcmp (gmeta->instrument, "OLI_TIRS")) && ngain_bias > 0)
    {
        /* Gains */
        attr.type = DFNT_FLOAT64;
        attr.nval = 1;
        attr.name = OUTPUT_PAN_GAIN;
        if (!strncmp (gmeta->instrument, "ETM", 3))
            dval[0] = (double) xml_metadata->band[8].toa_gain;
        else if (!strcmp (gmeta->instrument, "OLI_TIRS"))
            dval[0] = (double) xml_metadata->band[7].toa_gain;
        if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing global attribute (pan gains)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        /* Biases */
        attr.type = DFNT_FLOAT64;
        attr.nval = 1;
        attr.name = OUTPUT_PAN_BIAS;
        if (!strncmp (gmeta->instrument, "ETM", 3))
            dval[0] = (double) xml_metadata->band[8].toa_bias;
        else if (!strcmp (gmeta->instrument, "OLI_TIRS"))
            dval[0] = (double) xml_metadata->band[7].toa_bias;
        if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing global attribute (pan biases)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
  
    attr.type = DFNT_FLOAT64;
    attr.nval = 2;
    attr.name = UL_LAT_LONG;
    dval[0] = gmeta->ul_corner[0];
    dval[1] = gmeta->ul_corner[1];
    if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (UL corner)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_FLOAT64;
    attr.nval = 2;
    attr.name = LR_LAT_LONG;
    dval[0] = gmeta->lr_corner[0];
    dval[1] = gmeta->lr_corner[1];
    if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (LR corner)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = OUTPUT_WEST_BOUND;
    dval[0] = gmeta->bounding_coords[ESPA_WEST];
    if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (west bounding coord)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = OUTPUT_EAST_BOUND;
    dval[0] = gmeta->bounding_coords[ESPA_EAST];
    if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (east bounding coord)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = OUTPUT_NORTH_BOUND;
    dval[0] = gmeta->bounding_coords[ESPA_NORTH];
    if (put_attr_double (hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (north bounding coord)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = OUTPUT_SOUTH_BOUND;
    dval[0] = gmeta->bounding_coords[ESPA_SOUTH];
    if (put_attr_double(hdf_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (south bounding coord)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    attr.type = DFNT_CHAR8;
    attr.nval = strlen (hdf_version);
    attr.name = OUTPUT_HDF_VERSION;
    if (put_attr_string (hdf_id, &attr, hdf_version) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (HDF Version)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    attr.type = DFNT_CHAR8;
    attr.nval = strlen (hdfeos_version);
    attr.name = OUTPUT_HDFEOS_VERSION;
    if (put_attr_string (hdf_id, &attr, hdfeos_version) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (HDFEOS Version)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Use the production date from the first band */
    attr.type = DFNT_CHAR8;
    attr.nval = strlen (xml_metadata->band[0].production_date);
    attr.name = OUTPUT_PROD_DATE;
    if (put_attr_string (hdf_id, &attr, xml_metadata->band[0].production_date)
        != SUCCESS)
    {
        sprintf (errmsg, "Writing global attribute (production date)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Successful write */
    return (SUCCESS);
}

/******************************************************************************
MODULE:  write_sds_attributes

PURPOSE: Write the attributes (metadata) for the current SDS, using the
metadata from the current band.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error writing the SDS attributes
SUCCESS         Successfully wrote the SDS attributes

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/6/2014     Gail Schmidt     Original development
3/21/2014    Gail Schmidt     Added the application version for band attributes

NOTES:
******************************************************************************/
int write_sds_attributes
(
    int32 sds_id,             /* I: SDS ID to write attributes */
    Espa_band_meta_t *bmeta   /* I: pointer to band metadata structure */
)
{
    char FUNC_NAME[] = "write_sds_attributes";  /* function name */
    char errmsg[STR_SIZE];      /* error message */
    char tmp_msg[STR_SIZE];     /* temporary message */
    char message[5000];         /* description of QA bits or classes */
    int i;                      /* looping variable for each SDS */
    int count;                  /* number of chars copied in snprintf */
    double dval[MAX_TOTAL_BANDS];/* attribute values to be written */
    Espa_hdf_attr_t attr;       /* attribute fields */

    /* Write the band-related attributes to the SDS.  Some are required and
       others are optional.  If the optional fields are not defined, then
       they won't be written. */
    attr.type = DFNT_CHAR8;
    attr.nval = strlen (bmeta->long_name);
    attr.name = OUTPUT_LONG_NAME;
    if (put_attr_string (sds_id, &attr, bmeta->long_name) != SUCCESS)
    {
        sprintf (errmsg, "Writing attribute (long name) to SDS: %s",
            bmeta->name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    attr.type = DFNT_CHAR8;
    attr.nval = strlen (bmeta->data_units);
    attr.name = OUTPUT_UNITS;
    if (put_attr_string (sds_id, &attr, bmeta->data_units) != SUCCESS)
    {
        sprintf (errmsg, "Writing attribute (units ref) to SDS: %s",
            bmeta->name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (bmeta->valid_range[0] != ESPA_INT_META_FILL &&
        bmeta->valid_range[1] != ESPA_INT_META_FILL)
    {
        attr.type = DFNT_INT32;
        attr.nval = 2;
        attr.name = OUTPUT_VALID_RANGE;
        dval[0] = (double) bmeta->valid_range[0];
        dval[1] = (double) bmeta->valid_range[1];
        if (put_attr_double (sds_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing attribute (valid range) to SDS: %s",
                bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    attr.type = DFNT_INT32;
    attr.nval = 1;
    attr.name = OUTPUT_FILL_VALUE;
    dval[0] = (double) bmeta->fill_value;
    if (put_attr_double (sds_id, &attr, dval) != SUCCESS)
    {
        sprintf (errmsg, "Writing attribute (fill value) to SDS: %s",
            bmeta->name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (bmeta->saturate_value != ESPA_INT_META_FILL)
    {
        attr.type = DFNT_INT32;
        attr.nval = 1;
        attr.name = OUTPUT_SATU_VALUE;
        dval[0] = (double) bmeta->saturate_value;
        if (put_attr_double (sds_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing attribute (saturate value) to SDS: %s",
                bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    if (bmeta->scale_factor != ESPA_INT_META_FILL)
    {
        attr.type = DFNT_FLOAT32;
        attr.nval = 1;
        attr.name = OUTPUT_SCALE_FACTOR;
        dval[0] = (float) bmeta->scale_factor;
        if (put_attr_double (sds_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing attribute (scale factor) to SDS: %s",
                bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

    }

    if (bmeta->add_offset != ESPA_INT_META_FILL)
    {
        attr.type = DFNT_FLOAT64;
        attr.nval = 1;
        attr.name = OUTPUT_ADD_OFFSET;
        dval[0] = (double) bmeta->add_offset;
        if (put_attr_double (sds_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing attribute (add offset) to SDS: %s",
                bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    if (fabs (bmeta->calibrated_nt - ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
    {
        attr.type = DFNT_FLOAT32;
        attr.nval = 1;
        attr.name = OUTPUT_CALIBRATED_NT;
        dval[0] = (double) bmeta->calibrated_nt;
        if (put_attr_double (sds_id, &attr, dval) != SUCCESS)
        {
            sprintf (errmsg, "Writing attribute (calibrated nt) to SDS: %s",
                bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    if (bmeta->nbits != ESPA_INT_META_FILL && bmeta->nbits > 0)
    {
        count = snprintf (message, sizeof (message),
            "\n\tBits are numbered from right to left "
            "(bit 0 = LSB, bit N = MSB):\n"
            "\tBit    Description\n");
        if (count < 0 || count >= sizeof (message))
        {
            sprintf (errmsg, "Overflow of message string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        for (i = 0; i < bmeta->nbits; i++)
        {
            count = snprintf (tmp_msg, sizeof (tmp_msg), "\t%d      %s\n", i,
                bmeta->bitmap_description[i]);
            if (count < 0 || count >= sizeof (tmp_msg))
            {
                sprintf (errmsg, "Overflow of tmp_msg string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (strlen (message) + strlen (tmp_msg) >= sizeof (message))
            {
                sprintf (errmsg, "Overflow of message string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            strcat (message, tmp_msg);
        }

        attr.type = DFNT_CHAR8;
        attr.nval = strlen (message);
        attr.name = "Bitmap description";
        if (put_attr_string (sds_id, &attr, message) != SUCCESS)
        {
            sprintf (errmsg, "Writing attribute (Bitmap description) to SDS: "
                "%s", bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    if (bmeta->nclass != ESPA_INT_META_FILL && bmeta->nclass > 0)
    {
        count = snprintf (message, sizeof (message),
            "\n\tClass  Description\n");
        if (count < 0 || count >= sizeof (message))
        {
            sprintf (errmsg, "Overflow of message string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        for (i = 0; i < bmeta->nclass; i++)
        {
            count = snprintf (tmp_msg, sizeof (tmp_msg), "\t%d      %s\n",
                bmeta->class_values[i].class,
                bmeta->class_values[i].description);
            if (count < 0 || count >= sizeof (tmp_msg))
            {
                sprintf (errmsg, "Overflow of tmp_msg string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (strlen (message) + strlen (tmp_msg) >= sizeof (message))
            {
                sprintf (errmsg, "Overflow of message string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            strcat (message, tmp_msg);
        }

        attr.type = DFNT_CHAR8;
        attr.nval = strlen (message);
        attr.name = "Class description";
        if (put_attr_string (sds_id, &attr, message) != SUCCESS)
        {
            sprintf (errmsg, "Writing attribute (Class description) to SDS: "
                "%s", bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    if (strcmp (bmeta->app_version, ESPA_STRING_META_FILL))
    {
        attr.type = DFNT_CHAR8;
        attr.nval = strlen (bmeta->app_version);
        attr.name = OUTPUT_APP_VERSION;
        if (put_attr_string (sds_id, &attr, bmeta->app_version) != SUCCESS)
        {
            sprintf (errmsg, "Writing attribute (app version) to SDS: %s",
                bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Successful write */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  create_hdf_metadata

PURPOSE: Create the old format HDF metadata file, using info from the XML file
(packaged with the HDF product), which will point to the existing raw binary
bands as external SDSs.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error creating the HDF file
SUCCESS         Successfully created the HDF file

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
8/7/2014     Gail Schmidt     Original development

NOTES:
  1. The ESPA products are 2D thus only 2D products are supported.
  2. XDim, YDim will refer to the x,y dimension size in the first SDS.  From
     there, different x,y dimensions will contain the pixel size at the end of
     XDim, YDim.  Example: XDim_15, YDim_15.  For Geographic projections, the
     name will be based on the count of grids instead of the pixel size.
******************************************************************************/
int create_hdf_metadata
(
    char *hdf_file,                     /* I: output HDF filename */
    Espa_internal_meta_t *xml_metadata  /* I: XML metadata structure */
)
{
    char FUNC_NAME[] = "create_hdf_metadata";  /* function name */
    char errmsg[STR_SIZE];        /* error message */
    char dim_name[2][STR_SIZE];   /* array of dimension names */
    bool bnd_found = false;       /* was the current band found */
    int i;                        /* looping variable for each SDS */
    int bnd;                      /* looping variable for the desired bands */
    int nlines;                   /* number of lines in the band */
    int nsamps;                   /* number of samples in the band */
    int dim;                      /* looping variable for dimensions */
    int32 hdf_id;                 /* HDF file ID */
    int32 sds_id;                 /* ID for each SDS */
    int32 dim_id;                 /* ID for current dimension in SDS */
    int32 data_type;              /* data type for HDF file */
    int32 rank = 2;               /* rank of the SDS; set for 2D products */
    int32 dims[2];                /* array for dimension sizes; only 2D prods */

    /* Open the HDF file for creation (overwriting if it exists) */
    hdf_id = SDstart (hdf_file, DFACC_CREATE);
    if (hdf_id == HDF_ERROR)
    {
        sprintf (errmsg, "Creating the HDF file: %s", hdf_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Loop through the bands in the XML file and set each band as an
       external SDS in this HDF file.  We want to write in a particular order
       however, versus the order of the bands in the XML file. */
    for (bnd = 0; bnd < NOLD_SR; bnd++)
    {
        bnd_found = false;
        for (i = 0; i < xml_metadata->nbands; i++)
        {
            /* If this is the band we are looking for currently, then process
               it.  Otherwise skip. */
            if (strcmp (xml_metadata->band[i].name, hdfname_mapping[bnd][0]))
                continue;

            /* Provide the status of processing */
            bnd_found = true;
            printf ("Processing SDS: %s --> %s\n", xml_metadata->band[i].name,
                hdfname_mapping[bnd][1]);

            /* Define the dimensions for this band */
            nlines = xml_metadata->band[i].nlines;
            nsamps = xml_metadata->band[i].nsamps;
            dims[0] = nlines;
            dims[1] = nsamps;

            /* Determine the HDF data type */
            switch (xml_metadata->band[i].data_type)
            {
                case (ESPA_INT8):
                    data_type = DFNT_INT8;
                    break;
                case (ESPA_UINT8):
                    data_type = DFNT_UINT8;
                    break;
                case (ESPA_INT16):
                    data_type = DFNT_INT16;
                    break;
                case (ESPA_UINT16):
                    data_type = DFNT_UINT16;
                    break;
                case (ESPA_INT32):
                    data_type = DFNT_INT32;
                    break;
                case (ESPA_UINT32):
                    data_type = DFNT_UINT32;
                    break;
                case (ESPA_FLOAT32):
                    data_type = DFNT_FLOAT32;
                    break;
                case (ESPA_FLOAT64):
                    data_type = DFNT_FLOAT64;
                    break;
                default:
                    sprintf (errmsg, "Unsupported ESPA data type.");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
            }

            /* Select/create the SDS index for the current band */
            sds_id = SDcreate (hdf_id, hdfname_mapping[bnd][1], data_type,
                rank, dims);
            if (sds_id == HDF_ERROR)
            {
                sprintf (errmsg, "Creating SDS in the HDF file: %d.", bnd);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Set the dimension name for each dimension in this SDS */
            strcpy (dim_name[0], "YDim");
            strcpy (dim_name[1], "XDim");

            /* Write the dimension names to the HDF file */
            for (dim = 0; dim < rank; dim++)
            {
                dim_id = SDgetdimid (sds_id, dim);
                if (dim_id == HDF_ERROR) 
                {
                    sprintf (errmsg, "Getting dimension id for dimension %d "
                        "and SDS %d.", dim, bnd);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                if (SDsetdimname (dim_id, dim_name[dim]) == HDF_ERROR)
                {
                    sprintf (errmsg, "Setting dimension name (%s) for "
                        "dimension %d and SDS %d.", dim_name[dim], dim, bnd);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }

            /* Identify the external dataset for this SDS, starting at byte
               location 0 since these are raw binary files without any
               headers */
            if (SDsetexternalfile (sds_id, xml_metadata->band[i].file_name,
                0 /* offset */) == HDF_ERROR)
            {
                sprintf (errmsg, "Setting the external dataset for this SDS "
                    "(%d): %s.", bnd, xml_metadata->band[i].file_name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Write the SDS-level metadata */
            if (write_sds_attributes (sds_id, &xml_metadata->band[i]) !=
                SUCCESS)
            {
                sprintf (errmsg, "Writing band attributes for this SDS (%d).",
                    bnd);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Terminate access to the data set and SD interface */
            SDendaccess (sds_id);
        }

        /* If the band wasn't found then produce an error message, unless this
           is the last band (fmask) which is optional. */
        if (!bnd_found && (bnd != NOLD_SR - 1))
        {
            sprintf (errmsg, "Band %s was not found in the XML file, but it "
               "is expected to be available for output.",
               hdfname_mapping[bnd][0]);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Write the global metadata */
    if (write_global_attributes (hdf_id, xml_metadata) != SUCCESS)
    {
        sprintf (errmsg, "Writing global attributes for this HDF file.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Terminate access to the HDF file */
    SDend (hdf_id);

    /* Write HDF-EOS attributes and metadata */
    if (write_hdf_eos_attr_old (hdf_file, xml_metadata) != SUCCESS)
    {
        sprintf (errmsg, "Writing HDF-EOS attributes for this old-style HDF "
            "file.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Successful conversion */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  convert_hdf_to_old_hdf

PURPOSE: Converts the current HDF file format with associated XML file to the
old ESPA HDF file format.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error converting to HDF
SUCCESS         Successfully converted to HDF

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
8/7/2014     Gail Schmidt     Original development

NOTES:
  1. The ESPA raw binary band files will be used, as-is, and linked to as
     external SDSs from the HDF file.
  2. An ENVI header file will be written for the HDF files which contain
     SDSs of the same resolution (i.e. not a multi-resolution product).
******************************************************************************/
int convert_hdf_to_old_hdf
(
    char *espa_xml_file,   /* I: input ESPA XML metadata filename, delivered
                                 with the HDF file */
    char *hdf_file         /* I: output HDF filename */
)
{
    char FUNC_NAME[] = "convert_hdf_to_old_hdf";  /* function name */
    char errmsg[STR_SIZE];   /* error message */
    char hdr_file[STR_SIZE]; /* ENVI header file */
    int i;                   /* looping variable */
    int indx;                /* band index for sr_band1 */
    int count;               /* number of chars copied in snprintf */
    Espa_internal_meta_t xml_metadata;  /* XML metadata structure to be
                                populated by reading the MTL metadata file */
    Envi_header_t envi_hdr;  /* output ENVI header information */

    /* Validate the input metadata file */
    if (validate_xml_file (espa_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Initialize the metadata structure */
    init_metadata_struct (&xml_metadata);

    /* Parse the metadata file into our internal metadata structure; also
       allocates space as needed for various pointers in the global and band
       metadata */
    if (parse_metadata (espa_xml_file, &xml_metadata) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Create the HDF file for the HDF metadata from the XML metadata */
    if (create_hdf_metadata (hdf_file, &xml_metadata) != SUCCESS)
    {
        sprintf (errmsg, "Creating the HDF metadata file (%s) which links to "
            "the raw binary bands as external SDSs.", hdf_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write out the ENVI header for the HDF product */
    indx = -99;
    for (i = 0; i < xml_metadata.nbands; i++)
    {
        /* If this is the sr_band1 then use it for the ENVI header info */
        if (!strcmp (xml_metadata.band[i].name, hdfname_mapping[0][0]))
            indx = i;
    }

    /* Create the ENVI structure using the index band */
    if (create_envi_struct (&xml_metadata.band[indx], &xml_metadata.global,
        &envi_hdr) != SUCCESS)
    {
        sprintf (errmsg, "Creating the ENVI header for %s", hdf_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Make sure the number of bands being written doesn't exceed the
       maximum defined ENVI header bands */
    if (xml_metadata.nbands > MAX_ENVI_BANDS)
    {
        sprintf (errmsg, "Number of bands being written exceeds the "
            "predefined maximum of bands in envi_header.h: %d",
            MAX_ENVI_BANDS);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Update a few of the parameters in the header file */
    count = snprintf (envi_hdr.file_type, sizeof (envi_hdr.file_type),
        "HDF scientific data");
    if (count < 0 || count >= sizeof (envi_hdr.file_type))
    {
        sprintf (errmsg, "Overflow of envi_hdr.file_type string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write the ENVI header for the HDF file */
    count = snprintf (hdr_file, sizeof (hdr_file), "%s.hdr", hdf_file);
    if (count < 0 || count >= sizeof (hdr_file))
    {
        sprintf (errmsg, "Overflow of hdr_file string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (write_envi_hdr (hdr_file, &envi_hdr) != SUCCESS)
    {
        sprintf (errmsg, "Writing the ENVI header: %s", hdr_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Free the metadata structure */
    free_metadata (&xml_metadata);

    /* Successful conversion */
    return (SUCCESS);
}
