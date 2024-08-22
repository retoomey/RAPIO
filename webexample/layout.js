/** Toomey August 2024
 * Code for my app, creates a vertical divider between
 * an imgui emscripten app and leaflet window.
 */
const container = document.getElementById('container');
const leftCanvas = document.getElementById('leftDiv');
const rightCanvas = document.getElementById('rightDiv');
const separator = document.getElementById('vbar');

let isDragging = false;

separator.addEventListener('mousedown', (e) => {
    isDragging = true;
    document.body.style.cursor = 'ew-resize';
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;

    const containerRect = container.getBoundingClientRect();
    const offsetX = e.clientX - containerRect.left;
    const leftWidth = (offsetX / containerRect.width) * 100;
    const rightWidth = 100 - leftWidth;

    leftCanvas.style.width = `${leftWidth}%`;
    rightCanvas.style.width = `${rightWidth}%`;

    // Need to fire to reextent the windows properly
    window.dispatchEvent(new Event('resize'));
});

document.addEventListener('mouseup', () => {
    isDragging = false;
    document.body.style.cursor = 'default';
});
