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
    embed { width:640px; height:480px; display:block; margin: auto; box-shadow: 0 0 1em #888888; }
    H2 { text-align: center; }
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
<h2>%APPNAME%</h2>
<div id="nacl-container"></div>
</body>

</html>

)STRING_DELIMITER";
