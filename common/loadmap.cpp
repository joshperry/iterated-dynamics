/** loadmap.c **/


#include        <string.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"

/***************************************************************************/

#define dac ((Palettetype *)g_dac_box)

int ValidateLuts(const char *fn)
{
    FILE * f;
    unsigned        r, g, b, index;
    char    line[160];
    char    temp[FILE_MAX_PATH+1];
    char    temp_fn[FILE_MAX_PATH];
    strcpy(temp,MAP_name);
    strcpy(temp_fn,fn);
#ifdef XFRACT
    merge_pathnames(temp,temp_fn,3);
#else
    merge_pathnames(temp,temp_fn,0);
#endif
    if (has_ext(temp) == nullptr) /* Did name have an extension? */
        strcat(temp,".map");  /* No? Then add .map */
    findpath(temp, line);         /* search the dos path */
    f = fopen(line, "r");
    if (f == nullptr) {
        sprintf(line,"Could not load color map %s",fn);
        stopmsg(0,line);
        return 1;
    }
    for (index = 0; index < 256; index++) {
        if (fgets(line,100,f) == nullptr)
            break;
        sscanf(line, "%u %u %u", &r, &g, &b);
        /** load global dac values **/
        dac[index].red   = (BYTE)((r%256) >> 2);/* maps default to 8 bits */
        dac[index].green = (BYTE)((g%256) >> 2);/* DAC wants 6 bits */
        dac[index].blue  = (BYTE)((b%256) >> 2);
    }
    fclose(f);
    while (index < 256)  { /* zap unset entries */
        dac[index].red = dac[index].blue = dac[index].green = 40;
        ++index;
    }
    colorstate = 2;
    strcpy(colorfile,fn);
    return 0;
}


/***************************************************************************/

int SetColorPaletteName(char * fn)
{
    if (ValidateLuts(fn) != 0)
    {
        return 1;
    }
    if (mapdacbox == nullptr)
    {
        mapdacbox = (BYTE *) malloc(768L);
        if (mapdacbox == nullptr)
        {
            stopmsg(0, "Insufficient memory for color map.");
            return 1;
        }
    }
    memcpy((char *) mapdacbox, (char *) g_dac_box, 768);
    return 0;
}

