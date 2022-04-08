# CMake Utils

This directory holds scripts to help the porting process from `qmake` to `cmake` for Qt6.

If you're looking to port your own Qt-based project from `qmake` to `cmake`, please use
[qmake2cmake](https://wiki.qt.io/Qmake2cmake).

# Requirements

* [Python 3.7](https://www.python.org/downloads/),
* `pipenv` or `pip` to manage the modules.

## Python modules

Since Python has many ways of handling projects, you have a couple of options to
install the dependencies of the scripts:

### Using `pipenv`

The dependencies are specified on the `Pipfile`, so you just need to run
`pipenv install` and that will automatically create a virtual environment
that you can activate with a `pipenv shell`.

### Using `pip`

It's highly recommended to use a [virtualenvironment](https://virtualenv.pypa.io/en/latest/)
to avoid conflict with other packages that are already installed: `pip install virtualenv`.

* Create an environment: `virtualenv env`,
* Activate the environment: `source env/bin/activate`
  (on Windows: `source env\Scripts\activate.bat`)
* Install the requirements: `pip install -r requirements.txt`

If the `pip install` command above doesn't work, try:

```
python3.7 -m pip install -r requirements.txt
```

# Contributing to the scripts

You can verify if the styling of a script is compliant with PEP8, with a couple of exceptions:

Install [flake8](http://flake8.pycqa.org/en/latest/) (`pip install flake8`) and run it
on all python source files:

```
make flake8
```

You can also modify the file with an automatic formatter,
like [black](https://black.readthedocs.io/en/stable/) (`pip install black`),
and execute it:

```
make format
```
