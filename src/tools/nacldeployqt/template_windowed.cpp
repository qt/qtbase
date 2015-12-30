const char * templateWindowed = R"STRING_DELIMITER( 

<!DOCTYPE html>

<!-- "windowed" template for Qt on NaCl -->

<html>

<head>
    <meta http-equiv="Pragma" content="no-cache">
    <meta http-equiv="Expires" content="-1">
    <title>%APPNAME%</title>
    <script src="qtloader.js" type="text/javascript"></script>
</head>

<style>
    .qt-container { width:640px; height:480px; display:block; margin: auto; box-shadow: 0 0 1em #888888; }
    .qt-container { display:flex; justify-content: center; align-items: center }
    .qt-embed { width:640px; height:480px; display:block; margin: auto; box-shadow: 0 0 1em #888888; }

    H2 { text-align: center; }
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

    // Create Qt element
    var qtEmbed = document.getElementById("qt-container")
    qt.createElement(qtEmbed)
    qt.load()
}
</script>

<body onload="init()">
<h2>%APPNAME%</h2>
<div id="qt-container"></div>
</body>

</html>

)STRING_DELIMITER";
