# qtwasmtestrunner
This is a utility that launches a small webserver and\
either a browser or a webdriver (only chrome/chromedriver at the time of writing)\
This allows running wasm tests and printing the output to stdout like a normal test.

chromedriver must be installed: https://chromedriver.chromium.org/ \
to use it with chromedriver (default operation), and it must be in PATH\
unless --chromedriver_path is passed with full path to chromedriver

Run the script with --help for more info.
