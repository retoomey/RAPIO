/** Toomey August 2024
 * Using ESM 6 (2015) modules
 *
 * Tools for applying/implementing RAPIO JSON color maps
 */

/** Base of all color maps used by the RAPIODataTileLayer */
export class ColorMapBase {
  constructor(name){
     this.name = name;
  }
  /** Do we need this in javascript since we are typeless? */
  apply(floatData, imageData){}
}

/** The regular JSON created color map. */
export class ColorMap extends ColorMapBase {
  /** Create a regular color map */
  constructor(name, colors){
     super(name);
     this.colors = colors;
  }

  /** Apply our color map data */
  apply(floatData, imageData){
    const data = imageData.data;

    // Process each pixel
    for (let i = 0, index = 0; i < data.length; i += 4, index++) {

       // Quick hunt bin by upper bound
       let u = this.colors.findIndex(color => color.upper > floatData[index]);
       if (u === -1) u = this.colors.length - 1;

       // Found bin, apply the colors
       const c = this.colors[u];
       if (c.linear){
          // Linear bin
          const wt = (floatData[index] - c.lower) / c.delta;
          const nwt = (1.0 - wt);
          data[i] = Math.floor(0.5 + nwt * c.r + wt * c.r2) & 0xFF; // Red
          data[i + 1] = Math.floor(0.5 + nwt * c.g + wt * c.g2) & 0xFF; // Green
          data[i + 2] = Math.floor(0.5 + nwt * c.b + wt * c.b2) & 0xFF; // Blue
          data[i + 3] = Math.floor(0.5 + nwt * c.a + wt * c.a2) & 0xFF; // Alpha
       }else{
          // Single bin
          data[i] = c.r;     // Red
          data[i + 1] = c.g; // Green
          data[i + 2] = c.b; // Blue
          data[i + 3] = c.a; // Alpha
       }
    }
  }
}

/** Simple color map, hard setting some test colors for various things.
 * This can also be a default map until a color map is assigned */
export class SimpleColorMap extends ColorMapBase {
  constructor(name){
    super(name);
  }

  /** Apply our color map data */
  apply(floatData, imageData){
    const data = imageData.data;

    // Process each pixel
    for (let i = 0, index = 0; i < data.length; i += 4, index++) {

       const v = floatData[index];
       // isGood from RAPIO.  Eh.
       //if (v > -99899.0) || (v != -99900.0 && v != -99901 && v != -99903){
       if (v > -99899.0){ // isGood approx
          data[i] = 0;     // Red
          data[i + 1] = 255; // Green
          data[i + 2] = 0; // Blue
          data[i + 3] = 255; // Alpha
       }else{
	  // The 
          data[i] = 255;     // Red
          data[i + 1] = 0; // Green
          data[i + 2] = 0; // Blue
          data[i + 3] = 255; // Alpha
       }
    }
  }
}

/** Fetch a color map JSON and handle it.
 * FIXME: Currently hardlinked to my server.  Will update with URL and a binary file call option.
 */
export async function fetchColorMap() {
  try{
    const response = await fetch('./colormap'); // should be param or config right?
    const data = await response.json();

    // It's our job here to make sure the color map is valid
    // and ready to use.
    
    // --------------------------------------------------
    // Convert number special values from c++ specials
    const replacements = {
       "inf": Infinity,
       "-inf": -Infinity,
       "nan": NaN
    };
    const convertToBoolean = value => value === "true" ? true : value === "false" ? false : value;

    // Convert array, though I might deprecate this and just use 'upper'
    //data.UpperBounds = data.UpperBounds.map(value => replacements[value] || value);

    // Convert critical values in the color storage if needed
    // Boost property tree c++ writes numbers as strings (annoying) but we may be
    // replacing it with a better json library.  So we'll check if they are still
    // strings or not and convert
    for (let i = 0; i < data.Colors.length; i++) {
      const c = data.Colors[i];
        if (typeof c.lower === 'string'){ 
          c.lower = replacements[c.lower] || parseFloat(c.lower);
        }
        if (typeof c.upper === 'string'){ 
          c.upper = replacements[c.upper] || parseFloat(c.upper);
        }
        if (typeof c.delta === 'string'){ 
          c.delta = replacements[c.delta] || parseFloat(c.delta);
        }
        if (typeof c.linear === 'string'){
          c.linear = convertToBoolean(c.linear);
        }
        if (typeof c.r === 'string'){ 
          c.r = replacements[c.r] || parseInt(c.r);
        }
        if (typeof c.g === 'string'){ 
          c.g = replacements[c.g] || parseInt(c.g);
        }
        if (typeof c.b === 'string'){ 
          c.b = replacements[c.b] || parseInt(c.b);
        }
        if (typeof c.a === 'string'){ 
          c.a = replacements[c.a] || parseInt(c.a);
        }
        if (typeof c.r2 === 'string'){ 
          c.r2 = replacements[c.r2] || parseInt(c.r2);
        }
        if (typeof c.g2 === 'string'){ 
          c.g2 = replacements[c.g2] || parseInt(c.g2);
        }
        if (typeof c.b2 === 'string'){ 
          c.b2 = replacements[c.b2] || parseInt(c.b2);
        }
        if (typeof c.a2 === 'string'){ 
          c.a2 = replacements[c.a2] || parseInt(c.a2);
        }
    }
    // --------------------------------------------------

    const currentColorMap = new ColorMap(data.name, data.Colors);
    return currentColorMap;

    } catch (error) {
       console.error('Failed to fetch/parse colormap:', error);
       throw error;
    }
}
