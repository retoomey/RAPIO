// Toomey July 2024
// This is a mess as I experiment/learn this.
// Lot's of potential for cool things here.
// 1.  Trying to implement dynamic client side color maps
// 2.  Will try overlay readout.
// 3.  Will try a table tracking readout.
// I think 2 and 3 will work best using the tiles for the data,
// we'll see.
//
// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include <iostream>

// Extra widgets
// https://github.com/Flix01/imgui
// has a bunchs of extra classes.  We're going to
// have to make our own widgets at some point
//#include "imguidatechooser.h"

// I just added the header directly for us
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h> 
#include <emscripten/fetch.h> 
#include <functional>
static std::function<void()>            MainLoopForEmscriptenP;
static void MainLoopForEmscripten()     { MainLoopForEmscriptenP(); }
#define EMSCRIPTEN_MAINLOOP_BEGIN       MainLoopForEmscriptenP = [&]()
#define EMSCRIPTEN_MAINLOOP_END         ; emscripten_set_main_loop(MainLoopForEmscripten, 0, true)
#else
#define EMSCRIPTEN_MAINLOOP_BEGIN
#define EMSCRIPTEN_MAINLOOP_END
#endif

/** Store all of the application state in a single object.
 * This wil be easier to pass around.  Also if we write
 * to cookies or database to maintain will have in one place
 * for easy load/unload.
 */
class myApplicationState {
public:
   /** Default settings to start up */
   myApplicationState():
     radioMapDataType(0), 
     backgroundMapShow(true), backgroundMap(0), 
     dataMap(0), showDebugTiles(false)
   {};
	   
   int radioMapDataType;   //<<< Map data type choice radio
   bool backgroundMapShow; //<<< Show the background map
   int backgroundMap;      //<<< Chosen background map
   int dataMap;            //<<< The displayed map URL
   bool showDebugTiles;    //<<< Show the tile outline/debug info
};

/** Variables that leaflet sets */
// Access JavaScript variables
double get_lat() {
    return EM_ASM_DOUBLE({
        return Module.lat;
    });
}

double getReadoutValue() {
    return EM_ASM_DOUBLE({
        return Module.dataValue;
    });
}

/** Get current lat/lon from the mouse over map */
bool getLatLon(double& lat, double& lon)
{
  bool inside = EM_ASM_INT({
     return Module.isMouseOverMap;
  });

  // Skip the other fields for speed if
  // we don't need them
  if (inside){
    lat = EM_ASM_DOUBLE({
      return Module.lat;
    });
    lon = EM_ASM_DOUBLE({
      return Module.lon;
    });
  }
  return inside;
}

/*
// Function to get coordinates and tile information
EM_JS(void, get_tile_info, (double* lon, double* lat, int* zoom, char* tileKey, int* pixelX, int* pixelY, bool* inside), {
    setValue(lon, Module.lon, 'double');
    setValue(lat, Module.lat, 'double');
    setValue(zoom, Module.zoom, 'i32');
    stringToUTF8(Module.tileKey, tileKey, 256); // Assumes tileKey buffer size is 256
    setValue(pixelX, Module.pixelX, 'i32');
    setValue(pixelY, Module.pixelY, 'i32');
    setValue(inside, Module.isMouseOverMap, 'i8');
});
get_tile_info(&lon, &lat, &zoom, tileKey, &pixelX, &pixelY, &mouseOverMap);
*/

double get_lon() {
    return EM_ASM_DOUBLE({
        return Module.lon;
    });
}

bool is_mouse_over_map() {
    return EM_ASM_INT({
        return Module.isMouseOverMap;
    });
}

// Calling from javascript Module. to us.
float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

// Experiments to fetch from webserver directly -------------

std::atomic<bool> isFetchInProgress(false);

void OnDataFetched(emscripten_fetch_t *fetch) {
    if (fetch->status == 200) {
       std::string data((char*)fetch->data, fetch->numBytes);
       std::cout << "String is "<<data<<"\n";
    } else {
       std::cout << "Failed to fetch data:" << fetch->status << "\n";
    }
    isFetchInProgress.store(false);// reset
    emscripten_fetch_close(fetch);
}

/** Fetch color map info */
void FetchData() {
    // Keep from reenter, not sure working 100%
    if (isFetchInProgress.load()){
	   return;
    }
    isFetchInProgress.store(true);

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    // Set fields after init
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY
	      | EMSCRIPTEN_FETCH_REPLACE;
    attr.onsuccess = OnDataFetched;
    attr.onerror = OnDataFetched;

    attr.onerror = [](emscripten_fetch_t *fetch) {
      printf("The download actually failed.\n");
      isFetchInProgress.store(false);// reset
    };
    /*
    attr.onprogress = [](emscripten_fetch_t *fetch) {
      if (fetch->status != 200) return;
      printf("onprogress: dataOffset: %llu, numBytes: %llu, totalBytes: %llu\n", fetch->dataOffset, fetch->numBytes, fetch->totalBytes);
      if (fetch->totalBytes > 0) {
        printf("Downloading.. %.2f%% complete.\n", (fetch->dataOffset + fetch->numBytes) * 100.0 / fetch->totalBytes);
      } else {
        printf("Downloading.. %lld bytes complete.\n", fetch->dataOffset + fetch->numBytes);
      }
    };
    */

    //attr.onsuccess = [](emscripten_fetch_t *fetch) {
    //  printf("Good download.\n");
    //  isFetchInProgress.store(false);// reset
    //};
    emscripten_fetch(&attr, "http://localhost:8080/colormap");
    //emscripten_fetch(&attr, "https://raw.githubusercontent.com/retoomey/RAPIO/master/webexample/main.cpp");

}
// END Experiments to fetch from webserver directly -------------

/** Call fetchColorMap from our javascript library */
void doFetchColorMap() {
  EM_ASM({
    // Create a new promise to handle the asynchronous fetch operation
    // Since Enscripten doesn't handle await guess we're stuck with this
    // We have a async load the module and async call the method...
    const p = new Promise((resolve, reject) => {
      // Dynamically import the module
      import('./rColorMap.js').then(module => {
        // Call the fetchColorMap function from the imported module
        module.fetchColorMap()
          .then(data => {
	
	    // Tell the map about new color map
	    Module.map.myRAPIOLayer.setColorMap(data);
	    // FIXME: Can we trigger a refresh of map..maybe in the main loop is best?
            resolve();  // Resolve the promise once fetchColorMap completes successfully
          })
          .catch(error => {
            console.error("Error calling fetchColorMap:", error);
            reject(error);  // Reject the promise if there's an error
          });
      }).catch(err => {
        console.error("Failed to import module:", err);
        reject(err);  // Reject the promise if the module import fails
      });
    });

    // Handle the promise returned by the asynchronous operation
    p.then(() => {
      //console.log("Promise resolved in EM_ASM");
    }).catch(error => {
      console.error("Promise rejected in EM_ASM:", error);
    });
  });
}

EMSCRIPTEN_BINDINGS(my_module) {
  emscripten::function("lerp", &lerp);
  emscripten::class_<myApplicationState>("myApplicationState")
        .constructor<>()
        .property("radioMapDataType", &myApplicationState::radioMapDataType)
        .property("backgroundMap", &myApplicationState::backgroundMap)
        .property("showDebugTiles", &myApplicationState::showDebugTiles);
}

/** void initMap()
 * Create the basic map setup we use
 * https://leaflet-extras.github.io/leaflet-providers/preview/index.html
 *
 */
EM_JS(void, initMap, (), {

   var map = L.map('map').setView([35.3333, -97.2777], 3);

   Module.map = map;
   //Module.tileCache = new WeakMap();
   Module.tileCache = {};
   Module.lat = 0.0;
   Module.lon = 0.0;
   Module.zoom = 0;
   Module.isMouseOverMap = false;

   // Function to calculate tile information
   function getTileInfo(latlng, zoom) {
     // Convert lat/lng to pixel coordinates at the given zoom level
     var point = map.project(latlng, zoom);
     var tileSize = 256; // Default tile size in Leaflet

     // Calculate the tile coordinates
     var tileX = Math.floor(point.x / tileSize);
     var tileY = Math.floor(point.y / tileSize);

     // Calculate the pixel coordinates within the tile
     var pixelX = Math.floor(point.x % tileSize);
     var pixelY = Math.floor(point.y % tileSize);

     return {
       tileX: tileX,
       tileY: tileY,
       pixelX: pixelX,
       pixelY: pixelY
     };
   }

   // Attach move event listener (tracking map itself)
   // Not using yet
  // map.on('move', function(event) {
     // console.log('Map moved:', event);
     // console.log('Current center:', map.getCenter());
  // });
   map.on('resize', function(event){
     initGLCanvas();
   });


   let openGLSetup = false;

   // Mouse movement
   map.on('mousemove', function(event) {

     if (!openGLSetup){
      console.log("-------->Setting up opengl");
       initGLCanvas();
       openGLSetup = true;
     }
     var zoom = map.getZoom();
    // var tileInfo = getTileInfo(event.latlng, zoom);
   //console.log('Mouse moved to:', event.latlng);
   //console.log('Tile coordinates:', tileInfo.tileX, tileInfo.tileY);
   //console.log('Pixel position within tile:', tileInfo.pixelX, tileInfo.pixelY);
     var point = map.project(event.latlng, zoom);
     var tileSize = 256; // Default tile size in Leaflet

     // Calculate the tile coordinates
     var tileX = Math.floor(point.x / tileSize);
     var tileY = Math.floor(point.y / tileSize);

     // Calculate the pixel coordinates within the tile
     var pixelX = Math.floor(point.x % tileSize);
     var pixelY = Math.floor(point.y % tileSize);

     Module.lat = event.latlng.lat; // double
     Module.lon = event.latlng.lng; // double
     Module.zoom = zoom; // int
     Module.isMouseOverMap = true;
     Module.dataValue = 0.0; // First attempt at read out

     // Looking for tile...see if it exists...
     if (Module.map.myRAPIOLayer){
        //console.log("We have the layer!\n");
	// Try to get the tile matching coordinates
        var tileKey = `${zoom}_${tileX}_${tileY}`;
	var cachedTile = Module.map.myRAPIOLayer.getTileFromCache(tileKey);
	if (cachedTile){
	   //console.log("Tile found:", tileKey);
	   // Ok attempt to get the value in the tile at location x,y
           Module.dataValue = cachedTile.floatData[pixelY*256+pixelX]; // guessing
	}else{
	   //console.log("Tile NOT found:", tileKey);
	}
     }

     // Try opengl overlay hack attempt
     var mouseEvent = event.originalEvent;
     drawReadout(mouseEvent.clientX, mouseEvent.clientY);
   });

   // Mouse out of map
   map.on('mouseout', function() {
     Module.isMouseOverMap = false;
     Module.dataValue = 0.0;
     clearReadout();
   });

});

/** Set up the RAPIO map layer */
EM_JS(void, initRAPIOLayer, (), {
  Module.mapReady = false;

  // Can't use 'await' in EM_JS, so here we are promising
  const mapReady = new Promise((resolve, reject) => {
  import('./rDataLayer.js').then(module => {

  // ----------------------------------------------------------------------
  //
  // Data pull layer (Color map is client side)
  var RAPIOMap = new module.RAPIODataTileLayer('http://localhost:8080/tmsdata/{z}/{x}/{y}', {
    maxZoom: 18,
    attribution: 'Map data &copy; <a href="https://github.com/retoomey/RAPIO">RAPIO</a>',
    id: 'data',
    tileSize: 256,
    transparent: true
  });
  Module.map.myRAPIOLayer = RAPIOMap;

  // Regular image layer (Color map is server side)
  var RAPIOImageMap = new L.TileLayer('http://localhost:8080/tms/{z}/{x}/{y}', {
    maxZoom: 18,
    attribution: 'Map data &copy; <a href="https://github.com/retoomey/RAPIO">RAPIO</a>',
    id: 'data',
    tileSize: 256,
    transparent: true
  });
  Module.map.myRAPIOImageLayer = RAPIOImageMap;

  Module.mapReady = true;
  resolve();
  // ----------------------------------------------------------------------

  }).catch(err => {
    console.error("Failed to load/create RAPIO data layer (rDataLayer.js): ",err);
    reject(err);
  });

});

});

EM_JS(bool, is_layer_ready, (), {
  return Module.mapReady;
});

/** Create debug tile layer.  This is better than us
 * generating it in the tile itself */
EM_JS(void, initDebugLayer, (), {

  L.GridLayer.DebugCoords = L.GridLayer.extend({
    createTile: function (coords) {
       var tile = document.createElement('div');

       var text = document.createElement('span');
       text.innerHTML = [coords.x, coords.y, coords.z].join(', ');
       text.style.position = 'relative';
       text.style.left = '10px';
       text.style.fontSize = '16px';

       // Create a black border around white text
       text.style.color = 'white';
       text.style.textShadow = `
           -1px -1px 0 black,
           1px -1px 0 black,
           -1px 1px 0 black,
           1px 1px 0 black`; 
       tile.appendChild(text);

       // Tile style
       tile.style.outline = '2px solid red';

       return tile;
    }
  });

  L.gridLayer.debugCoords = function(opts) {
    return new L.GridLayer.DebugCoords(opts);
  };

  Module.map.myDebugLayer =  L.gridLayer.debugCoords();
});

// Define a JavaScript function to change the tile layer of the Leaflet map
EM_JS(void, changeMapLayer, (bool backgroundMapShow, int backgroundMap, int dataMap, bool showDebugTiles), {

    //console.log(">>>changing layers! "+backgroundMapShow+"\n");

    // Remove all layers
    Module.map.eachLayer(function (layer) {
        Module.map.removeLayer(layer);
    });

    if (backgroundMapShow == true){
    // Choose background map
    if (backgroundMap == 0){

      var Stadia_AlidadeSmoothDark = L.tileLayer('https://tiles.stadiamaps.com/tiles/alidade_smooth_dark/{z}/{x}/{y}{r}.{ext}', {
	minZoom: 0,
	maxZoom: 20,
	attribution: '&copy; <a href="https://www.stadiamaps.com/" target="_blank">Stadia Maps</a> &copy; <a href="https://openmaptiles.org/" target="_blank">OpenMapTiles</a> &copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
	ext: 'png'
      });

      Module.map.addLayer(Stadia_AlidadeSmoothDark);
   }else{

      var newTileLayer = L.tileLayer('http://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 18,
        attribution: 'Map data &copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors, ' +
                    'Imagery © <a href="https://www.mapbox.com/">Mapbox</a>',
        id: 'mapbox/streets-v11',
        tileSize: 512,
        zoomOffset: -1
     });

      Module.map.addLayer(newTileLayer);
    }
    }

    // Map layer
    if (dataMap == 0){
      Module.map.addLayer(Module.map.myRAPIOImageLayer);
    }else {
      Module.map.addLayer(Module.map.myRAPIOLayer);
    }

    // Debug layer
    if(showDebugTiles){
      Module.map.addLayer(Module.map.myDebugLayer);
    }
});

/*
void ShowFontSelector(const char* label)
{
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font_current = ImGui::GetFont();
    if (ImGui::BeginCombo(label, font_current->GetDebugName()))
    {
        for (ImFont* font : io.Fonts->Fonts)
        {
            ImGui::PushID((void*)font);
            if (ImGui::Selectable(font->GetDebugName(), font == font_current))
                io.FontDefault = font;
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
}
*/

/** Show the list of background maps */
void showMapListCombo(myApplicationState& state) {
  static const char* items[] = { "Stadia Black", "White" };
  if (ImGui::Checkbox("Background:", &state.backgroundMapShow)){
    changeMapLayer(state.backgroundMapShow, state.backgroundMap, state.dataMap, state.showDebugTiles);
  }
  ImGui::SameLine();
  if (ImGui::BeginCombo("##maplistcombo", items[state.backgroundMap])) 
  {
	for (int n = 0; n < IM_ARRAYSIZE(items); n++)
        {
            bool is_selected = (state.backgroundMap == n);
            if (ImGui::Selectable(items[n], is_selected))
            {
		state.backgroundMap = n;
		state.backgroundMapShow = true;
		// EM_JS needs hoops for class
                changeMapLayer(state.backgroundMapShow, state.backgroundMap, state.dataMap, state.showDebugTiles);
            }

            // Set the initial focus when opening the combo
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
    ImGui::EndCombo();
  }
}

void renderLeafletMap() {
//    GLuint texture = updateTextureFromCanvas();
    ImGui::Begin("Leaflet Map");
 //   ImGui::Image((void*)(intptr_t)texture, ImVec2(512, 384));
    //if (ImGui::Button("Close Me")){
   //    show_another_window = false;
   // }
    emscripten_run_script("console.log('Called from C++');");
    ImGui::End();
}

/** Hardcode a style for now from ImThemes.  Actually I think
 * having a menu and theme files for a few would be nice at some
 * point.  Though since we can't 'read' in the web display
 * we'd pull them through the server. */
void SetupImGuiStyle()
{
	// Sonic Riders style by Sewer56 from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 0.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 4.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.729411780834198f, 0.7490196228027344f, 0.7372549176216125f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08627451211214066f, 0.08627451211214066f, 0.08627451211214066f, 0.9399999976158142f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 0.5f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.7098039388656616f, 0.3882353007793427f, 0.3882353007793427f, 0.5400000214576721f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 0.4000000059604645f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 0.6700000166893005f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.4666666686534882f, 0.2196078449487686f, 0.2196078449487686f, 0.6700000166893005f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.4666666686534882f, 0.2196078449487686f, 0.2196078449487686f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.4666666686534882f, 0.2196078449487686f, 0.2196078449487686f, 0.6700000166893005f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.3372549116611481f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.7098039388656616f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.4666666686534882f, 0.2196078449487686f, 0.2196078449487686f, 0.6499999761581421f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.7098039388656616f, 0.3882353007793427f, 0.3882353007793427f, 0.6499999761581421f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 0.5f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.7098039388656616f, 0.3882353007793427f, 0.3882353007793427f, 0.5400000214576721f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 0.6499999761581421f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 0.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.7098039388656616f, 0.3882353007793427f, 0.3882353007793427f, 0.5400000214576721f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.7098039388656616f, 0.3882353007793427f, 0.3882353007793427f, 0.5400000214576721f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.7098039388656616f, 0.3882353007793427f, 0.3882353007793427f, 0.5400000214576721f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 0.6600000262260437f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 0.6600000262260437f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.7098039388656616f, 0.3882353007793427f, 0.3882353007793427f, 0.5400000214576721f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 0.6600000262260437f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.8392156958580017f, 0.658823549747467f, 0.658823549747467f, 0.6600000262260437f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.09803921729326248f, 0.1490196138620377f, 0.9700000286102295f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1372549086809158f, 0.2588235437870026f, 0.4196078479290009f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

/** Show the readout feedback area */
static void showReadout()
{
  double lat, lon;
  bool mouseOver = getLatLon(lat, lon);

  if (mouseOver){
    const auto v = getReadoutValue();
    ImGui::Text("Readout: %.2f [%.2f,%.2f]", v, lat, lon);
  }else{
    ImGui::Text("Readout: ---");
  }

}

static void ShowDesktop(bool* p_open, myApplicationState& state)
{
    static bool use_work_area = true;
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);
    //static ImGuiWindowFlags flags;

    // Needs to be static to not 'reset' after change
    static ImVec4 clear_color = ImVec4(0/255.0f, 88/255.0f, 128/255.0f, 1.00f);

   // static bool toggle = false;

    if (ImGui::Begin("Main window", p_open, flags))
    {
	showReadout();

        //ImGui::ColorEdit3("A color", (float*)&clear_color); 
        if (ImGui::Checkbox("Show Debug Tiles", &state.showDebugTiles)){
           changeMapLayer(state.backgroundMapShow, state.backgroundMap, state.dataMap, state.showDebugTiles);
	}
	showMapListCombo(state);

	// Radio choice
	int old = state.dataMap;
        ImGui::Text("Data mode:"); ImGui::SameLine();
        ImGui::RadioButton("Image", &state.dataMap, 0); ImGui::SameLine();
        ImGui::RadioButton("Data", &state.dataMap, 1);
	if (old != state.dataMap){
           changeMapLayer(state.backgroundMapShow, state.backgroundMap, state.dataMap, state.showDebugTiles);
	}

	// FIXME: if works probably want one function right?
	//double lon = get_lon();
	//double lat = get_lat();
        //bool mouseOver = is_mouse_over_map();

	if (ImGui::Button("Fetch Colormap")) {
	  doFetchColorMap();
        }

#if 0
        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

    const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);
        if (ImGui::BeginTable("color_scroller", 3, flags, outer_size))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            ImGui::TableSetupColumn("U", ImGuiTableColumnFlags_None);
            //ImGui::TableSetupColumn("Two", ImGuiTableColumnFlags_None);
            //ImGui::TableSetupColumn("Three", ImGuiTableColumnFlags_None);
            ImGui::TableHeadersRow();

            // Demonstrate using clipper for large vertical lists
            ImGuiListClipper clipper;
            clipper.Begin(1000);
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::TableNextRow();
                    for (int column = 0; column < 1; column++)
                    {
                        ImGui::TableSetColumnIndex(column);
                        ImGui::Text("Hello %d,%d", column, row);
                    }
                }
            }
            ImGui::EndTable();
        }
#endif

	// ------------------------------------------------
	// Convenient for now as we learn the basics
	static bool show_demo_window = false;
        ImGui::Checkbox("Demo Widgets", &show_demo_window);
        if (show_demo_window){
           ImGui::ShowDemoWindow(&show_demo_window);
	}
	// ------------------------------------------------

        //ShowFontSelector("Fonts##Selector");
	/*
	 *
    if (ImGui::IsItemHovered()) // still learning.  
    {
        if (radioValue == 0)
        {
            ImGui::SetTooltip("Select Option 1");
        }
    }
    */

	//ImGui::TestDateChooser();
#if 0
        ImGui::Checkbox("Use work area instead of main area", &use_work_area);
        ImGui::SameLine();
        //HelpMarker("Main Area = entire viewport,\nWork Area = entire viewport minus sections used by the main menu bars, task bars etc.\n\nEnable the main-menu bar in Examples menu to see the difference.");

        ImGui::CheckboxFlags("ImGuiWindowFlags_NoBackground", &flags, ImGuiWindowFlags_NoBackground);
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoDecoration", &flags, ImGuiWindowFlags_NoDecoration);
        ImGui::Indent();
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoTitleBar", &flags, ImGuiWindowFlags_NoTitleBar);
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoCollapse", &flags, ImGuiWindowFlags_NoCollapse);
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoScrollbar", &flags, ImGuiWindowFlags_NoScrollbar);
        ImGui::Unindent();

        if (p_open && ImGui::Button("Close this window"))
            *p_open = false;
#endif
    }
    ImGui::End();
}

//EMSCRIPTEN_WEBGL_CONTEXT_HANDLE special_context;

// Main code
int main(int, char**)
{
  // Attempt to create a context in the gl-canvas for overlay.  Highly experimental
#if 0
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  attrs.alpha = 1;
  attrs.depth = 1;
  attrs.stencil = 1;
  attrs.antialias = 1;
  attrs.majorVersion = 1;
  attrs.minorVersion = 0;

  special_context = emscripten_webgl_create_context("#gl-canvas", &attrs);
  //emscripten_webgl_make_context_current(special_context);
#endif

  // END EXPERIMENT

    // State information (at the top, currently reset on page refresh)
    //
    static myApplicationState state;
    static bool show = true; // full screen?
    
    // Javascript setup
    initMap();
    initRAPIOLayer();
    initDebugLayer();

    //TESTSTATE(&state);

    // Create RAPIO map layer
    // Might not be ready, right?
 //   changeMapLayer(state.backgroundMapShow, state.backgroundMap, state.dataMap, state.showDebugTiles);

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    //SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_Window* window = SDL_CreateWindow("RAPIO Web Experiment", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    //ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImVec4 clear_color = ImVec4(0/255.0f, 88/255.0f, 128/255.0f, 1.00f);
    //ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.00f);

    // Set up theme once?  Ok?
    SetupImGuiStyle();

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;


    EMSCRIPTEN_MAINLOOP_BEGIN
#else
	    adsfasdfasdf
    while (!done)
#endif
    {

	 // Make sure context set in loop, right?
	//SDL_GL_MakeCurrent(window, gl_context);
	 
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

	//if (ImGui::Button("Increase Font Size")) {
	static bool first = true;
	if (first){
          //ImGui::GetIO().FontGlobalScale += 1.0f;
	  first = false;
	}

	if (is_layer_ready()){
	  // First post ready, set initial layers, etc.
	  static bool firstLayer = true;
          if (firstLayer){
	    changeMapLayer(state.backgroundMapShow, state.backgroundMap, state.dataMap, state.showDebugTiles);
	    firstLayer = false;
	  }
        ShowDesktop(&show, state);
	}else{
          ImGui::Text("Loading...");
	}

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);


	// Now try special context...
//	emscripten_webgl_make_context_current(special_context);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
