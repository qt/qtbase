const char * templateWindowed = R"STRING_DELIMITER( 

<!DOCTYPE html>

<!-- "windowed" template for Qt on NaCl -->

<html>

<head>
    <meta http-equiv="Pragma" content="no-cache">
    <meta http-equiv="Expires" content="-1">
    <title>%APPNAME%</title>
</head>

<style>
    embed { width:640px; height:480px; display:block; margin: auto; }
    H2 { text-align: center; }
</style>

<script>
%LOADERSCRIPT%
</script>

<body>
<h2>%APPNAME%</h2>
</body>

</html>

)STRING_DELIMITER";
