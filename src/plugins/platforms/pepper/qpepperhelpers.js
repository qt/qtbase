this.qCallFunction = function(message)
{
    // Expect message to be in the form "tag:code"
    var parts = message.split(":");
    var tag = parts[0];
    var code = parts[1];

    // Eval code
    var result = eval(code);
    // Post the result back as a tagged message
    this.postMessage(tag + ":" + result);
}

qGetDevicePixelRatio = function()
{
    var dpr = 1;
    if(window.devicePixelRatio !== undefined) dpr = window.devicePixelRatio;
    return dpr;
}

