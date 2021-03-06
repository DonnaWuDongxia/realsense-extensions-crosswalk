var statusElement = document.getElementById('status');
var loadPhoto = document.getElementById('loadPhoto');
var saveButton = document.getElementById('save');
var stickerButton = document.getElementById('sticker');
var fileInput = document.getElementById('fileInput');
var measureRadio = document.getElementById('measure');
var refocusRadio = document.getElementById('refocus');
var pasteOnPlaneRadio = document.getElementById('pastOnPlane');
var popColorRadio = document.getElementById('popColor');
var imageCanvas = document.getElementById('image');
var overlayCanvas = document.getElementById('overlay');

var imageContext, imageData;
var overlayContext;
var refocus, depthMask, measurement, paster, photoUtils, XDMUtils;
var currentPhoto, savePhoto;
var canvasWidth, canvasHeight;

var clickCount = 0;
var startX = 0;
var startY = 0;
var endX = 0, endY = 0;
var hasImage = false;
var sticker;
var hasSelectPoints = false;

function drawCross(x, y) {
  overlayContext.beginPath();
  overlayContext.strokeStyle = 'blue';
  overlayContext.lineWidth = 2;
  overlayContext.moveTo(x - 7, y - 7);
  overlayContext.lineTo(x + 7, y + 7);
  overlayContext.stroke();
  overlayContext.moveTo(x + 7, y - 7);
  overlayContext.lineTo(x - 7, y + 7);
  overlayContext.stroke();
  overlayContext.closePath();
}

function resetRadioButtons() {
  measureRadio.checked = false;
  refocusRadio.checked = false;
  pasteOnPlaneRadio.checked = false;
  popColorRadio.checked = false;
}

function measureDistance(e) {
  if (hasImage == false)
    return;

  clickCount = clickCount + 1;
  var x = parseInt((e.clientX - overlayCanvas.offsetLeft) * canvasWidth / imageCanvas.scrollWidth);
  var y = parseInt((e.clientY - overlayCanvas.offsetTop) * canvasHeight / imageCanvas.scrollHeight);
  if (clickCount % 2 == 0) {
    drawCross(x, y);
    overlayContext.beginPath();
    overlayContext.moveTo(startX, startY);
    overlayContext.lineTo(x, y);
    overlayContext.strokeStyle = 'blue';
    overlayContext.lineWidth = 2;
    overlayContext.stroke();
    overlayContext.closePath();
    statusElement.innerHTML = 'Status Info : Measure: ';
    measurement.measureDistance(currentPhoto, { x: startX, y: startY }, { x: x, y: y }).then(
        function(d) {
          statusElement.innerHTML +=
              'Distance between(' + startX + ',' + startY + ') - (' + x + ',' + y + ') = ' +
              parseFloat(d.distance).toFixed(2) + ' millimeters, Confidence = ' +
              parseFloat(d.confidence).toFixed(2) + ', Precision=' +
              parseFloat(d.precision).toFixed(2) + 'mm';
          overlayContext.fillStyle = 'blue';
          overlayContext.font = 'bold 14px Arial';
          overlayContext.fillText(
              parseFloat(d.distance).toFixed(2) + ' mm',
              (startX + x) / 2, (startY + y) / 2 - 5);
        },
        function(e) { statusElement.innerHTML = e.message; });
  } else {
    overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
    drawCross(x, y);
    startX = x;
    startY = y;
  }
}

function depthRefocus(e) {
  if (hasImage == false)
    return;

  var x = parseInt((e.clientX - overlayCanvas.offsetLeft) * canvasWidth / imageCanvas.scrollWidth);
  var y = parseInt((e.clientY - overlayCanvas.offsetTop) * canvasHeight / imageCanvas.scrollHeight);

  overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
  drawCross(x, y);

  refocus.init(currentPhoto).then(
      function(success) {
        refocus.apply({ x: x, y: y }, 50.0).then(
            function(photo) {
              savePhoto = photo;
              photo.queryContainerImage().then(
                  function(image) {
                    imageData = imageContext.createImageData(image.width, image.height);
                    statusElement.innerHTML =
                        'Depth refocus success. Please select focus point again.';
                    overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
                    imageData.data.set(image.data);
                    imageContext.putImageData(imageData, 0, 0);
                  },
                  function(e) { statusElement.innerHTML = e.message; });
            },
            function(e) { statusElement.innerHTML = e.message; });
      },
      function(e) { statusElement.innerHTML = e.message; });
}

function doPasteOnPlane() {
  if (!hasImage || !pasteOnPlaneRadio.checked || !paster)
    return;

  if (!hasSelectPoints) {
    statusElement.innerHTML =
        'Select two points on the image to paste the sticker.';
    return;
  }

  var coordX = parseInt((startX + endX) / 2);
  var coordY = parseInt((startY + endY) / 2);
  const PI = 3.14159265;
  var rotation = 90 - 180 / PI * Math.atan2((endY - startY), (endX - startX));
  var stickerData = {
    height: -1,
    rotation: rotation,
    isCenter: true
  };

  paster.setPhoto(currentPhoto).then(
      function(success) {
        paster.setSticker(sticker, { x: coordX, y: coordY }, stickerData).then(
            function(success) {
              paster.paste().then(
                  function(photo) {
                    savePhoto = photo;
                    photo.queryContainerImage().then(
                        function(image) {
                          statusElement.innerHTML = 'Finished paste on plane.';
                          imageData.data.set(image.data);
                          imageContext.putImageData(imageData, 0, 0);
                        },
                        function(e) { statusElement.innerHTML = e.message; });
                  },
                  function(e) { statusElement.innerHTML = e.message; });
            },
            function(e) { statusElement.innerHTML = e.message; });
      },
      function(e) { statusElement.innerHTML = e.message; });
}

function pasteOnPlane(e) {
  if (hasImage == false || !sticker)
    return;

  clickCount = clickCount + 1;
  endX = parseInt((e.clientX - overlayCanvas.offsetLeft) * canvasWidth / imageCanvas.scrollWidth);
  endY = parseInt((e.clientY - overlayCanvas.offsetTop) * canvasHeight / imageCanvas.scrollHeight);
  if (clickCount % 2 == 0) {
    drawCross(endX, endY);
    overlayContext.beginPath();
    overlayContext.moveTo(startX, startY);
    overlayContext.lineTo(endX, endY);
    overlayContext.strokeStyle = 'blue';
    overlayContext.lineWidth = 2;
    overlayContext.stroke();
    overlayContext.closePath();

    hasSelectPoints = true;
    doPasteOnPlane();
  } else {
    overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
    drawCross(endX, endY);
    startX = endX;
    startY = endY;
  }
}

function popColor(e) {
  if (hasImage == false)
    return;

  var x = parseInt((e.clientX - overlayCanvas.offsetLeft) * canvasWidth / imageCanvas.scrollWidth);
  var y = parseInt((e.clientY - overlayCanvas.offsetTop) * canvasHeight / imageCanvas.scrollHeight);

  overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
  drawCross(x, y);

  depthMask.init(currentPhoto).then(
      function(success) {
        depthMask.computeFromCoordinate({ x: x, y: y }).then(
            function(maskImage) {
              currentPhoto.queryContainerImage().then(
                  function(colorImage) {
                    for (var x = 0; x < colorImage.width; x++) {
                      for (var y = 0; y < colorImage.height; y++) {
                        var index = y * colorImage.width * 4 + x * 4;
                        var maskIndex = y * maskImage.width + x;
                        var alpha = 1.0 - maskImage.data[maskIndex];

                        // BGR
                        var grey = 0.0722 * colorImage.data[index + 2] +
                            0.7152 * colorImage.data[index + 1] + 0.2126 * colorImage.data[index];

                        colorImage.data[index] =
                            parseInt(colorImage.data[index] * (1 - alpha) + grey * (alpha));
                        colorImage.data[index + 1] =
                            parseInt(colorImage.data[index + 1] * (1 - alpha) + grey * (alpha));
                        colorImage.data[index + 2] =
                            parseInt(colorImage.data[index + 2] * (1 - alpha) + grey * (alpha));
                      }
                    }

                    imageContext.clearRect(0, 0, canvasWidth, canvasHeight);
                    imageData = imageContext.createImageData(colorImage.width, colorImage.height);
                    imageData.data.set(colorImage.data);
                    imageContext.putImageData(imageData, 0, 0);

                    currentPhoto.clone().then(
                        function(photo) {
                          savePhoto = photo;
                          savePhoto.setContainerImage(colorImage).then(
                              function() {
                                statusElement.innerHTML =
                                    'Finish processing color pop, select again!';
                              },
                              function(e) { statusElement.innerHTML = e.message; });
                        },
                        function(e) { statusElement.innerHTML = e.message; });
                  },
                  function(e) { statusElement.innerHTML = e.message; });
            },
            function(e) { statusElement.innerHTML = e.message; });
      },
      function(e) { statusElement.innerHTML = e.message; });
}

function main() {
  try {
    refocus = new realsense.DepthEnabledPhotography.DepthRefocus();
    depthMask = new realsense.DepthEnabledPhotography.DepthMask();
    measurement = new realsense.DepthEnabledPhotography.Measurement();
  } catch (e) {
    statusElement.innerHTML = e.message;
    return;
  }

  photoUtils = realsense.DepthEnabledPhotography.PhotoUtils;
  XDMUtils = realsense.DepthEnabledPhotography.XDMUtils;

  imageContext = imageCanvas.getContext('2d');
  overlayContext = overlay.getContext('2d');

  fileInput.addEventListener('change', function(e) {
    var file = fileInput.files[0];
    var imageType = /image.*/;

    if (file.type.match(imageType)) {
      var reader = new FileReader();

      reader.onload = function(e) {
        var pastedImage = new Image();
        pastedImage.src = reader.result;

        var tempCanvas = document.createElement('canvas');
        var tempContext = tempCanvas.getContext('2d');
        tempContext.drawImage(pastedImage, 0, 0);
        var pastedImageData = tempContext.getImageData(
            0, 0, pastedImage.width, pastedImage.height);

        sticker = {
          format: 'RGB32',
          width: pastedImage.width,
          height: pastedImage.height,
          data: pastedImageData.data
        };

        doPasteOnPlane();
      };

      reader.readAsDataURL(file);
    } else {
      statusElement.innerHTML = 'File not supported!';
    }
  });

  measureRadio.addEventListener('click', function(e) {
    if (measureRadio.checked) {
      if (hasImage == false) {
        statusElement.innerHTML = 'Please load a photo first.';
        return;
      }

      statusElement.innerHTML = 'Select two points to measure distance.';
      overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
      currentPhoto.queryContainerImage().then(
          function(image) {
            imageContext.clearRect(0, 0, canvasWidth, canvasHeight);
            imageData = imageContext.createImageData(image.width, image.height);
            imageData.data.set(image.data);
            imageContext.putImageData(imageData, 0, 0);
          },
          function(e) { statusElement.innerHTML = e.message; });
    }
  }, false);

  refocusRadio.addEventListener('click', function(e) {
    if (refocusRadio.checked) {
      if (hasImage == false) {
        statusElement.innerHTML = 'Please load a photo first.';
        return;
      }

      statusElement.innerHTML = 'Select the refocus point.';
      overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
      currentPhoto.queryContainerImage().then(
          function(image) {
            imageContext.clearRect(0, 0, canvasWidth, canvasHeight);
            imageData = imageContext.createImageData(image.width, image.height);
            imageData.data.set(image.data);
            imageContext.putImageData(imageData, 0, 0);
          },
          function(e) { statusElement.innerHTML = e.message; });
    }
  }, false);

  pasteOnPlaneRadio.addEventListener('click', function(e) {
    if (pasteOnPlaneRadio.checked) {
      if (hasImage == false) {
        statusElement.innerHTML = 'Please load a photo first.';
        return;
      }

      if (!paster) {
        paster = new realsense.DepthEnabledPhotography.Paster();
      }

      stickerButton.disabled = false;
      fileInput.disabled = false;

      if (!sticker) {
        statusElement.innerHTML =
            'Please click [Set Sticker] button to load the pasted image.';
      } else {
        statusElement.innerHTML =
            'Select two points on the image to paste the sticker.';
      }

      overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
      currentPhoto.queryContainerImage().then(
          function(image) {
            imageContext.clearRect(0, 0, canvasWidth, canvasHeight);
            imageData = imageContext.createImageData(image.width, image.height);
            imageData.data.set(image.data);
            imageContext.putImageData(imageData, 0, 0);
          },
          function(e) { statusElement.innerHTML = e.message; });
    }
  }, false);

  popColorRadio.addEventListener('click', function(e) {
    if (popColorRadio.checked) {
      if (hasImage == false) {
        statusElement.innerHTML = 'Please load a photo first.';
        return;
      }

      statusElement.innerHTML = 'Select the point to pop color.';
      overlayContext.clearRect(0, 0, canvasWidth, canvasHeight);
      currentPhoto.queryContainerImage().then(
          function(image) {
            imageContext.clearRect(0, 0, canvasWidth, canvasHeight);
            imageData = imageContext.createImageData(image.width, image.height);
            imageData.data.set(image.data);
            imageContext.putImageData(imageData, 0, 0);
          },
          function(e) { statusElement.innerHTML = e.message; });
    }
  }, false);

  overlayCanvas.addEventListener('mousedown', function(e) {
    if (measureRadio.checked) {
      measureDistance(e);
    }
    if (refocusRadio.checked) {
      depthRefocus(e);
    }
    if (pasteOnPlaneRadio.checked) {
      pasteOnPlane(e);
    }
    if (popColorRadio.checked) {
      popColor(e);
    }
  }, false);

  saveButton.onclick = function(e) {
    if (!savePhoto) {
      statusElement.innerHTML = 'There is no photo to save';
      return;
    }
    XDMUtils.saveXDM(savePhoto).then(
        function(blob) {
          xwalk.experimental.native_file_system.requestNativeFileSystem('pictures',
              function(fs) {
                var fileName = '/pictures/depthphoto_' + RSUtils.getDateString() + '.jpg';
                fs.root.getFile(fileName, { create: true }, function(entry) {
                  entry.createWriter(function(writer) {
                    writer.onwriteend = function(e) {
                      statusElement.innerHTML =
                          'The depth photo has been saved to ' + fileName + ' successfully.';
                    };
                    writer.onerror = function(e) {
                      statusElement.innerHTML = 'Failed to save depth photo.';
                    };
                    writer.write(blob);
                  },
                  function(e) { statusElement.innerHTML = e; });
                },
                function(e) { statusElement.innerHTML = e; });
              });
        },
        function(e) { statusElement.innerHTML = e.message; });
  };

  loadPhoto.addEventListener('change', function(e) {
    var file = loadPhoto.files[0];
    XDMUtils.isXDM(file).then(
        function(success) {
          if (success) {
            XDMUtils.loadXDM(file).then(
                function(photo) {
                  currentPhoto = photo;
                  savePhoto = photo;
                  currentPhoto.queryContainerImage().then(
                      function(image) {
                        resetRadioButtons();
                        canvasWidth = image.width;
                        canvasHeight = image.height;
                        imageCanvas.width = canvasWidth;
                        imageCanvas.height = canvasHeight;
                        overlayCanvas.width = canvasWidth;
                        overlayCanvas.height = canvasHeight;
                        imageData = imageContext.createImageData(image.width, image.height);
                        statusElement.innerHTML = 'Load successfully.';
                        imageData.data.set(image.data);
                        imageContext.putImageData(imageData, 0, 0);
                        hasImage = true;

                        photoUtils.getDepthQuality(currentPhoto).then(
                            function(quality) {
                              statusElement.innerHTML += ' The photo quality is ' + quality;
                            },
                            function(e) { statusElement.innerHTML = e.message; });
                      },
                      function(e) { statusElement.innerHTML = e.message; });
                },
                function(e) { statusElement.innerHTML = e.message; });
          } else {
            statusElement.innerHTML = 'This is not a XDM file. Load failed.';
          }
        },
        function(e) { statusElement.innerHTML = e.message; });
  });
}
