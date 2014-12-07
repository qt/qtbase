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

<body>
<h2>%APPNAME%</h2>
</body>

</html>

)STRING_DELIMITER";
