 /** Toomey August 2024
  * The module global object for emscripten/other layers
  */
var Module = {
  preRun: [],
  postRun: [],

  print: (function() {
    return function(text) {
      text = Array.prototype.slice.call(arguments).join(' ');
      console.log(text);
    };
  })(),

  printErr: function(text) {
    text = Array.prototype.slice.call(arguments).join(' ');
    console.error(text);
  },

  canvas: (function() {
    var canvas = document.getElementById('canvas');
    // canvas.addEventListener("webglcontextlost", function(e) { 
    //   alert('FIXME: WebGL context lost, please reload the page'); e.preventDefault(); 
    // }, false);
    return canvas;
  })(),

  setStatus: function(text) {
    //console.log("status3: " + text);
  },

  monitorRunDependencies: function(left) {
    // no run dependencies to log
  },

  onRuntimeInitialized: function() {
   // Note:  Have access to Module additions created by c++
   // console.log('lerp result: ' + Module.lerp(1, 2, 0.5));
  }
};

window.onerror = function(event) {
  console.log("onerror: " + event);
};
