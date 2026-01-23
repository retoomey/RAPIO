// ######################################################################################
//       library_sci_palette.h
//       by Brian T Kaney, [2017-2018]
// ######################################################################################

#ifndef SCI_PALETTE_H
#define SCI_PALETTE_H

#define LOW   0 // --used as the array index to specify the upper
#define HIGH  1 // --and lower range values in a data palette range

#define RED   0 // -----to specify color indicies
#define GREEN 1
#define BLUE  2

typedef struct { int          number_range_colors;
                 short int ** ranges;
                 int **       range_palette;

                 int          number_linear_colors;
                 short int    min;
                 short int    max;
                 int **       linear_palette;
}
SCIPalette;

SCIPalette *
SCIPaletteFromFile(char * filename);
void
DestroySCIPalette(SCIPalette * pal);

#endif // ifndef SCI_PALETTE_H
