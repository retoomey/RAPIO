/** Toomey August 2024
 * Using ESM 6 (2015) modules
 *
 * A custom leaflet data layer using raw data tiles returned by a RAPIO tool or webserver URL
 * Raw data tiles are a generated subview of actual data, not colored.
 * The actual visible tiles must have a color map applied.
 *
 * FIXME: Probably will pull out the tile caching to separate file.
 */

import { ColorMap } from './rColorMap.js';
import { SimpleColorMap } from './rColorMap.js';
//import { * } as ColorMaps from './rColorMap.js'; ColorMaps.ColorMap 

/*  Create tile layer that uses our server and mrmsdata format */
export var RAPIODataTileLayer = L.TileLayer.extend({

  initialize: function() {
    // Call the parent class constructor
    L.TileLayer.prototype.initialize.apply(this, arguments);

    // Initialize the cache as a Map
    this.tileCache = new Map();

    // Until our color map is set, use a default one
    this.colorMap = new SimpleColorMap();

    // Set up event listeners
    this.on('tileunload', this.handleTileUnload, this);
  },

  createTile(coords, done) {
    const tile = document.createElement('canvas');
    tile.coords = coords;

    // Get the URL we need
    const url = this.getTileUrl(coords);

    // Set initial tile sizes, nothing in it yet
    // Think we can draw error text or something
    var tileSize = this.getTileSize();
    tile.setAttribute('width', tileSize.x);
    tile.setAttribute('height', tileSize.y);

    // We have to pull the data ourselves.  If we use img and
    // auto browser loading and then canvas we get distorted
    // data values.  But since it's our tile server we can do
    // whatever we want.  Currently we just send 256*256*float
    fetch(url)
      .then(response => { // Async action
        if (!response.ok) {
          throw new Error(`Network response was not ok: ${response.statusText}`);
        }
        return response.arrayBuffer();
      })
      .then(buffer => { // Async success read

        // Get our raw MRMS data tile
        const byteLength = buffer.byteLength;
	if (byteLength != 262144){
          throw new Error("Unexpected MRMS data tile size: "+byteLength+" != 262144");
	}
	// Create a rawData view..
	var floatData = new Float32Array(buffer);

        // Get canvas data
        var ctx = tile.getContext('2d');
        var imageData = ctx.createImageData(tileSize.x, tileSize.y);
        var data = imageData.data;

        // Apply color map to the image data
	this.colorMap.apply(floatData, imageData);
        ctx.putImageData(imageData, 0, 0);

	// Generate a unique key for the tile and store the canvas in the cache
        var tileKey = `${coords.z}_${coords.x}_${coords.y}`;

        this.tileCache.set(tileKey, {
            canvas: tile,
            imageData: imageData,
	    floatData: floatData
        });

        done(null, tile);
      })
      .catch(error => { // Async fail read
        done(error, null);
      });

    return tile;
  },

  /** Set the color map we use */
  setColorMap: function(colorMap) {
     this.colorMap = colorMap;
     // FIXME: Do we trigger a cache purge/refresh or make caller do it?
  },

  // Method to get a tile from the cache if it exists
  getTileFromCache: function(tileKey) {
      return this.tileCache.get(tileKey);
  },

  // Method to get the size of the tileCache
  getTileCacheSize: function() {
        return this.tileCache.size;
  },

  // Handle the tileunload event
  handleTileUnload: function(event) {
      // Remove the tile from the cache
      const tileKey = `${event.coords.z}_${event.coords.x}_${event.coords.y}`;
      this.tileCache.delete(tileKey);
      //console.log("Deleted key "+tileKey+"\n");
  }

});

