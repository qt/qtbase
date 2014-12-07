const char * templateBackgroundJs = R"STRING_DELIMITER(

chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create('%MAINHTML%', {
    'bounds': {
      'width': 400,
      'height': 500
    }
  });
});

)STRING_DELIMITER";
