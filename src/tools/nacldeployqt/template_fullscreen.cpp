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
    body { margin:0; overflow:hidden;}
    embed { height:100vh; width:100vw; }
</style>

<script type="text/javascript">

function init()
{
    // Create and configure Qt/NaCl controller
    var qt = new Qt({
        src : "%APPNAME%.nmf",
        type : "%APPTYPE%",
        query : window.location.search.substring(1),
        isChromeApp : "%CHROMEAPP%"
    });

    // Create and append nacl <embed>
    var qtEmbed = qt.createQtElement()
    document.getElementById("nacl-container").appendChild(qtEmbed)
}

</script>

<body onload="init()">
<div id="nacl-container"></div>
</body>

</html>

)STRING_DELIMITER";
