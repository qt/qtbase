const char * templateFullscreen = R"STRING_DELIMITER( 

<!DOCTYPE html>

<!-- "fullscreen" template for Qt on NaCl -->

<html>

<head>
    <meta http-equiv="Pragma" content="no-cache">
    <meta http-equiv="Expires" content="-1">
    <title>%APPNAME%</title>
</head>

<style>
    body { margin:0; overflow:hidden;}
    embed { height:100vh; width:100vw; }
</style>

<script>
    function decodeQuery() {
        var query = window.location.search.substring(1);
        var vars = query.split('&');
        var keyValues = {}
        for (var i = 0; i < vars.length; i++) {
            // split key/value on the first '='
            var key = decodeURIComponent(vars[i].split(/=(.+)/ )[0]);
            var value = decodeURIComponent(vars[i].split(/=(.+)/ )[1]);
            if (key && key.length > 0)
                keyValues[key] = value;
        }
        return keyValues;
    }

    function createNaClEmbed()
    {
        // Create NaCl <embed> element.
        var embed = document.createElement("EMBED");
        embed.setAttribute("id", "nacl_module");
        embed.setAttribute("name", "%APPNAME%");
        embed.setAttribute("src", "%APPNAME%.nmf");
        embed.setAttribute("type", "%APPTYPE%");

        // Decode and set URL query string values as attributes on the embed tag.
        // This allows passing for example ?QSG_VISUALIZE=overdraw
        var query = decodeQuery();
        for (var key in query) {
            if (key !== undefined)
                embed.setAttribute(key, query[key])
        }
        document.body.appendChild(embed);
    }

    document.addEventListener("DOMContentLoaded", createNaClEmbed);

</script>

<body>
</body>

</html>

)STRING_DELIMITER";
