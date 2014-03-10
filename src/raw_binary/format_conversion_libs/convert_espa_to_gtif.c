/*****************************************************************************
FILE: convert_espa_to_gtif.c
  
PURPOSE: Contains functions for creating the GeoTIFF products for each of
the bands in the XML file.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/9/2014     Gail Schmidt     Original development

NOTES:
  1. The XML metadata format written via this library follows the ESPA internal
     metadata format found in ESPA Raw Binary Format v1.0.doc.  The schema for
     the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/static/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/

#include "convert_espa_to_gtif.h"

/******************************************************************************
MODULE:  convert_espa_to_gtif

PURPOSE: Converts the internal ESPA raw binary file to GeoTIFF file format.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error converting to GeoTIFF
SUCCESS         Successfully converted to GeoTIFF

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/9/2014     Gail Schmidt     Original development

NOTES:
  1. The GDAL tools will be used for converting the raw binary (ENVI format)
     files to GeoTIFF.
  2. An associated .tfw (ESRI world file) will be generated for each GeoTIFF
     file.
******************************************************************************/
int convert_espa_to_gtif
(
    char *espa_xml_file,   /* I: input ESPA XML metadata filename */
    char *gtif_file        /* I: base output GeoTIFF filename */
)
{
    char FUNC_NAME[] = "convert_espa_to_gtif";  /* function name */
    char errmsg[STR_SIZE];      /* error message */
    char gdal_cmd[STR_SIZE];    /* command string for GDAL call */
    char gtif_band[STR_SIZE];   /* name of the GeoTIFF file for this band */
    char *cptr = NULL;          /* pointer to empty space in the band name */
    int i;                      /* looping variable for each band */
    int count;                  /* number of chars copied in snprintf */
    Espa_internal_meta_t xml_metadata;  /* XML metadata structure to be
                                   populated by reading the XML metadata file */

    /* Validate the input metadata file */
    if (validate_xml_file (espa_xml_file, ESPA_SCHEMA) != SUCCESS)
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

    /* Loop through the bands in the XML file and convert them to GeoTIFF.
       The filenames will have the GeoTIFF base name followed by _ and the
       band name of each band in the XML file.  Blank spaced in the band name
       will be replaced with underscores. */
    for (i = 0; i < xml_metadata.nbands; i++)
    {
        /* Determine the output GeoTIFF band name */
        count = snprintf (gtif_band, sizeof (gtif_band), "%s_%s.tif", gtif_file,
            xml_metadata.band[i].name);
        if (count < 0 || count >= sizeof (gtif_band))
        {
            sprintf (errmsg, "Overflow of gtif_file string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Loop through this filename and replace any occurances of blank
           spaces with underscores */
        while ((cptr = strchr (gtif_band, ' ')) != NULL)
            *cptr = '_';

        /* Convert the files */
        printf ("Converting %s to %s\n", xml_metadata.band[i].file_name,
            gtif_band);
        count = snprintf (gdal_cmd, sizeof (gdal_cmd),
            "gdal_translate -of Gtiff -a_nodata %ld -co \"TFW=YES\" -q %s %s",
            xml_metadata.band[i].fill_value, xml_metadata.band[i].file_name,
            gtif_band);
        if (count < 0 || count >= sizeof (gdal_cmd))
        {
            sprintf (errmsg, "Overflow of gdal_cmd string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (system (gdal_cmd) == -1)
        {
            sprintf (errmsg, "Running gdal_translate: %s", gdal_cmd);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Free the metadata structure */
    free_metadata (&xml_metadata);

    /* Successful conversion */
    return (SUCCESS);
}
