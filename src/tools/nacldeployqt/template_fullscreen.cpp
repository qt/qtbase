const char * templateFullscreen = R"STRING_DELIMITER( 

<!DOCTYPE html>

<!-- "fullscreen" template for Qt on NaCl -->

<html>

<head>
    <meta http-equiv="Pragma" content="no-cache">
    <meta http-equiv="Expires" content="-1">
    <title>%APPNAME%</title>
    <script src="qtloader.js" type="text/javascript"></script>
</head>

<style>
    .qt-container { height:100vh; width:100vw; }
    .qt-container { display:flex; justify-content: center; align-items: center }
    .qt-embed  { height:100vh; width:100vw; }

    body { margin:0; overflow:hidden; }
</style>

<script type="text/javascript">

function init()
{
    // Create and configure Qt/NaCl controller
    var qt = new QtLoader({
        src : "%APPSOURCE%",
        type : "%APPTYPE%",
        query : window.location.search.substring(1),
        isChromeApp : "%CHROMEAPP%"
    });

    // Create and append nacl <embed>
    var qtEmbed = qt.createElement()
    document.body.appendChild(qtEmbed)
    qt.load();
}

</script>

<body onload="init()">
</body>

</html>

)STRING_DELIMITER";
