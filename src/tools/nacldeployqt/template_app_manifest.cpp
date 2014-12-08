const char * templateAppManifest = R"STRING_DELIMITER( 
{
  "name": "%APPNAME%",
  "description": "%APPNAME%",
  "version": "0.1",
  "manifest_version": 2,
  "app": {
    "background": {
      "scripts": ["background.js"]
    }
  },
  "permissions": [ %PERMISSIONS% ]
}
)STRING_DELIMITER";
