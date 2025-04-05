/**
 * Log-Polar Image module
 * Provides functions for converting standard images to log-polar format
 */

const lpx = require('bindings')('lpx_addon');
const fs = require('fs');
const path = require('path');

/**
 * Initialize the log-polar vision system
 * @param {string} scanTablesFile - Path to the scan tables file
 * @param {number} [width=0] - Width of the images (0 for auto)
 * @param {number} [height=0] - Height of the images (0 for auto)
 * @returns {boolean} - True if initialization was successful
 */
function initialize(scanTablesFile, width = 0, height = 0) {
  return lpx.initLPX(scanTablesFile, width, height);
}

/**
 * Shutdown the log-polar vision system
 */
function shutdown() {
  lpx.shutdownLPX();
}

/**
 * Start the webcam capture
 * @param {number} [deviceId=0] - Camera device ID
 * @returns {boolean} - True if camera started successfully
 */
function startCamera(deviceId = 0) {
  return lpx.startCamera(deviceId);
}

/**
 * Stop the webcam capture
 */
function stopCamera() {
  lpx.stopCamera();
}

/**
 * Capture a frame from the webcam and convert to log-polar
 * @param {number} [centerX=0.5] - Relative X coordinate of the center (0-1)
 * @param {number} [centerY=0.5] - Relative Y coordinate of the center (0-1)
 * @returns {Buffer} - Log-polar image data
 */
function captureFrame(centerX = 0.5, centerY = 0.5) {
  return lpx.captureFrame(centerX, centerY);
}

/**
 * Convert a standard image file to log-polar format
 * @param {string} imageFile - Path to the image file
 * @param {number} [centerX=0.5] - Relative X coordinate of the center (0-1)
 * @param {number} [centerY=0.5] - Relative Y coordinate of the center (0-1)
 * @returns {Buffer} - Log-polar image data
 */
function convertImageFile(imageFile, centerX = 0.5, centerY = 0.5) {
  return lpx.convertImageFile(imageFile, centerX, centerY);
}

/**
 * Render a log-polar image to a standard image
 * @param {Buffer} lpxData - Log-polar image data
 * @param {number} [width=640] - Width of the output image
 * @param {number} [height=480] - Height of the output image
 * @returns {Buffer} - JPEG image data
 */
function renderLogPolarImage(lpxData, width = 640, height = 480) {
  return lpx.renderLogPolarImage(lpxData, width, height);
}

/**
 * Save log-polar data to a file
 * @param {Buffer} lpxData - Log-polar image data
 * @param {string} filename - Output filename
 */
function saveLogPolarData(lpxData, filename) {
  fs.writeFileSync(filename, lpxData);
}

/**
 * Load log-polar data from a file
 * @param {string} filename - Input filename
 * @returns {Buffer} - Log-polar image data
 */
function loadLogPolarData(filename) {
  return fs.readFileSync(filename);
}

module.exports = {
  initialize,
  shutdown,
  startCamera,
  stopCamera,
  captureFrame,
  convertImageFile,
  renderLogPolarImage,
  saveLogPolarData,
  loadLogPolarData
};
