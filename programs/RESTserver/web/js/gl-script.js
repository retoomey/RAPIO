// How to ESM or whatever 'module' everything?
var program;
var positionBuffer;
var visibleReadout = false;

function initGLCanvas() {
    var canvas = document.getElementById('gl-canvas');
    var gl = canvas.getContext('webgl');

    if (!gl) {
        console.error('WebGL not supported.');
        return;
    }

    // Set the canvas size
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;

    // Make sure opengl matches size
    gl.viewport(0, 0, canvas.width, canvas.height);

    // Enable alpha blending
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

    // Clear the canvas with a transparent color
    gl.clearColor(0.0, 0.0, 0.0, 0.0); // RGBA: Black with 0 alpha (transparent)
    gl.clear(gl.COLOR_BUFFER_BIT);

    // Define the vertex and fragment shaders
    var vertexShaderSource = `
        attribute vec4 a_position;
        void main() {
            gl_Position = a_position;
        }
    `;
    var fragmentShaderSource = `
        precision mediump float;
        void main() {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 0.5); // Semi-transparent red
        }
    `;

    // Compile shaders and link program
    var vertexShader = createShader(gl, gl.VERTEX_SHADER, vertexShaderSource);
    var fragmentShader = createShader(gl, gl.FRAGMENT_SHADER, fragmentShaderSource);
    program = createProgram(gl, vertexShader, fragmentShader);
    gl.useProgram(program);

    // Create a buffer and put a single clip space rectangle in
    positionBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);

    // FIXME: probably not needed
    if (visibleReadout){
      // Draw the initial rectangle
      drawReadout(window.innerWidth / 2, window.innerHeight / 2);
    }
}

function createShader(gl, type, source) {
    var shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        console.error(gl.getShaderInfoLog(shader));
        gl.deleteShader(shader);
        return null;
    }
    return shader;
}

function createProgram(gl, vertexShader, fragmentShader) {
    var program = gl.createProgram();
    gl.attachShader(program, vertexShader);
    gl.attachShader(program, fragmentShader);
    gl.linkProgram(program);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        console.error(gl.getProgramInfoLog(program));
        gl.deleteProgram(program);
        return null;
    }
    return program;
}

// Update position buffer with new clip space coordinates
function updateRectanglePosition(gl, program, clipX, clipY) {
    var canvas = document.getElementById('gl-canvas');
    
    // Define the rectangle size (50x50 pixels)
    var rectWidth = 50 / canvas.width * 2; // Convert to clip space
    var rectHeight = 50 / canvas.height * 2; // Convert to clip space

    // Create the rectangle vertices centered around the clip space position
    var positions = new Float32Array([
        clipX, clipY,
        clipX + rectWidth, clipY,
        clipX, clipY - rectHeight,
        clipX + rectWidth, clipY - rectHeight
    ]);

    // Update the position buffer
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, positions, gl.STATIC_DRAW);

    // Lookup attribute location
    var positionLocation = gl.getAttribLocation(program, 'a_position');
    gl.enableVertexAttribArray(positionLocation);
    gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);
}

function clearReadout() {
   visibleReadout = false;
   var canvas = document.getElementById('gl-canvas');
   var gl = canvas.getContext('webgl');
   gl.clear(gl.COLOR_BUFFER_BIT);
}

function drawReadout(clientX, clientY) {
   visibleReadout = true;
   // console.log("X and Y is " + clientX + " " + clientY);
    var canvas = document.getElementById('gl-canvas');
    var gl = canvas.getContext('webgl');
    var rect = canvas.getBoundingClientRect(); // Get canvas position and size

    // Convert mouse coordinates to canvas space
    var mouseX = clientX - rect.left;
    var mouseY = clientY - rect.top;

    // Convert canvas coordinates to clip space (-1 to 1)
    //var clipX = (mouseX / canvas.width) * 2 - 1;
    //var clipY = (mouseY / canvas.height) * -2 + 1;
    var clipX = (mouseX / rect.width) * 2 - 1;
    var clipY = (mouseY / rect.height) * -2 + 1;

    // Update rectangle position
    updateRectanglePosition(gl, program, clipX, clipY);

    // FIXME: probably not needed
    gl.clear(gl.COLOR_BUFFER_BIT);
    if (visibleReadout){
      gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    }
}

// Handle window resize
window.addEventListener('resize', function() {
    //var canvas = document.getElementById('gl-canvas');
    //canvas.width = window.innerWidth;
    //canvas.height = window.innerHeight;

    // Do we need to clear?
    //var gl = canvas.getContext('webgl');
    //gl.viewport(0, 0, canvas.width, canvas.height);
    //gl.clear(gl.COLOR_BUFFER_BIT);

    initGLCanvas(); // Redraw the OpenGL content
});

// Initialize the canvas when the window is fully loaded
window.addEventListener('load', function() {
    console.log("------------>LOADED GL");
    initGLCanvas();
});
