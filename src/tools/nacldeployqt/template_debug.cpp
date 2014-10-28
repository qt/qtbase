const char * templateDebug = R"STRING_DELIMITER( 

<!DOCTYPE html>
<!--
    "Debug" template for Qt on NaCl
-->
<html>
<head>
  <meta http-equiv="Pragma" content="no-cache">
  <meta http-equiv="Expires" content="-1">
  <title>%APPNAME%</title>

</head>
<body>
  <h2>Native Client Module: index</h2>
  <p>Status: <code id="status">Loading</code></p>

  <div id="listener">
    <embed id="nacl_module" name="%APPNAME%" src="%APPNAME%.nmf"
           type="%APPTYPE%" ps_stdout="/dev/tty" ps_stderr="/dev/tty"
           width=640 height=480 />
  </div>

  <p>Standard output/error:</p>
  <textarea id="stdout" rows="25" cols="80">
</textarea>

  <script>
listenerDiv = document.getElementById("listener")
stdout = document.getElementById("stdout")
nacl_module = document.getElementById("nacl_module")

function updateStatus(message) {
  document.getElementById("status").innerHTML = message
}

function addToStdout(message) {
  stdout.value += message;
  stdout.scrollTop = stdout.scrollHeight;
}

function handleMessage(message) {
  addToStdout(message.data)
}

function handleCrash(event) {
  updateStatus("Crashed/exited with status: " + nacl_module.exitStatus)
}

function handleLoad(event) {
  updateStatus("Loaded")
}

listenerDiv.addEventListener("load", handleLoad, true);
listenerDiv.addEventListener("message", handleMessage, true);
listenerDiv.addEventListener("crash", handleCrash, true);
  </script>
</body>
</html>

)STRING_DELIMITER";
